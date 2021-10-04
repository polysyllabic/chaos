/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#ifndef CHAOS_UHID_HPP
#define CHAOS_UHID_HPP

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <linux/uhid.h>

#include <mogi/thread.h>
#include "controllerState.hpp"
#include "deviceTypes.hpp"

namespace Chaos {

  class ChaosUhid : public Mogi::Thread {
  private:
    int fd;
    const char *path;
    struct pollfd pfds[2];
    int ret;
    struct termios state;
	
    ControllerState* stateToPoll;
	
    void doAction();
	
public:
    ChaosUhid(ControllerState* state);
    ~ChaosUhid();
	
    void sendUpdate(void* data);
  };

};

#endif
