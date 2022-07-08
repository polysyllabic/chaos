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
 * TO DO: Replace this with something from the C++ <random> library
*/
#include <math.h>
#include <time.h>
#include "random.hpp"

using namespace Chaos;

Random::Random() {
	srand(time(NULL));
}

double Random::uniform(double min, double max) {
	return (max - min) * ((double) rand() / (double) RAND_MAX) + min;
}

double Random::normal(double mean, double variance) {
	// from http://en.wikipedia.org/wiki/Box–Muller_transform
	static bool haveSpare = false;
	static double rand1, rand2;

	if (haveSpare) {
		haveSpare = false;
		return sqrt(variance * rand1) * sin(rand2) + mean;
	}

	haveSpare = true;

	rand1 = rand() / ((double) RAND_MAX);
	if (rand1 < 1e-100)
		rand1 = 1e-100;
	rand1 = -2 * log(rand1);
	rand2 = (rand() / ((double) RAND_MAX)) * 2 * M_PI;

	return sqrt(variance * rand1) * cos(rand2) + mean;
}
