/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
 */
#pragma once
#define CHAOS_VERSION_MAJOR 2
#define CHAOS_VERSION_MINOR 0
#define CHAOS_VERSION "2.0.0-alpha.6"

// Comment out this line for testing on a different platform.
// TODO: Enable keyboard emulation of controller signals when this is false
#define RASPBERRY_PI
/* #undef USE_DUALSENSE */

#define SEC_TO_MICROSEC 1000000.0

// These values probably should be encapsulated in a class somewhere, at least if they can ever
// change between controllers. For now we leave them as global defines.
#define JOYSTICK_MIN (-128)
#define JOYSTICK_MAX (127)
