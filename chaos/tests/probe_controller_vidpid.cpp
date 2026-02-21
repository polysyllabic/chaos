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
#include <iomanip>
#include <iostream>
#include <unistd.h>

#include <raw-gadget.hpp>

namespace {
  bool probeInterceptedControllerVidPid(int& vendor, int& product) {
    RawGadgetPassthrough passthrough;
    if (passthrough.initialize() != 0) {
      return false;
    }

    passthrough.start();
    for (int i = 0; i < 500 && !passthrough.readyProductVendor(); i++) {
      usleep(10000);
    }
    passthrough.stop();

    if (!passthrough.readyProductVendor()) {
      return false;
    }

    vendor = passthrough.getVendor();
    product = passthrough.getProduct();
    return true;
  }
}

int main() {
  int vendor = 0;
  int product = 0;
  if (!probeInterceptedControllerVidPid(vendor, product)) {
    std::cerr << "Failed to detect controller VID/PID on intercepted USB port." << std::endl;
    return 1;
  }

  std::cout << "Detected controller VID=0x"
            << std::hex << std::setfill('0') << std::setw(4) << vendor
            << " PID=0x" << std::setw(4) << product << std::dec << std::endl;
  return 0;
}
