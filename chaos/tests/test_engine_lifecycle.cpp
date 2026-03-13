#include <atomic>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include <toml++/toml.h>
#include <zmq.hpp>

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

struct IpcEndpoint {
  std::string endpoint;
  std::string path;
};

class AckResponder {
public:
  explicit AckResponder(const std::string& endpoint)
      : context(1), socket(context, zmq::socket_type::rep) {
    socket.set(zmq::sockopt::linger, 0);
    socket.set(zmq::sockopt::rcvtimeo, 50);
    socket.bind(endpoint);
    worker = std::thread([this]() { run(); });
  }

  ~AckResponder() { stop(); }

  void stop() {
    if (!running.exchange(false)) {
      return;
    }
    if (worker.joinable()) {
      worker.join();
    }
    socket.close();
    context.close();
  }

private:
  void run() {
    while (running.load()) {
      zmq::message_t message;
      auto received = socket.recv(message, zmq::recv_flags::none);
      if (!received) {
        continue;
      }
      auto sent = socket.send(zmq::buffer("ACK", 3), zmq::send_flags::none);
      (void) sent;
    }
  }

  std::atomic<bool> running{true};
  zmq::context_t context;
  zmq::socket_t socket;
  std::thread worker;
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

[[command]]
name = "MOVE_Y"
binding = "LY"

[[command]]
name = "CAMERA_Y"
binding = "RY"

[[command]]
name = "FIRE"
binding = "R2"

[[modifier]]
name = "RACE"
type = "test_race"

[[modifier]]
name = "Inverted"
type = "scaling"
applies_to = [ "CAMERA_Y" ]
amplitude = -1

[[modifier]]
name = "Moonwalk"
type = "scaling"
applies_to = [ "MOVE_Y" ]
amplitude = -1

[[modifier]]
name = "SEQ_AXIS_CLIP"
type = "sequence"
begin_sequence = [ { event = "hold", command = "MOVE_Y", value = 128 } ]
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

static IpcEndpoint makeIpcEndpoint() {
  char path_template[] = "/tmp/chaos_iface_XXXXXX.sock";
  int fd = mkstemps(path_template, 5);
  if (fd < 0) {
    throw std::runtime_error("Failed to create temporary IPC endpoint path");
  }
  close(fd);
  std::remove(path_template);
  IpcEndpoint result;
  result.path = path_template;
  result.endpoint = "ipc://" + result.path;
  return result;
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

static bool testFirstUnpauseKeepsHybridTriggersReleased() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();

  ok &= check(engine.getState(AXIS_L2, TYPE_AXIS) == JOYSTICK_MIN,
              "L2 axis should start released before first unpause");
  ok &= check(engine.getState(AXIS_R2, TYPE_AXIS) == JOYSTICK_MIN,
              "R2 axis should start released before first unpause");

  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should unpause on SHARE press/release");

  ok &= check(engine.getState(BUTTON_L2, TYPE_BUTTON) == 0,
              "L2 button should remain released after first unpause");
  ok &= check(engine.getState(BUTTON_R2, TYPE_BUTTON) == 0,
              "R2 button should remain released after first unpause");
  ok &= check(engine.getState(AXIS_L2, TYPE_AXIS) == JOYSTICK_MIN,
              "L2 axis should remain released after first unpause without trigger input");
  ok &= check(engine.getState(AXIS_R2, TYPE_AXIS) == JOYSTICK_MIN,
              "R2 axis should remain released after first unpause without trigger input");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testReleaseOnlyShareUnpausesWhenReady() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  controller.inject({0, 0, TYPE_BUTTON, BUTTON_SHARE});
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "release-only SHARE should unpause when engine is ready");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testInterfaceReconnectResumePaths() {
  bool ok = true;

  TestController controller;
  IpcEndpoint listener = makeIpcEndpoint();
  IpcEndpoint talker = makeIpcEndpoint();
  AckResponder responder(talker.endpoint);
  ChaosEngine engine(controller, listener.endpoint, talker.endpoint, true);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should unpause before simulated interface timeout");
  // Allow interface ACK traffic from the initial unpause to settle before forcing unhealthy.
  usleep(50000);

  engine.setInterfaceHealthForTest(false);
  ok &= check(waitFor([&]() { return engine.isPaused(); }),
              "engine should pause when interface becomes unhealthy");

  engine.setInterfaceHealthForTest(true);
  usleep(30000);
  ok &= check(engine.isPaused(),
              "engine should remain paused after interface recovers until manual resume");

  controller.inject({0, 0, TYPE_BUTTON, BUTTON_SHARE});
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "release-only SHARE should resume after interface health recovers");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  responder.stop();
  std::remove(config_path.c_str());
  std::remove(listener.path.c_str());
  std::remove(talker.path.c_str());
  return ok;
}

