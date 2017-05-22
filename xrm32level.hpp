/*
 *  Copyright (C) 2015 Felix Homann
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 */


#pragma once
#include <atomic>
#include <cmath>
#include <string>

namespace Xrm32 {

template<uint N>
class Level {
public:

    explicit Level(float level = 0.f)
    {
        static_assert(N > 0, "Template parameter N has to be greater than 0!");
        setFloat(level);
    }

    explicit Level(std::string osc_value_string)
    {
        setOscString(osc_value_string);
    }

    virtual ~Level()
    {
        // Nothing to do here.
    }

    /**
     * @brief setFloat Set Level by float value.
     * @param level
     */
    void setFloat(float level)
    {
        _idx = indexFromFloat(level);
    }

    /**
     * @brief getFloat Get the Level's float representation
     * @return Float value [0.f, 1.0f]
     */
    float getFloat() const
    {
        return static_cast<float>(_idx) / (N - 1);
    }

    /**
     * @brief getDb Get dB representation of this Level.
     * @return dB value.
     */
    float getDb() const
    {
        // Conversion according to Behringer
        float db;
        const uint idx = _idx; // Get a working copy of the atomic value

        if (idx >= N / 2) {
	  db = (40.f * idx) / (N - 1) - 30;
        } else if (idx >= N / 4) {
	  db = (80.f * idx) / (N - 1) - 50;
        } else if (idx >= N / 16) {
	  db = (160.f * idx) / (N - 1) - 70;
        } else if ( idx > 0) {
	  db = (480.f * idx) / (N - 1) - 90;
        } else { // _idx == 0
            db = -144.f;
        }

        return db;
    }

    /**
     * @brief indexFromFloat Static conversion function.
     * @param flevel Float represention of level [0.f ... 1.0f]
     * @return Index corresponding to float level.
     */
    static uint indexFromFloat(float flevel)
    {
        // "Clip" flevel
        if (flevel > 1.0f) {
            flevel = 1.0f;
        } else if (flevel <= 0) {
            flevel = 0;
        }

        // Index rounding according to private email from Jan Duwe @ Behringer
        uint idx = static_cast<uint>(flevel * (N - 1 + 0.5f));

        // "Clip" index
        if (idx > N - 1) {
            idx = N - 1;
        }

        return idx;
    }


    /**
     * @brief indexFromDb Static conversion function from dB to index.
     * @param db A level's dB value
     * @return Index of dB value
     */
    static uint indexFromDb(float db)
    {
        float level;
        if (db >= (40.f * N) / (2 * (N - 1)) - 30) {
            level = (db + 30) / 40;
        } else if (db >= (80.f * N) / (4 * (N - 1)) - 50) {
            level = (db + 50) / 80;
        } else if (db >= (160.f * N) / (16 * (N - 1)) - 70) {
            level = (db + 70) / 160;
        } else if (db > -90) {
            level = (db + 90) / 480;
        } else { // db <= -90, idx = 0;
            level = 0;
        }

        uint idx = (uint)(level * (N - 1 + 0.5f));
        return idx;
    }

    /**
     * @brief setDb Set Level by dB value
     * @param db
     */
    void setDb(float db)
    {
        _idx = indexFromDb(db);
    }

    /**
     * @brief getNumSteps Get number of steps used in this level.
     * @return Number of steps
     */
    static uint getNumSteps()
    {
        return N;
    }

    /**
     * @brief getOscString Get an OSC string representation of this level.
     * @return OSC string, in this case the dB value as string.
     */
    std::string getOscString() const {
      float dB = getDb();

      std::string dbString;
      if (dB == -144.f) {
	dbString = "-oo";
      } else {
	 std::string sign = dB < 0 ? "-" : "+";
	 if (dB < 0) {
	   dB = -dB;
	 }
	 float rounded = static_cast<int>(10 * dB + 0.5f) * 0.1f;
	 int dbInt = static_cast<int>(rounded);
	 int fractional = 10 * rounded - 10 * static_cast<int>(rounded) ;
	 if (dbInt == 0 && fractional == 0) {
	   sign = "";
	 }
	 dbString = sign + std::to_string(dbInt) + "." + std::to_string(fractional);

      }
      return dbString;
    }

    /**
     * @brief setOscString Set Level by OSC string
     * @param val Signed dB value as string, e.g. "-10.0" or "+2.0"
     */
    void setOscString(std::string val) {
        if (val == "-oo") {
            _idx = 0;
        } else {
            float db = std::stof(val);
            setDb(db);
        }
    }

    /**
     * @brief getIndex Get the index representation of this Level
     * @return Index representation
     */
    uint getIndex() const {
        return _idx;
    }

    /**
     * @brief setIndex Set Level by index.
     * @param index
     */
    void setIndex(uint index) {
        if (index > N - 1) {
            _idx = N - 1;
        } else {
            _idx = index;
        }
    }

private:
    std::atomic<uint> _idx; // We use an atomic here so we
};

}
