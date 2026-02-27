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

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include <plog/Helpers/HexDump.h>
#include <plog/Log.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <raw-gadget.hpp>

#include "Controller.hpp"
#include "ControllerInput.hpp"
#include "ControllerInputTable.hpp"
#include "ControllerState.hpp"
#include "DeviceEvent.hpp"
#include "UsbPassthrough.hpp"

namespace {

enum class OutputMode {
  Translate,
  Raw
};

struct Options {
  std::optional<std::filesystem::path> log_file;
  plog::Severity verbosity = plog::verbose;
  std::optional<double> duration_sec;
  useconds_t loop_sleep_us = 10000;
  unsigned int endpoint = 0x84;
  OutputMode mode = OutputMode::Translate;
  bool include_accel = false;
  int joystick_fuzz = 10;
  bool suppress_repeats_raw = true;
  bool no_data = false;
};

std::atomic<bool> keep_running{true};

void stopHandler(int) {
  keep_running.store(false);
}

void printUsage(const char* program) {
  std::cerr
      << "Usage: " << program << " [options]\n"
      << "Options:\n"
      << "  -v, --verbosity <0-6>      plog verbosity (default: 6/verbose)\n"
      << "  -o, --output <path|->      Log output destination (default: stdout)\n"
      << "      --raw                  Use raw packet mode (default: translate mode)\n"
      << "      --no-data              Suppress ordinary incoming data output in both modes\n"
      << "      --accel                In translate mode, print ACCX/ACCY/ACCZ changes\n"
      << "      --fuzz=<int>           In translate mode, suppress LX/LY/RX/RY when abs(value) < fuzz (default: 10)\n"
      << "  -e, --endpoint <value>     USB endpoint to monitor, hex or decimal (default: 0x84)\n"
      << "  -t, --time <seconds>       Optional run duration before exiting\n"
      << "      --sleep-us <us>        Poll sleep interval in microseconds (default: 10000)\n"
      << "      --print-repeats        In raw mode, print repeated incoming packets (default: off)\n"
      << "  -h, --help                 Show this help\n"
      << "\n"
      << "Notes:\n"
      << "  - Handshake/control traffic is shown by transport-layer verbose logs in both modes.\n"
      << "  - Translate mode prints signal names and values from translated device events.\n"
      << "  - --no-data suppresses ordinary payload lines in both modes.\n"
      << "  - Raw mode prints endpoint packet bytes, suppressing consecutive duplicates by default.\n";
}

plog::Severity clampSeverity(int raw) {
  const int clamped = std::clamp(raw, static_cast<int>(plog::none), static_cast<int>(plog::verbose));
  return static_cast<plog::Severity>(clamped);
}

std::string hexValue(unsigned int value, unsigned int width) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setfill('0') << std::setw(width) << value << std::dec;
  return out.str();
}

std::string formatVidPid(int vendor, int product) {
  std::ostringstream out;
  out << "VID=0x" << std::hex << std::setfill('0') << std::setw(4) << vendor
      << " PID=0x" << std::setw(4) << product << std::dec;
  return out.str();
}

bool parseArgs(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      std::exit(EXIT_SUCCESS);
    }
    if (arg == "-v" || arg == "--verbosity") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        options.verbosity = clampSeverity(std::stoi(argv[++i]));
      } catch (const std::exception&) {
        std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
        return false;
      }
      continue;
    }
    if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      std::filesystem::path output_path = argv[++i];
      if (output_path != "-") {
        options.log_file = output_path;
      } else {
        options.log_file.reset();
      }
      continue;
    }
    if (arg == "--raw") {
      options.mode = OutputMode::Raw;
      continue;
    }
    if (arg == "--no-data") {
      options.no_data = true;
      continue;
    }
    if (arg == "--accel") {
      options.include_accel = true;
      continue;
    }
    if (arg.rfind("--fuzz=", 0) == 0) {
      try {
        const int parsed = std::stoi(arg.substr(7));
        if (parsed < 0) {
          std::cerr << "Invalid value for --fuzz: " << parsed << "\n";
          return false;
        }
        options.joystick_fuzz = parsed;
      } catch (const std::exception&) {
        std::cerr << "Invalid value for --fuzz: " << arg.substr(7) << "\n";
        return false;
      }
      continue;
    }
    if (arg == "-e" || arg == "--endpoint") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        const unsigned long parsed = std::stoul(argv[++i], nullptr, 0);
        if (parsed > std::numeric_limits<unsigned char>::max()) {
          std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
          return false;
        }
        options.endpoint = static_cast<unsigned int>(parsed);
      } catch (const std::exception&) {
        std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
        return false;
      }
      continue;
    }
    if (arg == "-t" || arg == "--time") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        options.duration_sec = std::stod(argv[++i]);
      } catch (const std::exception&) {
        std::cerr << "Invalid value for " << arg << ": " << argv[i] << "\n";
        return false;
      }
      if (!options.duration_sec || *options.duration_sec <= 0.0) {
        std::cerr << "Run duration must be greater than zero.\n";
        return false;
      }
      continue;
    }
    if (arg == "--sleep-us") {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << arg << "\n";
        return false;
      }
      try {
        const long parsed = std::stol(argv[++i]);
        if (parsed <= 0 || parsed > static_cast<long>(std::numeric_limits<useconds_t>::max())) {
          std::cerr << "Invalid value for --sleep-us: " << argv[i] << "\n";
          return false;
        }
        options.loop_sleep_us = static_cast<useconds_t>(parsed);
      } catch (const std::exception&) {
        std::cerr << "Invalid value for --sleep-us: " << argv[i] << "\n";
        return false;
      }
      continue;
    }
    if (arg == "--print-repeats") {
      options.suppress_repeats_raw = false;
      continue;
    }
    std::cerr << "Unknown option: " << arg << "\n";
    return false;
  }
  return true;
}

