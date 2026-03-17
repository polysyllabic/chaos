#include "UsbSniffifyHardwareTestUtils.hpp"

#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>

#include <config.hpp>

namespace UsbSniffifyHardwareTest {
namespace {

std::string trimWhitespace(std::string value) {
  auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
    return std::isspace(c) != 0;
  });
  auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
    return std::isspace(c) != 0;
  }).base();

  if (first >= last) {
    return "";
  }
  return std::string(first, last);
}

std::vector<std::string> listDirectoryEntries(const std::string& path) {
  std::vector<std::string> entries;
  DIR* dir = opendir(path.c_str());
  if (dir == nullptr) {
    return entries;
  }

  while (dirent* entry = readdir(dir)) {
    const std::string name = entry->d_name;
    if (name == "." || name == "..") {
      continue;
    }
    entries.push_back(name);
  }

  closedir(dir);
  std::sort(entries.begin(), entries.end());
  return entries;
}

std::string detectRuntimePlatform(const std::string& model) {
  if (model.find("Raspberry Pi 5") != std::string::npos ||
      model.find("Raspberry Pi 500") != std::string::npos ||
      model.find("Compute Module 5") != std::string::npos) {
    return "pi5";
  }
  if (model.find("Raspberry Pi 4") != std::string::npos ||
      model.find("Raspberry Pi 400") != std::string::npos ||
      model.find("Compute Module 4") != std::string::npos) {
    return "pi4";
  }
  if (model.find("Raspberry Pi") != std::string::npos) {
    return "raspberry-pi";
  }
  return "unknown";
}

bool vectorContains(const std::vector<std::string>& values, const std::string& needle) {
  return std::find(values.begin(), values.end(), needle) != values.end();
}

}  // namespace

RuntimeInfo inspectRuntime() {
  RuntimeInfo info;
  info.compiledPlatform = trimWhitespace(CHAOS_TARGET_PLATFORM);
  info.compiledDefaultUdc = trimWhitespace(CHAOS_DEFAULT_UDC_DEVICE);
  info.compiledDefaultDriver = trimWhitespace(CHAOS_DEFAULT_UDC_DRIVER);
  info.udcCandidates = listDirectoryEntries("/sys/class/udc");
  info.rawGadgetModuleLoaded = access("/sys/module/raw_gadget", F_OK) == 0;
  info.rawGadgetDevicePresent = access("/dev/raw-gadget", F_OK) == 0;

  struct stat st {};
  if (stat("/dev/raw-gadget", &st) == 0) {
    info.rawGadgetDeviceIsChar = S_ISCHR(st.st_mode);
  }

  std::ifstream modelFile("/proc/device-tree/model", std::ios::binary);
  info.model.assign(std::istreambuf_iterator<char>(modelFile), std::istreambuf_iterator<char>());
  info.model.erase(std::remove(info.model.begin(), info.model.end(), '\0'), info.model.end());
  info.model = trimWhitespace(info.model);
  info.runtimePlatform = detectRuntimePlatform(info.model);

  return info;
}

bool check(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

int skip(const std::string& reason) {
  std::cout << "SKIP: " << reason << '\n';
  return kSkipExitCode;
}

bool isPi4OrPi5(const RuntimeInfo& info) {
  return info.runtimePlatform == "pi4" || info.runtimePlatform == "pi5";
}

bool isRoot() {
  return geteuid() == 0;
}

std::string chooseUdcDevice(const RuntimeInfo& info) {
  if (!info.compiledDefaultUdc.empty() && vectorContains(info.udcCandidates, info.compiledDefaultUdc)) {
    return info.compiledDefaultUdc;
  }
  if (!info.udcCandidates.empty()) {
    return info.udcCandidates.front();
  }
  if (!info.compiledDefaultUdc.empty()) {
    return info.compiledDefaultUdc;
  }
  return "";
}

std::string chooseUdcDriver(const RuntimeInfo& info, const std::string& device) {
  if (!info.compiledDefaultDriver.empty()) {
    return info.compiledDefaultDriver;
  }
  return device;
}

}  // namespace UsbSniffifyHardwareTest
