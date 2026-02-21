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
#include "UsbPassthrough.hpp"

#include <raw-gadget.hpp>

#include <memory>
#include <utility>
#include <vector>

using namespace Chaos;

class UsbPassthrough::Impl {
public:
  class ObserverAdapter : public EndpointObserver {
  public:
    explicit ObserverAdapter(Observer* o) : observer(o) {}

  protected:
    void notification(unsigned char* buffer, int length) override {
      if (observer != nullptr) {
        observer->notification(buffer, length);
      }
    }

  private:
    Observer* observer;
  };

  RawGadgetPassthrough passthrough;
  std::vector<std::unique_ptr<ObserverAdapter>> adapters;
  unsigned char endpoint = 0;
};

UsbPassthrough::UsbPassthrough() : impl(std::make_unique<Impl>()) {}

UsbPassthrough::~UsbPassthrough() = default;

void UsbPassthrough::setEndpoint(unsigned char endpoint) {
  impl->endpoint = endpoint;
  for (const auto& adapter : impl->adapters) {
    adapter->setEndpoint(endpoint);
  }
}

void UsbPassthrough::addObserver(Observer* observer) {
  auto adapter = std::make_unique<Impl::ObserverAdapter>(observer);
  adapter->setEndpoint(impl->endpoint);
  impl->passthrough.addObserver(adapter.get());
  impl->adapters.push_back(std::move(adapter));
}

int UsbPassthrough::initialize() {
  return impl->passthrough.initialize();
}

void UsbPassthrough::start() {
  impl->passthrough.start();
}

void UsbPassthrough::stop() {
  impl->passthrough.stop();
}

bool UsbPassthrough::readyProductVendor() const {
  return impl->passthrough.readyProductVendor();
}

int UsbPassthrough::getVendor() const {
  return impl->passthrough.getVendor();
}

int UsbPassthrough::getProduct() const {
  return impl->passthrough.getProduct();
}