void initializeLogging(const Options& options) {
  if (options.log_file) {
    const std::filesystem::path parent = options.log_file->parent_path();
    if (!parent.empty()) {
      std::filesystem::create_directories(parent);
    }
    const std::string log_file_path = options.log_file->string();
    plog::init(options.verbosity, log_file_path.c_str());
    return;
  }
  plog::init<plog::TxtFormatter>(options.verbosity, plog::streamStdOut);
}

class PacketObserver : public Chaos::UsbPassthrough::Observer {
public:
  virtual ~PacketObserver() = default;
  virtual const char* modeName() const = 0;
  virtual void onControllerVidPid(int, int) {}
};

class RawPacketObserver final : public PacketObserver {
public:
  RawPacketObserver(bool suppress_repeats, bool no_data)
      : suppress_repeats_(suppress_repeats),
        no_data_(no_data) {}

  const char* modeName() const override {
    return "raw";
  }

  void notification(unsigned char* buffer, int length) override {
    if (no_data_) {
      return;
    }
    if (length < 0) {
      PLOG_WARNING << "Ignoring invalid incoming packet length " << length;
      return;
    }
    std::vector<unsigned char> packet(buffer, buffer + length);
    if (suppress_repeats_) {
      std::lock_guard<std::mutex> guard(lock_);
      if (has_previous_ && packet == previous_packet_) {
        return;
      }
      previous_packet_ = packet;
      has_previous_ = true;
    }

    PLOG_INFO << "incoming endpoint packet len=" << length
              << " data: " << plog::hexdump(packet.data(), packet.size());
  }

private:
  const bool suppress_repeats_;
  const bool no_data_;
  std::mutex lock_;
  std::vector<unsigned char> previous_packet_;
  bool has_previous_ = false;
};

class TranslatePacketObserver final : public PacketObserver {
public:
  TranslatePacketObserver(bool include_accel, int joystick_fuzz, bool no_data)
      : include_accel_(include_accel),
        joystick_fuzz_(joystick_fuzz),
        no_data_(no_data),
        signal_table_(dummy_controller_) {}

  const char* modeName() const override {
    return "translate";
  }

  void onControllerVidPid(int vendor, int product) override {
    std::lock_guard<std::mutex> guard(lock_);
    if (vendor == vendor_ && product == product_) {
      return;
    }
    vendor_ = vendor;
    product_ = product;
    controller_state_.reset(Chaos::ControllerState::factory(vendor, product));
    if (controller_state_ == nullptr) {
      PLOG_WARNING << "Translate mode has no parser for " << formatVidPid(vendor, product)
                   << ". Waiting for a supported controller.";
    }
  }

  void notification(unsigned char* buffer, int length) override {
    if (no_data_) {
      return;
    }
    if (length == 0) {
      PLOG_VERBOSE << "Ignoring empty controller report during USB startup/reconnect.";
      return;
    }
    if (length < static_cast<int>(kExpectedReportLength)) {
      PLOG_WARNING << "Dropping short controller report: expected " << kExpectedReportLength
                   << " bytes, got " << length;
      return;
    }

    std::array<unsigned char, kExpectedReportLength> report{};
    std::memcpy(report.data(), buffer, report.size());

    std::vector<Chaos::DeviceEvent> events;
    {
      std::lock_guard<std::mutex> guard(lock_);
      if (controller_state_ == nullptr) {
        PLOG_VERBOSE << "Dropping controller report because translate parser is not initialized.";
        return;
      }
      controller_state_->getDeviceEvents(report.data(), static_cast<int>(report.size()), events);
    }

    for (const Chaos::DeviceEvent& event : events) {
      std::shared_ptr<Chaos::ControllerInput> input = signal_table_.getInput(event);
      if (input) {
        if (!shouldPrint(*input, event.value)) {
          continue;
        }
        PLOG_INFO << input->getName() << "=" << event.value;
      } else {
        PLOG_WARNING << "Unknown signal (type=" << static_cast<int>(event.type)
                     << ", id=" << static_cast<int>(event.id) << ")=" << event.value;
      }
    }
  }

private:
  bool shouldPrint(Chaos::ControllerInput& input, short value) const {
    switch (input.getSignal()) {
      case Chaos::ControllerSignal::ACCX:
      case Chaos::ControllerSignal::ACCY:
      case Chaos::ControllerSignal::ACCZ:
        return include_accel_;
      case Chaos::ControllerSignal::LX:
      case Chaos::ControllerSignal::LY:
      case Chaos::ControllerSignal::RX:
      case Chaos::ControllerSignal::RY:
        return std::abs(static_cast<int>(value)) >= joystick_fuzz_;
      default:
        return true;
    }
  }

