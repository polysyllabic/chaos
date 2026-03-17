#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <raw-helper.h>

#include "UsbSniffifyHardwareTestUtils.hpp"

int main() {
  using namespace UsbSniffifyHardwareTest;

  constexpr int kBusyReleaseTimeoutMs = 10000;
  constexpr useconds_t kBusyRetrySleepUs = 250000;

  const RuntimeInfo runtime = inspectRuntime();
  if (!isPi4OrPi5(runtime)) {
    return skip("raw-gadget session smoke test only runs on Raspberry Pi 4/5");
  }
  if (!isRoot()) {
    return skip("run as root to open /dev/raw-gadget and bind a live UDC");
  }

  const std::string udcDevice = chooseUdcDevice(runtime);
  const std::string udcDriver = chooseUdcDriver(runtime, udcDevice);

  bool ok = true;
  ok &= check(!udcDevice.empty(), "should resolve a UDC device before opening raw-gadget");
  ok &= check(!udcDriver.empty(), "should resolve a UDC driver before opening raw-gadget");
  if (!ok) {
    return 1;
  }

  std::cout << "Opening /dev/raw-gadget for UDC '" << udcDevice
            << "' with driver '" << udcDriver << "'\n";

  const int maxAttempts = kBusyReleaseTimeoutMs / static_cast<int>(kBusyRetrySleepUs / 1000);
  int lastErrno = 0;
  bool runSucceeded = false;
  for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
    int fd = usb_raw_open();
    ok &= check(fd >= 0, "usb_raw_open() should succeed");
    if (!ok) {
      return 1;
    }

    if (usb_raw_init(fd, USB_SPEED_HIGH, udcDriver.c_str(), udcDevice.c_str()) < 0) {
      lastErrno = errno;
      close(fd);
      std::cerr << "FAIL: usb_raw_init() failed for UDC '" << udcDevice
                << "': " << std::strerror(lastErrno) << '\n';
      return 1;
    }

    if (usb_raw_run(fd) == 0) {
      close(fd);
      usleep(100000);
      runSucceeded = true;
      break;
    }

    lastErrno = errno;
    close(fd);
    if (lastErrno != EBUSY) {
      std::cerr << "FAIL: usb_raw_run() failed: " << std::strerror(lastErrno) << '\n';
      return 1;
    }

    std::cout << "usb_raw_run() still busy after stop; waiting for prior session release"
              << " (attempt " << attempt << "/" << maxAttempts << ")\n";
    usleep(kBusyRetrySleepUs);
  }

  if (!runSucceeded) {
    std::cerr << "FAIL: usb_raw_run() remained busy for "
              << (kBusyReleaseTimeoutMs / 1000)
              << " seconds after stopping chaos.service.\n";
    return 1;
  }

  std::cout << "PASS: raw-gadget init/run/close succeeded on UDC '" << udcDevice << "'\n";
  return 0;
}
