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

#include <cmath>
#include <iostream>
#include <vector>

#include "xrm32level.hpp"
#include "xmairleveltester.h"

using namespace std::literals;
const uint CHANNEL = 12;

int main(int argc, char* argv[])
{
  std::cout << "Test the Xrm32Level implementation!" << std::endl;

  XMAirLevelTester tester(CHANNEL);
  std::shared_ptr<lo::Address> mixer = tester.find_mixer();
  if (mixer == nullptr) {
    std::cout << "No mixer found!" << std::endl;
    tester.stop();
    return -1;
  }

  uint num_steps = 1024*4;
  tester.run_tests(mixer, num_steps, true);

  std::cout << "\nThe expected result currently is that we get two dB mismatches for index 765 and 769 respectively."
  	    << "\nThe desktop apps seem to give the same dB values for those levels.\n" << std::endl;

  // // Count distinct dB Strings.
  tester.count_node_db(mixer);
  std::cout << "\nExpected number of distinct values is 658.\n" << std::endl;

  // Check Patrick-Gilles Maillot's rounding formula
  // as given on page 110 of his X32 OSC documentation.
  std::cout << "The Xrm32Level's mapping/rounding of float values to the actual console values"
	    << " should have proved correct above."
	    << "\nNow check Patrick-Gilles Maillot's rounding via roundf().\n" << std::endl;
  Xrm32::Level<1024> level;
  int count = 0;
  for (int i = 0; i < num_steps; ++i) {
    float flevel = i * 1.f /(num_steps - 1);
    level.setFloat(flevel);
    float rounded = roundf(flevel * 1023) / 1023; // rounding according to p.110
    float actual_value = level.getFloat();
    bool match = rounded == actual_value;
    if (!match) {
      ++count;
    }
    // std::cout << "flevel: " << flevel << "   Actual: " << actual_value << "   roundf: "
    // 	      << rounded << "   match: " << match << std::endl;
  }

  std::cout << "roundf() rounding didn't match the correct values " << count  << " times!" << std::endl;

  return 0;
}
