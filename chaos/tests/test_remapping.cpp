#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <unistd.h>

#include <toml++/toml.h>

#include <Controller.hpp>
#include <ControllerState.hpp>
#include <ControllerInputTable.hpp>
#include <EngineInterface.hpp>
#include <RemapModifier.hpp>
#include <signals.hpp>

using namespace Chaos;

class MockEngine : public EngineInterface {
public:
  Controller controller;
  ControllerInputTable signal_table;
  std::vector<DeviceEvent> applied_events;
  std::vector<DeviceEvent> pipelined_events;
  std::unordered_map<std::string, std::shared_ptr<Modifier>> modifier_map;
  std::list<std::shared_ptr<Modifier>> active_mods;

  MockEngine() : signal_table(controller) {}

  bool isPaused() override { return false; }

  void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) override {
    (void) sourceMod;
    pipelined_events.push_back(event);
  }

  short getState(uint8_t id, uint8_t type) override { return controller.getState(id, type); }
  bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) override {
    (void) event;
    (void) command;
    return false;
  }
  void setOff(std::shared_ptr<GameCommand> command) override { (void) command; }
  void setOn(std::shared_ptr<GameCommand> command) override { (void) command; }
  void setValue(std::shared_ptr<GameCommand> command, short value) override {
    (void) command;
    (void) value;
  }
  void applyEvent(const DeviceEvent& event) override {
    applied_events.push_back(event);
    controller.applyEvent(event);
  }

  std::shared_ptr<Modifier> getModifier(const std::string& name) override {
    auto it = modifier_map.find(name);
    if (it != modifier_map.end()) {
      return it->second;
    }
    return nullptr;
  }
  std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() override {
    return modifier_map;
  }
  std::list<std::shared_ptr<Modifier>>& getActiveMods() override { return active_mods; }

  std::shared_ptr<MenuItem> getMenuItem(const std::string& name) override {
    (void) name;
    return nullptr;
  }
  void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) override {
    (void) item;
    (void) new_val;
  }
  void restoreMenuState(std::shared_ptr<MenuItem> item) override { (void) item; }

  std::shared_ptr<ControllerInput> getInput(const std::string& name) override {
    return signal_table.getInput(name);
  }
  std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event) override {
    return signal_table.getInput(event);
  }
  void addControllerInputs(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<ControllerInput>>& vec) override {
    signal_table.addToVector(config, key, vec);
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<GameCommand>>& vec) override {
    (void) config;
    (void) key;
    (void) vec;
  }
  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<ControllerInput>>& vec) override {
    (void) config;
    (void) key;
    (void) vec;
  }
  void addGameConditions(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCondition>>& vec) override {
    (void) config;
    (void) key;
    (void) vec;
  }
  std::shared_ptr<Sequence> createSequence(toml::table& config, const std::string& key,
                                           bool required) override {
    (void) config;
    (void) key;
    (void) required;
    return nullptr;
  }
};

class ControllerStateProbe : public ControllerState {
public:
  ControllerStateProbe() = default;

  void getDeviceEvents(unsigned char* buffer, int length, std::vector<DeviceEvent>& events) override {
    (void) buffer;
    (void) length;
    (void) events;
  }

  void applyHackedState(unsigned char* buffer, short* chaosState) override {
    (void) buffer;
    (void) chaosState;
  }

  void onTouchpadActive(short value) { noteTouchpadActiveEvent(value); }
  void onTouchpadAxis() { noteTouchpadAxisEvent(); }
  void injectIfInactive(std::vector<DeviceEvent>& events) { addTouchpadInactivityEvents(events); }
};

static bool check(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

static std::shared_ptr<RemapModifier> makeRemap(const std::string& toml_text, MockEngine& engine) {
  toml::table config = toml::parse(toml_text);
  return std::make_shared<RemapModifier>(config, &engine);
}

static bool testAxisZeroClearsNegativeButton() {
  MockEngine engine;
  auto mod = makeRemap(
      R"(
name = "Axis Button Remap"
type = "remap"
remap = [
  { from = "RX", to = "SQUARE", to_neg = "CIRCLE", threshold = 0.5 }
]
)",
      engine);

  auto from = engine.getInput("RX");
  auto to = engine.getInput("SQUARE");
  auto to_neg = engine.getInput("CIRCLE");

  bool ok = true;
  DeviceEvent event = {0, 100, from->getButtonType(), from->getID()};
  ok &= check(mod->remap(event), "positive RX event should be accepted");
  ok &= check(event.type == to->getButtonType() && event.id == to->getID() && event.value == 1,
              "positive RX should map to SQUARE press");
  ok &= check(engine.applied_events.size() == 1, "positive RX should clear opposite button once");

  DeviceEvent event_neg = {0, -100, from->getButtonType(), from->getID()};
  ok &= check(mod->remap(event_neg), "negative RX event should be accepted");
  ok &= check(event_neg.type == to_neg->getButtonType() && event_neg.id == to_neg->getID() &&
                  event_neg.value == 1,
              "negative RX should map to CIRCLE press");
  ok &= check(engine.applied_events.size() == 2,
              "negative RX should clear opposite button once");

  size_t before_zero = engine.applied_events.size();
  DeviceEvent event_zero = {0, 0, from->getButtonType(), from->getID()};
  ok &= check(mod->remap(event_zero), "zero RX event should be accepted");
  ok &= check(event_zero.type == to->getButtonType() && event_zero.id == to->getID() &&
                  event_zero.value == 0,
              "zero RX should emit SQUARE release");
  ok &= check(engine.applied_events.size() == before_zero + 1,
              "zero RX should explicitly clear the negative button");
  if (engine.applied_events.size() > before_zero) {
    const auto& cleared = engine.applied_events.back();
    ok &= check(cleared.type == to_neg->getButtonType() && cleared.id == to_neg->getID() &&
                    cleared.value == 0,
                "zero RX should clear CIRCLE");
  }
  return ok;
}