static bool testInterfaceReconnectQueuesResumeIntentDuringOutage() {
  bool ok = true;

  TestController controller;
  IpcEndpoint listener = makeIpcEndpoint();
  IpcEndpoint talker = makeIpcEndpoint();
  AckResponder responder(talker.endpoint);
  ChaosEngine engine(controller, listener.endpoint, talker.endpoint, true);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should unpause before simulated interface timeout");
  usleep(50000);

  engine.setInterfaceHealthForTest(false);
  ok &= check(waitFor([&]() { return engine.isPaused(); }),
              "engine should pause when interface becomes unhealthy");

  unpauseEngine(controller);
  usleep(30000);
  ok &= check(engine.isPaused(),
              "engine should ignore SHARE unpause while interface remains unhealthy");

  engine.setInterfaceHealthForTest(true);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "queued SHARE release during outage should resume once interface health recovers");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  responder.stop();
  std::remove(config_path.c_str());
  std::remove(listener.path.c_str());
  std::remove(talker.path.c_str());
  return ok;
}

static bool testHybridTriggerCommandMatchesButtonAndAxisEvents() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  toml::table lookup = toml::parse("targets = [ \"FIRE\" ]");
  std::vector<std::shared_ptr<GameCommand>> commands;
  engine.addGameCommands(lookup, "targets", commands);
  ok &= check(commands.size() == 1 && commands.front() != nullptr,
              "test setup should resolve FIRE command");
  if (commands.empty() || !commands.front()) {
    std::remove(config_path.c_str());
    return false;
  }

  DeviceEvent button_evt = {0, 1, TYPE_BUTTON, BUTTON_R2};
  DeviceEvent axis_evt = {0, JOYSTICK_MAX, TYPE_AXIS, AXIS_R2};
  DeviceEvent other_evt = {0, JOYSTICK_MAX, TYPE_AXIS, AXIS_LX};

  ok &= check(engine.eventMatches(button_evt, commands.front()),
              "R2 button event should match FIRE");
  ok &= check(engine.eventMatches(axis_evt, commands.front()),
              "R2 axis event should match FIRE");
  ok &= check(!engine.eventMatches(other_evt, commands.front()),
              "unrelated event should not match FIRE");

  std::remove(config_path.c_str());
  return ok;
}

static bool testSequencePressDefaultsHybridAxisToMax() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  toml::table sequence_config = toml::parse(R"(
probe = [ { event = "press", command = "FIRE" } ]
)");
  auto seq = engine.createSequence(sequence_config, "probe", true);
  ok &= check(seq != nullptr, "sequence parser should build sequence for FIRE press");
  if (!seq) {
    std::remove(config_path.c_str());
    return false;
  }

  const auto& events = seq->getEvents();
  ok &= check(events.size() == 4, "hybrid press should emit button+axis hold/release events");
  if (events.size() == 4) {
    ok &= check(events[1].type == TYPE_AXIS && events[1].id == AXIS_R2,
                "hybrid press should emit AXIS_R2 hold event");
    ok &= check(events[1].value == JOYSTICK_MAX,
                "hybrid FIRE press default value should use JOYSTICK_MAX");
  }

  std::remove(config_path.c_str());
  return ok;
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