  static constexpr std::size_t kExpectedReportLength = 64;

  const bool include_accel_;
  const int joystick_fuzz_;
  const bool no_data_;
  std::mutex lock_;
  int vendor_ = -1;
  int product_ = -1;
  Chaos::Controller dummy_controller_;
  Chaos::ControllerInputTable signal_table_;
  std::unique_ptr<Chaos::ControllerState> controller_state_;
};

}  // namespace

int main(int argc, char** argv) {
  Options options;
  if (!parseArgs(argc, argv, options)) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  try {
    initializeLogging(options);
  } catch (const std::exception& err) {
    std::cerr << "Failed to initialize logging: " << err.what() << "\n";
    return EXIT_FAILURE;
  }

  RawGadgetPassthrough::requireUsbPermissionsOrExit();
  std::signal(SIGINT, stopHandler);
  std::signal(SIGTERM, stopHandler);

  Chaos::UsbPassthrough passthrough;
  std::unique_ptr<PacketObserver> packet_observer;
  if (options.mode == OutputMode::Raw) {
    packet_observer = std::make_unique<RawPacketObserver>(options.suppress_repeats_raw,
                                                          options.no_data);
  } else {
    packet_observer = std::make_unique<TranslatePacketObserver>(options.include_accel,
                                                                options.joystick_fuzz,
                                                                options.no_data);
  }

  passthrough.setEndpoint(static_cast<unsigned char>(options.endpoint));
  passthrough.addObserver(packet_observer.get());

  if (passthrough.initialize() != 0) {
    PLOG_ERROR << "UsbPassthrough initialization failed.";
    return EXIT_FAILURE;
  }
  passthrough.start();

  PLOG_INFO << "Monitoring endpoint " << hexValue(options.endpoint, 2)
            << " in " << packet_observer->modeName() << " mode.";
  if (options.mode == OutputMode::Raw) {
    PLOG_INFO << (options.suppress_repeats_raw
                    ? "Raw mode: repeated identical packets are suppressed."
                    : "Raw mode: repeated packets will be printed.");
    if (options.include_accel) {
      PLOG_WARNING << "--accel applies only to translate mode and is ignored in raw mode.";
    }
    if (options.joystick_fuzz != 10) {
      PLOG_WARNING << "--fuzz applies only to translate mode and is ignored in raw mode.";
    }
  } else if (!options.suppress_repeats_raw) {
    PLOG_WARNING << "--print-repeats only applies to --raw mode and is ignored.";
  } else {
    PLOG_INFO << "Translate mode filters: accel="
              << (options.include_accel ? "on" : "off")
              << ", joystick_fuzz=" << options.joystick_fuzz;
  }
  if (options.no_data) {
    PLOG_INFO << "Data output is disabled (--no-data). Handshake/control and transport logs remain.";
  }
  if (options.verbosity < plog::verbose) {
    PLOG_WARNING << "Handshake/control traffic logs require verbosity 6.";
  }

  const auto start_time = std::chrono::steady_clock::now();
  int last_vendor = -1;
  int last_product = -1;

  while (keep_running.load()) {
    if (options.duration_sec) {
      const double elapsed = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - start_time).count();
      if (elapsed >= *options.duration_sec) {
        break;
      }
    }

    if (passthrough.readyProductVendor()) {
      const int vendor = passthrough.getVendor();
      const int product = passthrough.getProduct();
      packet_observer->onControllerVidPid(vendor, product);
      if (vendor != last_vendor || product != last_product) {
        PLOG_INFO << "Controller detected: " << formatVidPid(vendor, product);
        last_vendor = vendor;
        last_product = product;
      }
    }

    usleep(options.loop_sleep_us);
  }

  passthrough.stop();
  PLOG_INFO << "Stopped gamepad_test monitor.";
  return EXIT_SUCCESS;
}
