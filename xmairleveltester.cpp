/*
 *  Copyright (C) 2017 Felix Homann
 *
 *  This file is part of xmairleveltest.
 *
 *  xmairleveltest is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with xmairleveltest.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xmairleveltester.h"

#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <memory>
#include <regex>
#include <iterator>
#include <fstream>
#include <sstream>

#include "xrm32level.hpp"

using namespace std::literals;
const auto DELAY = 10ms;
const int XMAIR_PORT = 10024;

XMAirLevelTester::XMAirLevelTester(uint channel) :
  _step{0},
  _channel{channel},
  _broadcast_addr{"255.255.255.255", XMAIR_PORT},
  _xinfo_msg{"N"},
  _lo_server{nullptr},
  _fader_level_types{"f"},
  _fader_db_types{"s"}
{
  if (_lo_server.is_valid()) {
    std::cout << "Server is valid!" << std::endl;

    // add device info handler
    _lo_server.add_method("/info", "ssss",
			  [this](const char* path, const lo::Message &msg) {
			    return this->_info_handler(path, msg);
			  });

    // Build path for fader handler
    std::string channel_num = _channel < 10 ? "0" + std::to_string(_channel)
					 : std::to_string(_channel);
    _fader_level_path = "/ch/" + channel_num + "/mix/fader";

    // Add fader handler
    _lo_server.add_method(_fader_level_path.c_str(), "f",
			  [this](const char* path, const lo::Message &msg)
			  {
			    return this->_fader_float_handler(path, msg);
			  });
    // Build path for fader dB handler
    _fader_db_node_msg = "ch/" + channel_num + "/mix/fader";
    // Add fader db handler
    _lo_server.add_method("node", "s",
			  [this](const char* path, const lo::Message &msg)
			  {
			    return this->_fader_db_handler(path, msg);
			  });

    std::cout << "Tester URL: " << _lo_server.url() << std::endl;
    _lo_server.start();
  }
}

XMAirLevelTester::~XMAirLevelTester()
{
  stop();
}

void XMAirLevelTester::stop()
{
  _lo_server.stop();
}

std::shared_ptr<lo::Address> XMAirLevelTester::find_mixer()
{
  // We get a future. The promise will be set by the _fader_handler.
  auto future_mixer_addr = _promise_mixer_addr.get_future();
  _broadcast_addr.send_from(_lo_server,  "/info", "N", nullptr);
  auto status = future_mixer_addr.wait_for(std::chrono::seconds(1));

   if (status != std::future_status::ready) {
     std::cout << "find_mixer() returns nullptr." << std::endl;
     return nullptr;
   }

   auto mixer_ptr = future_mixer_addr.get();

   return mixer_ptr;
}

void XMAirLevelTester::run_tests(std::shared_ptr<lo::Address> mixer, uint num_steps, bool log = true)
{
  uint mismatch_counter_float = 0;
  uint mismatch_counter_db =0;

  std::vector<int> mismatch_indices;
  std::cout << "Running tests on mixer at " <<  mixer->url()
	    << " on channel " << _channel << "." << std::endl;

  for (int i = 0; i < num_steps; ++i) {
    int err = check_fader_level(mixer, i * 1.0f/(num_steps - 1), log);
    if (1 & err) {
      ++mismatch_counter_float;
    }
    if (2 & err) {
      ++mismatch_counter_db;
    }
    if (err > 0) {
      mismatch_indices.push_back(i);
    }
  }

  // std::this_thread::sleep_for(500ms);
  std::cout << "===========" << std::endl;
  std::cout << "Number of mismatches(float): " << mismatch_counter_float << std::endl;
  std::cout << "Number of mismatches(db): " << mismatch_counter_db << std::endl;
  std::cout << "\nMismatches:" << std::endl;

  for (auto i : mismatch_indices) {
    check_fader_level(mixer, i * 1.0f/(num_steps - 1), true); // always log mismatches
  }
  std::cout << std::endl;
}


int XMAirLevelTester::count_node_db(std::shared_ptr<lo::Address> mixer_addr)
{
  std::string last_db("");
  int count = 0;
  for (int i = 0; i < 1024; ++i) {
    float level = i * 1.0f/(1024 - 1);
    mixer_addr->send_from(_lo_server, _fader_level_path.c_str(), "f", level);
    // Query dB string
    auto future_fader_db = _promise_fader_db.get_future();
    mixer_addr->send_from(_lo_server,"/node", "s", _fader_db_node_msg.c_str());
    auto node_db = future_fader_db.get();
    if (node_db != last_db) {
      ++count;
      last_db = node_db;
    }
  }
  return count;
}


int XMAirLevelTester::check_fader_level(std::shared_ptr<lo::Address> mixer_addr, float flevel, bool log)
{
  int err = 0;
  Xrm32::Level<1024> level; // Our Level implementation
  level.setFloat(flevel);
  // Send a set message to the console
  set_fader_float(mixer_addr, flevel);

  // Now ask for the value
  auto actual_fader_level = query_fader_float(mixer_addr);

  // Query dB string
  auto future_fader_db = _promise_fader_db.get_future();
  mixer_addr->send_from(_lo_server,"/node", "s", _fader_db_node_msg.c_str());
  auto node_db = future_fader_db.get();

  // Diagnostics
  if (log) {
    std:: cout << "Index: " << level.getIndex()
	       << "   level.getFloat(): " << level.getFloat()
	       << "   Received float: " << actual_fader_level
	       << "   Match(float): " << (actual_fader_level == level.getFloat())
	       << "   dB: " << level.getDb()
	       << "   level.getOscString(): " << level.getOscString()
	       << "   node_db: " << node_db
	       << "   Match(dB): " << (node_db == level.getOscString())
	       << std::endl;
  }
  if (actual_fader_level != level.getFloat()) {
    err = 1;
  }

  if(node_db != level.getOscString()) {
    err |= 1 << 1;
  }

  return err;
}

void XMAirLevelTester::set_fader_float(std::shared_ptr<lo::Address> mixer_addr, float flevel)
{
  mixer_addr->send_from(_lo_server, _fader_level_path.c_str(), "f", flevel);
  std::this_thread::sleep_for(DELAY); // Make sure the mixer isn't overrun by requests
}

float XMAirLevelTester::query_fader_float(std::shared_ptr<lo::Address> mixer_addr)
{
  auto future_fader_level = _promise_fader_level.get_future();
  mixer_addr->send_from(_lo_server,_fader_level_path.c_str(), "", nullptr);
  auto status = future_fader_level.wait_for(std::chrono::seconds(1));
  float retval = status == std::future_status::ready ? future_fader_level.get() : -1.0f;

  return retval;
}

void XMAirLevelTester::set_fader_db(std::shared_ptr<lo::Address> mixer_addr, std::string db)
{
  mixer_addr->send_from(_lo_server,  _fader_level_path.c_str(), "s", db.c_str());
  std::this_thread::sleep_for(DELAY); // Make sure the mixer isn't overrun by requests
}

std::string XMAirLevelTester::query_fader_db(std::shared_ptr<lo::Address> mixer_addr)
{
  auto future_fader_db = _promise_fader_db.get_future();
  mixer_addr->send_from(_lo_server,"/node", "s", _fader_db_node_msg.c_str());
  auto status = future_fader_db.wait_for(std::chrono::seconds(1));
  std::string retval = status == std::future_status::ready ? future_fader_db.get() : "TIMEOUT";

  return retval;
}

int XMAirLevelTester::_fader_float_handler(const char* path, const lo::Message &msg)
{
  // Use a lock guard to keep this code from being called concurrently
  std::lock_guard<std::mutex> lock(_mtx_fader_float);
  float received_value = msg.argv()[0]->f;
  // We need a new promise so that the handler can be called repeatedly
  std::promise<float> promise_tmp;
  std::swap(promise_tmp, _promise_fader_level);
  promise_tmp.set_value(received_value);
  return 1;
}

int XMAirLevelTester::_fader_db_handler(const char* path, const lo::Message &msg)
{
  // Use a lock guard to keep this code from being called concurrently
  std::lock_guard<std::mutex> lock(_mtx_fader_db);
  std::string node_reply = &msg.argv()[0]->s;
  // As a reply to our node query we  expect messages from the mixer on path "node"
  // of the form "/ch/12/mix/fader -10.0". The last part is the "Node" dB value as
  // a string.
  std::string regex_str = "(" + _fader_level_path + ")(\\s+)(.*)(\\s*)";
  std::regex fader_regex(regex_str);
  std::smatch match;

  if (std::regex_match(node_reply, match, fader_regex) ) {
     std::ssub_match sub_match = match[3];
     std::string db_string = sub_match.str();
     std::promise<std::string> promise_tmp;
     std::swap(promise_tmp, _promise_fader_db);
     promise_tmp.set_value(db_string);
  }

  return 1;
}

int XMAirLevelTester::_info_handler(const char *path, const lo::Message &msg)
{
  // Use a lock guard to keep this code from being called concurrently
  std::lock_guard<std::mutex> lock(_mtx_info);
  std::cout << "Found X Air device"
  	    << "\nName: " << &msg.argv()[1]->s
  	    << "\nModel: " << &msg.argv()[2]->s
  	    << "\nRev.: " <<  &msg.argv()[0]->s
  	    << "\nFirmware: " << &msg.argv()[3]->s
  	    << "\n" << std::endl;

  // In case this method is called repeatedly
  // we need another promise:
  std::promise<std::shared_ptr<lo::Address>> promise_tmp;
  std::swap(promise_tmp, _promise_mixer_addr);

  // Somehow we can't use a lo::Address directly as a promise value.
  // Work aroudn with a shared_ptr.
  //TODO: Find out why?
  auto mixer_ptr = std::make_shared<lo::Address>(msg.source().url());
  promise_tmp.set_value(mixer_ptr);

  return 0;
}
