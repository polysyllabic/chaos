#include <cstdint>
#include <iomanip>
#include <iostream>
#include <unistd.h>

#include <raw-gadget.hpp>

#include "UsbSniffifyHardwareTestUtils.hpp"

namespace {

struct SessionResult {
  bool controllerDetected = false;
  int vendor = 0;
  int product = 0;
  std::uint32_t generation = 0;
};

SessionResult runPassthroughSession(int timeoutMs) {
  SessionResult result;
  RawGadgetPassthrough passthrough;
  if (passthrough.initialize() != 0) {
    return result;
  }

  passthrough.start();
  const bool detected = UsbSniffifyHardwareTest::waitFor([&]() {
    return passthrough.readyProductVendor();
  }, timeoutMs);

  if (detected) {
    result.controllerDetected = true;
    result.vendor = passthrough.getVendor();
    result.product = passthrough.getProduct();
    result.generation = passthrough.getConnectionGeneration();
  }

  passthrough.stop();
  usleep(200000);
  return result;
}

}  // namespace

int main() {
  using namespace UsbSniffifyHardwareTest;

  const RuntimeInfo runtime = inspectRuntime();
  if (!isPi4OrPi5(runtime)) {
    return skip("passthrough smoke test only runs on Raspberry Pi 4/5");
  }
  if (!isRoot()) {
    return skip("run as root to exercise live usb_sniffify passthrough");
  }

  const SessionResult first = runPassthroughSession(5000);
  if (!first.controllerDetected) {
    return skip("no supported controller detected on any USB port for the first passthrough session");
  }

  std::cout << "First session detected controller VID=0x"
            << std::hex << std::setfill('0') << std::setw(4) << first.vendor
            << " PID=0x" << std::setw(4) << first.product << std::dec
            << " generation=" << first.generation << '\n';

  const SessionResult second = runPassthroughSession(5000);

  bool ok = true;
  ok &= check(second.controllerDetected,
              "second passthrough session should reconnect to the controller after the first stop()");
  ok &= check(second.vendor == first.vendor,
              "second passthrough session should preserve controller vendor ID");
  ok &= check(second.product == first.product,
              "second passthrough session should preserve controller product ID");
  ok &= check(first.generation > 0, "first passthrough session should report a non-zero connection generation");
  ok &= check(second.generation > 0, "second passthrough session should report a non-zero connection generation");

  if (!ok) {
    return 1;
  }

  std::cout << "Second session detected controller VID=0x"
            << std::hex << std::setfill('0') << std::setw(4) << second.vendor
            << " PID=0x" << std::setw(4) << second.product << std::dec
            << " generation=" << second.generation << '\n';
  std::cout << "PASS: usb_sniffify passthrough restart smoke test succeeded\n";
  return 0;
}
