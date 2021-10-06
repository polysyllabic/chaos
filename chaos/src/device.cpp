/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include "device.hpp"

#include <iostream>
#include <cstring>
#include <plog/Log.h>

using namespace Chaos;

void Device::init(const char* filename, int bytesPerFrame) {
  file = fopen( filename, "r" );

  if( file == NULL ) {
    PLOG_ERROR << "Couldn't open " << filename << std::endl;
  }
	
  this->bytesPerFrame = bytesPerFrame;
  this->buffer = new char[bytesPerFrame];
}

Device::~Device() {
  stop();
  WaitForInternalThreadToExit();
  fclose( file );
	
  if( buffer != NULL) {
    delete [] buffer;
  }
}

void Device::addObserver(DeviceObserver* observer) {
  this->observer = observer;
}

void Device::doAction() {
  PLOG_DEBUG << "Read: ";
  for (int i = 0; i < bytesPerFrame; i++) {
    buffer[i] = fgetc( file );
    PLOG_DEBUG << (int)buffer[i] << " ";
  }
  PLOG_DEBUG << std::endl;
	
  DeviceEvent event;
	
  interpret( buffer, &event );
	
  if (observer != NULL && event.time != -1) {
    observer->newDeviceEvent( &event );
  }
}
