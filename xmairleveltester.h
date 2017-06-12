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

#ifndef XMAIRLEVELTESTER_H
#define XMAIRLEVELTESTER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <future>
#include <thread>

#include <lo/lo_cpp.h>

#include "xrm32level.hpp"

class XMAirLevelTester {
public:
    /**
     * @brief XMAirLevelTester
     * @param channel The channel whose controls the tests are going to use.
     */
    XMAirLevelTester(uint channel);
    ~XMAirLevelTester();

    /**
     * @brief run_tests Start testing the fader levels of specified channel.
     * @param mixer_addr Mixer address used for testing.
     * @param num_steps Number of equidistant test levels.
     */
    void run_tests(const lo::Address& mixer_addr, uint num_steps, bool log);

    /**
     * @brief stop Stop the test.
     */
     void stop();

     /**
      * @brief Find a suitable mixer in the network. Currently only X/M Air mixers
      *        are being used. This is a simplistic method that might crash
      *        whenever there's more than one mixer on the network!
      * @return Returns a pointer to a lo:Address or nullptr if none is found.
      */
     lo::Address* find_mixer(); // Search for a mixer console

     /**
      * @brief check_fader_level Sets the tester channel's fader and a Xrm32Level to 'level'. Afterwards compares
      *        the actual levels on both in the float and the dB domain.
      * @param mixer_addr The mixer to use.
      * @param flevel The float level to test.
      * @param log Wether test details should be logged.
      * @return Returns a bitfield. The '1' bit set indicates mismatch in the float domain
      *         the '2' bit set indicates a mismatch in the dB representation.
      */
     int check_fader_level(const lo::Address& mixer_addr, float flevel, bool log = true); // Test a level

     /**
      * @brief Count distinct Node fader values
      * @param mixer_addr Address of mixer to run the test on.
      */
     int count_node_db(const lo::Address& mixer_addr);


     /**
      * @brief Set the fader level by float representation.
      * @param flevel The feder float level.
      */
     void set_fader_float(const lo::Address& mixer_addr, float flevel);

     /**
      * @brief Query the current float representation of the fader.
      * @return Current fader level (float) or -1.f in case of error.
      */
     float query_fader_float(const lo::Address& mixer_addr);

     /**
      * @brief Set the fader level by a dB string e.g. "-10.4", "+2.4".
      * @param db dB value as a string, e.g. "-10.4", "+2.4".
      * @return Current fader level (float) or -1.f in case of error
      */
     void set_fader_db(const lo::Address& mixer_addr, std::string db);

     /**
      * @brief Query the current dB string representation of the fader.
      * @return Current fader level (dB) as string
      */
     std::string query_fader_db(const lo::Address& mixer_addr);

private:
    uint _num_steps; // Number of steps to test
    uint _step = 0;
    uint _channel; // channel number to use for testing
    std::promise<lo::Address*> _promise_mixer_addr;
    std::promise<float> _promise_fader_level;
    std::promise<std::string> _promise_fader_db;
    lo::Address _broadcast_addr;
    lo::Message _xinfo_msg;
    lo::ServerThread _lo_server;
    std::string _fader_level_path, _fader_db_node_msg;
    std::string _fader_level_types, _fader_db_types;
    int _info_handler(const char* path, const lo::Message &msg);
    int _fader_float_handler(const char* path, const lo::Message &msg);
    int _fader_db_handler(const char* path, const lo::Message &msg);
    std::mutex _mtx_info, _mtx_fader_float, _mtx_fader_db, _mtx_db;
};

#endif // XMAIRLEVELTESTER_H
