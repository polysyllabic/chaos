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
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <map>
#include <cstdio>
#include <deque>
#include <array>

#include "device.h"	// Joystick, Mouse
#include "raw-gadget.hpp"
#include "controller-state.h"
#include "chaos-uhid.h"

#define PWM_RANGE (11)

namespace Chaos {

// Dummy functions to test on a non-pi
#ifndef __RASPBERRY_PI__
  void wiringPiSetup() {}
  void digitalWrite(int,int) {};
  void pinMode(int, int) {};
  void softPwmCreate(int,int,int) {};
  void softPwmWrite (int,int) {};
#define HIGH (1)
#define LOW (0)
#define OUTPUT (1)
#else
//	#include <wiringPi.h>
//	#include <softPwm.h>
#endif

// Functions that take an input and aply it to the raspberry pi pins
void functionAxis(int* pins, int value, double calibration);
void functionButton(int* pins, int value, double calibration);
void functionDpad(int* pins, int value, double calibration);

typedef struct FunctionInfo {
  void (*function)(int*, int, double);
  int pins[2];
  double calibration;
  unsigned char outputType;
} FunctionInfo;

class ControllerInjector  {
public:
  // sniff the input and pass/modify it on way to the output
  virtual bool sniffify(const DeviceEvent* input, DeviceEvent* output) = 0;
};

class Controller {
protected:
  std::deque<DeviceEvent> deviceEventQueue;
  //virtual void writeToSpoofFile( std::map<int,short>& controllerState ) = 0;
	
  virtual bool applyHardware(const DeviceEvent* event) = 0;
	
  void storeState(const DeviceEvent* event);
	
  void handleNewDeviceEvent(const DeviceEvent* event);
	
  // Database for tracking current button states:
  // Input: ((int)event->type<<8) + (int)event->id ] = event->value;
  // Output: state of that type/id combo
  //std::map<int,short> controllerState;
  short controllerState[1024];
	
  ControllerInjector* dualShockInjector = NULL;
	
public:
  Controller();
  int getState(int id, ButtonType type);
	
  void applyEvent(const DeviceEvent* event);
  void addInjector(ControllerInjector* injector);
	
};

  class ControllerRaw : public Controller, public EndpointObserver, public Mogi::Thread {
  private:
    ControllerState* mControllerState;
    std::deque<std::array<unsigned char,64>> bufferQueue;
	
    ChaosUhid* chaosHid;
	
    bool applyHardware(const DeviceEvent* event);
	
    // Main purpose of this Mogi::Thread is to handle DeviceEvent queue
    void doAction();
	
    void notification(unsigned char* buffer, int length); // overloaded from EndpointObserver

    //FILE* spooferFile = NULL;
  public:
    RawGadgetPassthrough mRawGadgetPassthrough;
	
    void initialize( );	
  };

};

#endif
