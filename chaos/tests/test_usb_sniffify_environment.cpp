#include <iostream>

#include "UsbSniffifyHardwareTestUtils.hpp"

int main() {
  using namespace UsbSniffifyHardwareTest;

  const RuntimeInfo runtime = inspectRuntime();
  if (!isPi4OrPi5(runtime)) {
    return skip("usb_sniffify hardware tests only run on Raspberry Pi 4/5. Detected model='" +
                runtime.model + "'");
  }

  std::cout << "Runtime model: " << runtime.model << '\n';
  std::cout << "Runtime platform: " << runtime.runtimePlatform << '\n';
  std::cout << "Compiled platform: " << runtime.compiledPlatform << '\n';
  std::cout << "Compiled default UDC: " << runtime.compiledDefaultUdc << '\n';
  std::cout << "Compiled default driver: " << runtime.compiledDefaultDriver << '\n';
  std::cout << "Detected UDC count: " << runtime.udcCandidates.size() << '\n';

  bool ok = true;
  ok &= check(runtime.compiledPlatform == "pi4" || runtime.compiledPlatform == "pi5",
              "compiled target platform should be pi4 or pi5 for Pi-only usb_sniffify hardware tests");
  ok &= check(runtime.rawGadgetModuleLoaded,
              "raw_gadget kernel module should be loaded before usb_sniffify hardware tests run");
  ok &= check(runtime.rawGadgetDevicePresent,
              "/dev/raw-gadget should exist before usb_sniffify hardware tests run");
  ok &= check(runtime.rawGadgetDeviceIsChar,
              "/dev/raw-gadget should be a character device");
  ok &= check(!runtime.udcCandidates.empty(),
              "at least one UDC should be available under /sys/class/udc");

  const std::string selectedUdc = chooseUdcDevice(runtime);
  ok &= check(!selectedUdc.empty(), "should resolve a usable UDC candidate");
  if (!selectedUdc.empty()) {
    std::cout << "Selected UDC candidate: " << selectedUdc << '\n';
  }

  if (!ok) {
    return 1;
  }

  std::cout << "PASS: usb_sniffify environment looks valid for Pi hardware tests\n";
  return 0;
}
