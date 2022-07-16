/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * This file contains code derived from the mogillc/nico library by Matt Bunting, Copyright 2016 by
 * Mogi, LLC and distributed under the LGPL library version 2 license.
 * The original version can be found here: https://github.com/mogillc/nico
 * 
 */
#include "timer.hpp"
#include <sys/time.h>

using namespace Chaos;

Timer::Timer() {}

void Timer::initialize() {
	// timecycle = chr::time_point_cast<usec>(chr::steady_clock::now());
  gettimeofday(&tv, nullptr); // Grab the current time (simply an initialization)
	timecycle = tv.tv_sec + tv.tv_usec * 1e-6; // Compute the current time, in seconds
	runningtime = 0;
  // runningtime = usec::zero();
}

void Timer::update() {
	oldtimecycle = timecycle;  // Store the old time
  gettimeofday(&tv, nullptr);   // Grab the current time
	timecycle = tv.tv_sec + tv.tv_usec * 1e-6; // Compute the current time, in seconds  // Store the old time
  // Grab the current time
	// timecycle = chr::time_point_cast<usec>(chr::steady_clock::now());
	dtime = timecycle - oldtimecycle;
  runningtime += dtime;
}

void Timer::reset() {
	initialize();
}

usec Timer::runningTime() {
	return runningtime;
}

usec Timer::dTime() {
	return dtime;
}
