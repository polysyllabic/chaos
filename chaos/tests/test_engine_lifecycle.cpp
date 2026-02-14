#include <atomic>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include <toml++/toml.h>

#include <ChaosEngine.hpp>
#include <Modifier.hpp>
#include <Controller.hpp>
#include <DeviceEvent.hpp>
#include <signals.hpp>

using namespace Chaos;

class TestController : public Controller {
public:
  void inject(const DeviceEvent& event) { handleNewDeviceEvent(event); }
};

class RaceModifier : public Modifier::Registrar<RaceModifier> {
public:
  static const std::string mod_type;
  static std::atomic<bool> in_update;
  static std::atomic<bool> block_updates;
  static std::atomic<bool> finish_called;
  static std::atomic<bool> overlap;

  RaceModifier(toml::table& config, EngineInterface* e) { initialize(config, e); }

  const std::string& getModType() override { return mod_type; }

  void update() override {
    in_update.store(true);
    while (block_updates.load()) {
      usleep(1000);
    }
    in_update.store(false);
  }

  void finish() override {
    if (in_update.load()) {
      overlap.store(true);
    }
    finish_called.store(true);
  }

  static void reset() {
    in_update.store(false);
    block_updates.store(false);
    finish_called.store(false);
    overlap.store(false);
  }
};

const std::string RaceModifier::mod_type = "test_race";
std::atomic<bool> RaceModifier::in_update{false};
std::atomic<bool> RaceModifier::block_updates{false};
std::atomic<bool> RaceModifier::finish_called{false};
std::atomic<bool> RaceModifier::overlap{false};

static bool check(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

template <typename Predicate>
static bool waitFor(Predicate&& pred, int timeout_ms = 1500) {
  for (int i = 0; i < timeout_ms; ++i) {
    if (pred()) {
      return true;
    }
    usleep(1000);
  }
  return false;
}

static std::string writeConfigFile() {
  static const char* kConfigText = R"(
config_file_ver = "1.0"
chaos_toml = "main"
game = "Engine Lifecycle Test"

[mod_defaults]
active_modifiers = 3
time_per_modifier = 30.0

[controller]
button_press_time = 0.01
button_release_time = 0.01
touchpad_inactive_delay = 0.04
touchpad_velocity = false
touchpad_scale_x = 1.0
touchpad_scale_y = 1.0
touchpad_skew = 0

[menu]
use_menu = false

[[command]]
name = "MOVE_X"
binding = "LX"

[[modifier]]
name = "RACE"
type = "test_race"
)";

  char path_template[] = "/tmp/chaos_engine_lifecycle_XXXXXX.toml";
  int fd = mkstemps(path_template, 5);
  if (fd < 0) {
    throw std::runtime_error("Failed to create temporary config file");
  }
  ssize_t wrote = write(fd, kConfigText, std::strlen(kConfigText));
  close(fd);
  if (wrote < 0) {
    throw std::runtime_error("Failed to write temporary config file");
  }
  return std::string(path_template);
}

static size_t activeCount(ChaosEngine& engine) {
  engine.lock();
  size_t count = engine.getActiveMods().size();
  engine.unlock();
  return count;
}

static void unpauseEngine(TestController& controller) {
  controller.inject({0, 1, TYPE_BUTTON, BUTTON_SHARE});
  controller.inject({0, 0, TYPE_BUTTON, BUTTON_SHARE});
}

static bool testDuplicateActivationDoesNotStack() {
  bool ok = true;
  RaceModifier::reset();

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  engine.newCommand("{\"winner\":\"RACE\"}");
  engine.newCommand("{\"winner\":\"RACE\"}");
  unpauseEngine(controller);

  ok &= check(waitFor([&]() { return activeCount(engine) == 1; }),
              "duplicate winner commands should produce one active instance");
  usleep(30000);
  ok &= check(activeCount(engine) == 1,
              "active modifier count should remain 1 after duplicate activation");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testRemoveDeferredUntilUpdateCompletes() {
  bool ok = true;
  RaceModifier::reset();

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);

  RaceModifier::block_updates.store(true);
  engine.newCommand("{\"winner\":\"RACE\"}");
  ok &= check(waitFor([]() { return RaceModifier::in_update.load(); }),
              "modifier update should start and block");

  engine.newCommand("{\"remove\":\"RACE\"}");
  usleep(20000);
  ok &= check(!RaceModifier::finish_called.load(),
              "finish should not run while update is still blocked");

  RaceModifier::block_updates.store(false);
  ok &= check(waitFor([]() { return RaceModifier::finish_called.load(); }),
              "finish should run after blocked update exits");
  ok &= check(!RaceModifier::overlap.load(),
              "finish must not overlap concurrent update execution");
  ok &= check(waitFor([&]() { return activeCount(engine) == 0; }),
              "removed modifier should leave active list");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

int main() {
  bool ok = true;
  ok &= testDuplicateActivationDoesNotStack();
  ok &= testRemoveDeferredUntilUpdateCompletes();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: engine lifecycle regression tests\n";
  return 0;
}
