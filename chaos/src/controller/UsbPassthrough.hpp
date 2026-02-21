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

#include <memory>

namespace Chaos {

  class UsbPassthrough {
  public:
    class Observer {
    public:
      virtual ~Observer() = default;
      virtual void notification(unsigned char* buffer, int length) = 0;
    };

    UsbPassthrough();
    ~UsbPassthrough();

    UsbPassthrough(const UsbPassthrough&) = delete;
    UsbPassthrough& operator=(const UsbPassthrough&) = delete;
    UsbPassthrough(UsbPassthrough&&) = delete;
    UsbPassthrough& operator=(UsbPassthrough&&) = delete;

    void setEndpoint(unsigned char endpoint);
    void addObserver(Observer* observer);

    void initialize();
    void start();
    void stop();

    bool readyProductVendor() const;
    int getVendor() const;
    int getProduct() const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
  };

};