static bool testInterfacePauseCommandPausesRunningEngine() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should be running before interface pause command");

  engine.newCommand("{\"pause\":1}");
  ok &= check(waitFor([&]() { return engine.isPaused(); }),
              "interface pause command should pause engine");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testResetRemovesActiveModsWhilePaused() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should be running before reset-while-paused test");

  engine.newCommand("{\"winner\":\"Inverted\"}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 1; }),
              "Inverted should become active before reset");

  engine.newCommand("{\"pause\":1}");
  ok &= check(waitFor([&]() { return engine.isPaused(); }),
              "engine should be paused before issuing reset command");

  engine.newCommand("{\"reset\":1}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 0; }),
              "reset command should clear active modifiers even while paused");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testScalingInvertedAndMoonwalkAffectExpectedAxes() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should be running before scaling tests");

  engine.newCommand("{\"winner\":\"Inverted\"}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 1; }),
              "Inverted should become active");

  controller.inject({0, -64, TYPE_AXIS, AXIS_RY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_RY, TYPE_AXIS) == 64; }),
              "Inverted should flip negative RY values to positive");

  controller.inject({0, 64, TYPE_AXIS, AXIS_RY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_RY, TYPE_AXIS) == -64; }),
              "Inverted should flip positive RY values to negative");

  controller.inject({0, -40, TYPE_AXIS, AXIS_LY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_LY, TYPE_AXIS) == -40; }),
              "Inverted should not affect MOVE_Y axis");

  engine.newCommand("{\"remove\":\"Inverted\"}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 0; }),
              "Inverted should be removable");

  engine.newCommand("{\"winner\":\"Moonwalk\"}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 1; }),
              "Moonwalk should become active");

  controller.inject({0, -70, TYPE_AXIS, AXIS_LY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_LY, TYPE_AXIS) == 70; }),
              "Moonwalk should flip negative LY values to positive");

  controller.inject({0, 70, TYPE_AXIS, AXIS_LY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_LY, TYPE_AXIS) == -70; }),
              "Moonwalk should flip positive LY values to negative");

  controller.inject({0, -36, TYPE_AXIS, AXIS_RY});
  ok &= check(waitFor([&]() { return engine.getState(AXIS_RY, TYPE_AXIS) == -36; }),
              "Moonwalk should not affect CAMERA_Y axis");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

static bool testSequenceBeginClipsOutOfRangeAxisValue() {
  bool ok = true;

  TestController controller;
  ChaosEngine engine(controller, "", "", false);
  const std::string config_path = writeConfigFile();
  ok &= check(engine.setGame(config_path), "test config should load");

  engine.start();
  unpauseEngine(controller);
  ok &= check(waitFor([&]() { return !engine.isPaused(); }),
              "engine should be running before sequence clip test");

  engine.newCommand("{\"winner\":\"SEQ_AXIS_CLIP\"}");
  ok &= check(waitFor([&]() { return activeCount(engine) == 1; }),
              "sequence axis-clip modifier should become active");
  ok &= check(waitFor([&]() { return engine.getState(AXIS_LY, TYPE_AXIS) == JOYSTICK_MAX; }),
              "begin_sequence axis value above range should clip to JOYSTICK_MAX");

  engine.stop();
  engine.WaitForInternalThreadToExit();
  std::remove(config_path.c_str());
  return ok;
}

int main() {
  bool ok = true;
  ok &= testFirstUnpauseKeepsHybridTriggersReleased();
  ok &= testReleaseOnlyShareUnpausesWhenReady();
  ok &= testInterfaceReconnectResumePaths();
  ok &= testInterfaceReconnectQueuesResumeIntentDuringOutage();
  ok &= testHybridTriggerCommandMatchesButtonAndAxisEvents();
  ok &= testSequencePressDefaultsHybridAxisToMax();
  ok &= testDuplicateActivationDoesNotStack();
  ok &= testRemoveDeferredUntilUpdateCompletes();
  ok &= testInterfacePauseCommandPausesRunningEngine();
  ok &= testResetRemovesActiveModsWhilePaused();
  ok &= testScalingInvertedAndMoonwalkAffectExpectedAxes();
  ok &= testSequenceBeginClipsOutOfRangeAxisValue();
  if (!ok) {
    return 1;
  }
  std::cout << "PASS: engine lifecycle regression tests\n";
  return 0;
}
