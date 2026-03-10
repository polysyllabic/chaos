/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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

#include <cstdint>
#include <memory>

namespace Chaos {

  class UsbPassthrough {
  public:
    class Observer {
    public:
      virtual ~Observer() = default;

      /**
       * \brief Receive a passthrough input report from the active controller.
       *
       * \param buffer Raw report bytes.
       * \param length Report length in bytes.
       */
      virtual void notification(unsigned char* buffer, int length) = 0;
    };

    /**
     * \brief Construct USB passthrough transport state.
     */
    UsbPassthrough();

    /**
     * \brief Tear down passthrough transport resources.
     */
    ~UsbPassthrough();

    UsbPassthrough(const UsbPassthrough&) = delete;
    UsbPassthrough& operator=(const UsbPassthrough&) = delete;
    UsbPassthrough(UsbPassthrough&&) = delete;
    UsbPassthrough& operator=(UsbPassthrough&&) = delete;

    /**
     * \brief Select the endpoint used for reading controller reports.
     *
     * \param endpoint Endpoint identifier.
     */
    void setEndpoint(unsigned char endpoint);

    /**
     * \brief Register an observer for decoded input reports.
     *
     * \param observer Observer to notify.
     */
    void addObserver(Observer* observer);

    /**
     * \brief Initialize passthrough transport resources.
     *
     * \return Zero on success, non-zero on failure.
     */
    int initialize();

    /**
     * \brief Start passthrough processing.
     */
    void start();

    /**
     * \brief Stop passthrough processing.
     */
    void stop();

    /**
     * \brief Check whether vendor/product IDs are available.
     */
    bool readyProductVendor() const;

    /**
     * \brief Get the detected USB vendor ID.
     */
    int getVendor() const;

    /**
     * \brief Get the detected USB product ID.
     */
    int getProduct() const;

    /**
     * \brief Get the transport reconnection generation counter.
     */
    std::uint32_t getConnectionGeneration() const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
  };

};
