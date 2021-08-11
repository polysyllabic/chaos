/*----------------------------------------------------------------------------
* This file is part of Twitch Controls Chaos (TCC).
* Copyright 2021 blegas78
*
* TCC is free software: you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later
* version.
*
* TCC is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along
* with TCC.  If not, see <https://www.gnu.org/licenses/>.
*---------------------------------------------------------------------------*/
#ifndef CONTROLLER_GPIO_HPP
#define CONTROLLER_GPIO_HPP

#include <cstdio>
#include <map>

#include "controller.hpp"
#include "device.h"	// Joystick, Mouse

namespace Chaos {

  class ControllerGpio : public Controller,  public DeviceObserver {
	
  private:
    char    *buf;

    bool applyHardware(const DeviceEvent* event);
	
    // Give me an event, and I'll tell you what to do with it:
    std::map<DeviceEvent, FunctionInfo> eventToFunction;
	
    Joystick joystick;
    Mouse mouse;
	
    //FILE* spooferFile = NULL;
    //int spooferFd = 0;
    //fd_set rfds;
    //void writeToSpoofFile( std::map<int,short>& controllerState );
	
  protected:
    void newDeviceEvent(const DeviceEvent*);	// Override from DeviceObserver
	
  public:
    ControllerGpio();
    ~ControllerGpio();
	
    void initialize( const char* joystickFileName );
    //	void setSpoofFile( const char* filename );	
    //	void applyEvent(const DeviceEvent* event);
    //	void addInjector(ControllerInjector* injector);	
  };

};

#endif