static bool testInvertUsesRemappedValue() {
  MockEngine engine;
  auto mod = makeRemap(
      R"(
name = "DPad Invert"
type = "remap"
remap = [
  { from = "DY", to = "RY", invert = true }
]
)",
      engine);

  auto from = engine.getInput("DY");
  auto to = engine.getInput("RY");

  DeviceEvent event = {0, 1, from->getButtonType(), from->getID()};
  bool ok = true;
  ok &= check(mod->remap(event), "DY event should be accepted");
  ok &= check(event.type == to->getButtonType() && event.id == to->getID(),
              "DY should map to RY");
  ok &= check(event.value == static_cast<short>(-JOYSTICK_MAX),
              "invert should use remapped axis value, not raw source value");
  return ok;
}

static bool testTouchpadStopClearsConfiguredAxes() {
  MockEngine engine;
  auto mod = makeRemap(
      R"(
name = "Touchpad Aiming Test"
type = "remap"
disable_signals = [ "RX", "RY" ]
remap = [
  { from = "TOUCHPAD_ACTIVE", to = "NOTHING" }
]
)",
      engine);

  mod->begin();
  engine.applied_events.clear();
  engine.pipelined_events.clear();

  auto touchpad_active = engine.getInput("TOUCHPAD_ACTIVE");
  auto rx = engine.getInput("RX");
  auto ry = engine.getInput("RY");

  bool ok = true;
  DeviceEvent start = {0, 1, touchpad_active->getButtonType(), touchpad_active->getID()};
  ok &= check(mod->remap(start), "TOUCHPAD_ACTIVE start should be accepted");

  DeviceEvent stop = {0, 0, touchpad_active->getButtonType(), touchpad_active->getID()};
  ok &= check(mod->remap(stop), "TOUCHPAD_ACTIVE stop should be accepted");

  ok &= check(engine.pipelined_events.empty(),
              "touchpad stop path should not call fakePipelinedEvent from remap");
  ok &= check(engine.applied_events.size() == 2,
              "touchpad stop should clear each configured axis once");

  bool saw_rx = false;
  bool saw_ry = false;
  for (const auto& ev : engine.applied_events) {
    ok &= check(ev.type == TYPE_AXIS, "touchpad stop should emit axis events");
    ok &= check(ev.value == 0, "touchpad stop should zero each axis");
    if (ev.id == rx->getID()) {
      saw_rx = true;
    } else if (ev.id == ry->getID()) {
      saw_ry = true;
    }
  }
  ok &= check(saw_rx && saw_ry, "touchpad stop should clear both RX and RY");
  return ok;
}

static bool testTouchpadInactiveDelayInjection() {
  ControllerStateProbe probe;
  ControllerState::setTouchpadInactiveDelay(0.001);
  probe.onTouchpadActive(1);
  probe.onTouchpadAxis();
  usleep(3000);

  std::vector<DeviceEvent> events;
  probe.injectIfInactive(events);

  bool ok = true;
  ok &= check(events.size() == 3, "touchpad inactivity should inject release and two axis resets");
  if (events.size() == 3) {
    ok &= check(events[0].type == TYPE_BUTTON && events[0].id == BUTTON_TOUCHPAD_ACTIVE && events[0].value == 0,
                "first injected event should release TOUCHPAD_ACTIVE");
    ok &= check(events[1].type == TYPE_AXIS && events[1].id == AXIS_TOUCHPAD_X && events[1].value == 0,
                "second injected event should reset TOUCHPAD_X");
    ok &= check(events[2].type == TYPE_AXIS && events[2].id == AXIS_TOUCHPAD_Y && events[2].value == 0,
                "third injected event should reset TOUCHPAD_Y");
  }
  return ok;
}

static bool testTouchpadInactiveDelayParsing() {
  Controller controller;
  ControllerInputTable table(controller);
  toml::table config = toml::parse(R"(
[controller]
touchpad_inactive_delay = 0.123
touchpad_velocity = false
touchpad_scale_x = 1.0
touchpad_scale_y = 1.0
touchpad_skew = 0
)");
  int errors = table.initializeInputs(config);

  bool ok = true;
  ok &= check(errors == 0, "touchpad settings parse should succeed");
  ok &= check(std::fabs(ControllerState::getTouchpadInactiveDelay() - 0.123) < 1e-6,
              "touchpad_inactive_delay should be propagated to ControllerState");
  return ok;
}

int main() {
  bool ok = true;
  ok &= testAxisZeroClearsNegativeButton();
  ok &= testInvertUsesRemappedValue();
  ok &= testTouchpadStopClearsConfiguredAxes();
  ok &= testTouchpadInactiveDelayInjection();
  ok &= testTouchpadInactiveDelayParsing();

  if (!ok) {
    return 1;
  }
  std::cout << "PASS: remap regression tests\n";
  return 0;
}
