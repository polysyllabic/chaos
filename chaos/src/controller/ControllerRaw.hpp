/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
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
 */
#pragma once
#include <array>

#include <thread.hpp>

#include "Controller.hpp"
#include "UsbPassthrough.hpp"

namespace Chaos {

  /**
   * \brief A concrete controller class that gets the data using the USB interface through the
   * raw-gadget driver.
   * 
   * Note that the old code had a ControllerGpio class for running the controller through a custom
   * GPIO cable, but I haven't ported that over, since it seems like an obsolete interface now that
   * USB interception is available.
   */
  class ControllerRaw : public Controller, public UsbPassthrough::Observer, public Thread {
  private:
	  ControllerState* mControllerState = nullptr;

  	// bool applyHardware(const DeviceEvent& event);
	
	  // Handles the DeviceEvent queue 
	  void doAction();
	
	  void notification(unsigned char* buffer, int length) override;

	  UsbPassthrough mUsbPassthrough;

  public:
    ControllerRaw();
    ~ControllerRaw() override;

	  void initialize();
		
  };
};
