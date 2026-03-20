#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unistd.h>

namespace UsbSniffifyHardwareTest {

constexpr int kSkipExitCode = 77;

struct RuntimeInfo {
  std::string model;
  std::string runtimePlatform;
  std::string compiledPlatform;
  std::string compiledDefaultUdc;
  std::string compiledDefaultDriver;
  std::vector<std::string> udcCandidates;
  bool rawGadgetModuleLoaded = false;
  bool rawGadgetDevicePresent = false;
  bool rawGadgetDeviceIsChar = false;
};

RuntimeInfo inspectRuntime();

bool check(bool condition, const std::string& message);
int skip(const std::string& reason);

bool isPi4OrPi5(const RuntimeInfo& info);
bool isRoot();

std::string chooseUdcDevice(const RuntimeInfo& info);
std::string chooseUdcDriver(const RuntimeInfo& info, const std::string& device);

template <typename Predicate>
bool waitFor(Predicate&& predicate, int timeoutMs, int pollIntervalUs = 10000) {
  const int attempts = (timeoutMs * 1000) / pollIntervalUs;
  for (int i = 0; i < attempts; ++i) {
    if (predicate()) {
      return true;
    }
    usleep(pollIntervalUs);
  }
  return predicate();
}

}  // namespace UsbSniffifyHardwareTest
