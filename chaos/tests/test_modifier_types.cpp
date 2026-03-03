#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <unistd.h>

#include <toml++/toml.h>

#include <Controller.hpp>
#include <ControllerInput.hpp>
#include <ControllerInputTable.hpp>
#include <CooldownModifier.hpp>
#include <DelayModifier.hpp>
#include <DisableModifier.hpp>
#include <DeviceEvent.hpp>
#include <EngineInterface.hpp>
#include <FormulaModifier.hpp>
#include <GameCommand.hpp>
#include <GameCondition.hpp>
#include <MenuInterface.hpp>
#include <MenuItem.hpp>
#include <MenuModifier.hpp>
#include <ParentModifier.hpp>
#include <RepeatModifier.hpp>
#include <ScalingModifier.hpp>
#include <Sequence.hpp>
#include <SequenceModifier.hpp>
#include <signals.hpp>

using namespace Chaos;

class MockMenuInterface : public MenuInterface {
public:
  std::unordered_map<std::string, std::shared_ptr<MenuItem>> items;

  std::shared_ptr<MenuItem> getMenuItem(const std::string& name) override {
    auto it = items.find(name);
    if (it != items.end()) {
      return it->second;
    }
    return nullptr;
  }

  void correctOffset(std::shared_ptr<MenuItem> sender) override { (void) sender; }
  void addToSequence(Sequence& sequence, const std::string& name) override {
    (void) sequence;
    (void) name;
  }
};

class MockEngine : public EngineInterface {
public:
  Controller controller;
  ControllerInputTable signal_table;
  MockMenuInterface menu_iface;

  std::unordered_map<std::string, std::shared_ptr<GameCommand>> command_map;
  std::unordered_map<std::string, std::shared_ptr<GameCondition>> condition_map;
  std::unordered_map<std::string, std::shared_ptr<Modifier>> modifier_map;
  std::list<std::shared_ptr<Modifier>> active_mods;

  std::vector<DeviceEvent> pipelined_events;
  std::vector<std::string> set_off_calls;
  std::vector<std::string> set_on_calls;
  std::vector<std::pair<std::string, short>> set_value_calls;
  std::vector<std::pair<std::string, unsigned int>> menu_set_calls;
  std::vector<std::string> menu_restore_calls;

  struct ConditionSpec {
    std::vector<std::string> while_commands;
    std::vector<std::string> clear_on_commands;
    double threshold = 1.0;
    ThresholdType threshold_type = ThresholdType::ABOVE;
    double clear_threshold = 1.0;
    ThresholdType clear_threshold_type = ThresholdType::ABOVE;
  };

  MockEngine() : signal_table(controller) {
    addCommand("CAMERA_X", "RX");
    addCommand("CAMERA_Y", "RY");
    addCommand("MOVE_X", "LX");
    addCommand("MOVE_Y", "LY");
    addCommand("DODGE", "L1");
    addCommand("AIM", "L2");
    addCommand("LISTEN", "R1");
    addCommand("FIRE", "R2");
    addCommand("GRAB", "TRIANGLE");
    addCommand("JUMP", "X");
    addCommand("CROUCH", "CIRCLE");
    addCommand("MELEE", "SQUARE");
    addCommand("GET_GUN", "DX");
    addCommand("GET_CONSUMABLE", "DY");
    addCondition("aiming", {{"AIM"}});
    addCondition("not_aiming", {{"AIM"}, {}, 1.0, ThresholdType::BELOW});
    addCondition("sprint", {{"DODGE"}});
    addCondition("movement", {{"MOVE_Y", "MOVE_X"}, {}, 0.2, ThresholdType::DISTANCE});
    addCondition("fast_movement", {{"MOVE_Y", "MOVE_X"}, {}, 0.8, ThresholdType::DISTANCE});
    addCondition("camera", {{"CAMERA_X", "CAMERA_Y"}, {}, 0.2, ThresholdType::DISTANCE});
    addCondition("gun_selected", {{"GET_GUN"}, {"GET_CONSUMABLE"}});
    addCondition("consumable_selected", {{"GET_CONSUMABLE"}, {"GET_GUN"}});
    addCondition("shooting", {{"AIM", "FIRE"}});
    addCondition("melee", {{"MELEE"}});
  }

  bool isPaused() override { return false; }

  void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) override {
    (void) sourceMod;
    pipelined_events.push_back(event);
  }

  short getState(uint8_t id, uint8_t type) override { return controller.getState(id, type); }
  bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) override {
    return command && command->getInput() && command->getInput()->matches(event);
  }
  void setOff(std::shared_ptr<GameCommand> command) override {
    if (command) {
      set_off_calls.push_back(command->getName());
      auto input = command->getInput();
      if (input) {
        DeviceEvent ev = {0, input->getMin(input->getButtonType()), static_cast<uint8_t>(input->getButtonType()),
                          input->getID()};
        controller.applyEvent(ev);
      }
    }
  }
  void setOn(std::shared_ptr<GameCommand> command) override {
    if (command) {
      set_on_calls.push_back(command->getName());
      auto input = command->getInput();
      if (input) {
        DeviceEvent ev = {0, input->getMax(input->getButtonType()), static_cast<uint8_t>(input->getButtonType()),
                          input->getID()};
        controller.applyEvent(ev);
      }
    }
  }
  void setValue(std::shared_ptr<GameCommand> command, short value) override {
    if (command) {
      set_value_calls.push_back({command->getName(), value});
      auto input = command->getInput();
      if (input) {
        DeviceEvent ev = {0, value, static_cast<uint8_t>(input->getButtonType()), input->getID()};
        controller.applyEvent(ev);
      }
    }
  }
  void applyEvent(const DeviceEvent& event) override { controller.applyEvent(event); }

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
    return menu_iface.getMenuItem(name);
  }
  void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) override {
    if (item) {
      menu_set_calls.push_back({item->getName(), new_val});
    }
  }
  void restoreMenuState(std::shared_ptr<MenuItem> item) override {
    if (item) {
      menu_restore_calls.push_back(item->getName());
    }
  }

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
  std::string getEventName(const DeviceEvent& event) override {
    auto input = signal_table.getInput(event);
    return input ? input->getName() : "UNKNOWN_EVENT";
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<GameCommand>>& vec) override {
    if (!config.contains(key)) {
      return;
    }
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
    }
    for (const auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      if (!cmd) {
        continue;
      }
      auto it = command_map.find(*cmd);
      if (it == command_map.end()) {
        throw std::runtime_error("Unrecognized command: " + *cmd + " in " + key);
      }
      vec.push_back(it->second);
    }
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<ControllerInput>>& vec) override {
    if (!config.contains(key)) {
      return;
    }
    const toml::array* cmd_list = config.get(key)->as_array();
    if (!cmd_list || !cmd_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
    }
    for (const auto& elem : *cmd_list) {
      std::optional<std::string> cmd = elem.value<std::string>();
      if (!cmd) {
        continue;
      }
      auto it = command_map.find(*cmd);
      if (it == command_map.end()) {
        throw std::runtime_error("Unrecognized command: " + *cmd + " in " + key);
      }
      vec.push_back(it->second->getInput());
    }
  }

  void addGameConditions(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCondition>>& vec) override {
    if (!config.contains(key)) {
      return;
    }
    const toml::array* cond_list = config.get(key)->as_array();
    if (!cond_list || !cond_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error(key + " must be an array of strings");
    }
    for (const auto& elem : *cond_list) {
      std::optional<std::string> cond_name = elem.value<std::string>();
      if (!cond_name) {
        continue;
      }
      auto it = condition_map.find(*cond_name);
      if (it == condition_map.end()) {
        throw std::runtime_error("Unrecognized condition: " + *cond_name + " in " + key);
      }
      vec.push_back(std::make_shared<GameCondition>(*it->second));
    }
  }

  std::shared_ptr<Sequence> createSequence(toml::table& config, const std::string& key,
                                           bool required) override {
    if (!config.contains(key)) {
      if (required) {
        throw std::runtime_error("Missing required sequence key: " + key);
      }
      return nullptr;
    }
    auto seq = std::make_shared<Sequence>(controller);
    auto fire_it = command_map.find("FIRE");
    auto fire = (fire_it != command_map.end()) ? fire_it->second->getInput() : nullptr;
    if (!fire) {
      throw std::runtime_error("Missing FIRE command/input in test setup");
    }
    // Use non-zero event time so SequenceModifier remains in IN_SEQUENCE long enough to test blocking.
    seq->addEvent({10000, 1, static_cast<uint8_t>(fire->getButtonType()), fire->getID()});
    return seq;
  }

private:
  void addCommand(const std::string& name, const std::string& signal_name) {
    auto signal = signal_table.getInput(signal_name);
    if (!signal) {
      throw std::runtime_error("Unknown signal in test setup: " + signal_name);
    }
    command_map[name] = std::make_shared<GameCommand>(name, signal);
  }

  void addCondition(const std::string& name, const ConditionSpec& spec) {
    auto cond = std::make_shared<GameCondition>(name);
    for (const auto& cmd_name : spec.while_commands) {
      auto it = command_map.find(cmd_name);
      if (it == command_map.end()) {
        throw std::runtime_error("Unknown command in condition setup: " + cmd_name);
      }
      cond->addWhile(it->second);
    }
    if (spec.while_commands.empty()) {
      throw std::runtime_error("Condition setup must include at least one while command: " + name);
    }
    cond->setThresholdType(spec.threshold_type);
    cond->setThreshold(spec.threshold);

    for (const auto& cmd_name : spec.clear_on_commands) {
      auto it = command_map.find(cmd_name);
      if (it == command_map.end()) {
        throw std::runtime_error("Unknown clear_on command in condition setup: " + cmd_name);
      }
      cond->addClearOn(it->second);
    }
    if (!spec.clear_on_commands.empty()) {
      cond->setClearThresholdType(spec.clear_threshold_type);
      cond->setClearThreshold(spec.clear_threshold);
    }
    condition_map[name] = cond;
  }
};

class ProbeChildModifier : public Modifier::Registrar<ProbeChildModifier> {
public:
  static const std::string mod_type;
  static std::unordered_map<std::string, int> begin_calls;
  static std::unordered_map<std::string, int> update_calls;
  static std::unordered_map<std::string, int> finish_calls;
  static std::unordered_map<std::string, int> tweak_calls;

  ProbeChildModifier(toml::table& config, EngineInterface* e) {
    initialize(config, e);
    delta = config["delta"].value_or(0);
    should_block = config["block"].value_or(false);
  }

  const std::string& getModType() override { return mod_type; }

  void begin() override { begin_calls[getName()]++; }
  void update() override { update_calls[getName()]++; }
  void finish() override { finish_calls[getName()]++; }

  bool tweak(DeviceEvent& event) override {
    tweak_calls[getName()]++;
    event.value = static_cast<short>(event.value + delta);
    return !should_block;
  }

  static void reset() {
    begin_calls.clear();
    update_calls.clear();
    finish_calls.clear();
    tweak_calls.clear();
  }

private:
  int delta = 0;
  bool should_block = false;
};

const std::string ProbeChildModifier::mod_type = "probe_child";
std::unordered_map<std::string, int> ProbeChildModifier::begin_calls;
std::unordered_map<std::string, int> ProbeChildModifier::update_calls;
std::unordered_map<std::string, int> ProbeChildModifier::finish_calls;
std::unordered_map<std::string, int> ProbeChildModifier::tweak_calls;

static bool check(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

template <typename T>
static std::shared_ptr<T> makeMod(const std::string& toml_text, MockEngine& engine) {
  toml::table config = toml::parse(toml_text);
  return std::make_shared<T>(config, &engine);
}

static std::shared_ptr<ControllerInput> commandInput(MockEngine& engine, const std::string& command_name) {
  auto it = engine.command_map.find(command_name);
  if (it == engine.command_map.end()) {
    return nullptr;
  }
  return it->second->getInput();
}

static DeviceEvent commandEvent(MockEngine& engine, const std::string& command_name, short value) {
  auto input = commandInput(engine, command_name);
  if (!input) {
    throw std::runtime_error("Unknown command in test event setup: " + command_name);
  }
  return {0, value, static_cast<uint8_t>(input->getButtonType()), input->getID()};
}

static DeviceEvent commandHybridAxisEvent(MockEngine& engine, const std::string& command_name, short value) {
  auto input = commandInput(engine, command_name);
  if (!input) {
    throw std::runtime_error("Unknown command in test hybrid event setup: " + command_name);
  }
  if (input->getType() != ControllerSignalType::HYBRID) {
    throw std::runtime_error("Command is not hybrid in test hybrid event setup: " + command_name);
  }
  return {0, value, TYPE_AXIS, input->getHybridAxis()};
}

static bool sawName(const std::vector<std::string>& calls, const std::string& name) {
  for (const auto& entry : calls) {
    if (entry == name) {
      return true;
    }
  }
  return false;
}

static bool testCooldownModifierBlocksAfterTrigger() {
  MockEngine engine;
  auto mod = makeMod<CooldownModifier>(
      R"(
name = "Cooldown Camera X"
type = "cooldown"
applies_to = [ "CAMERA_X" ]
trigger = [ "MOVE_X" ]
time_on = 0.002
time_off = 0.004
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "cooldown test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  DeviceEvent before_trigger = commandEvent(engine, "CAMERA_X", 400);
  ok &= check(mod->tweak(before_trigger), "CAMERA_X should pass before trigger");

  DeviceEvent trigger_evt = commandEvent(engine, "MOVE_X", 700);
  ok &= check(mod->tweak(trigger_evt), "trigger event should be accepted");

  usleep(3500);
  mod->_update(false);
  ok &= check(sawName(engine.set_off_calls, "CAMERA_X"),
              "cooldown should disable CAMERA_X when allow phase expires");

  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 600);
  ok &= check(!mod->tweak(blocked), "CAMERA_X should be blocked during cooldown");

  usleep(5000);
  mod->_update(false);
  DeviceEvent after_cooldown = commandEvent(engine, "CAMERA_X", 600);
  ok &= check(mod->tweak(after_cooldown), "CAMERA_X should pass after cooldown expires");
  return ok;
}

static bool testCooldownModifierRequiresWhileConditionWhenCumulativeStartType() {
  MockEngine engine;
  auto mod = makeMod<CooldownModifier>(
      R"(
name = "Cooldown With While"
type = "cooldown"
applies_to = [ "CAMERA_X" ]
while = [ "shooting" ]
start_type = "cumulative"
time_on = 0.002
time_off = 0.004
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  auto fire = commandInput(engine, "FIRE");
  bool ok = true;
  ok &= check(camera_x != nullptr && aim != nullptr && fire != nullptr,
              "cooldown-while test inputs should exist");
  if (!camera_x || !aim || !fire) {
    return false;
  }

  usleep(2500);
  mod->_update(false);
  ok &= check(engine.set_off_calls.empty(),
              "cooldown should not start while inCondition is false");

  engine.applyEvent(commandEvent(engine, "FIRE", 1));
  usleep(6000);
  mod->_update(false);
  ok &= check(engine.set_off_calls.empty(),
              "cooldown should stay untriggered if only FIRE is pressed");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false); // UNTRIGGERED -> ALLOW
  usleep(6000);
  mod->_update(false); // ALLOW, should reach BLOCK
  if (!sawName(engine.set_off_calls, "CAMERA_X")) {
    usleep(6000);
    mod->_update(false);
  }
  ok &= check(sawName(engine.set_off_calls, "CAMERA_X"),
              "cooldown should advance and block once while condition is true");

  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 600);
  ok &= check(!mod->tweak(blocked), "CAMERA_X should be blocked during cooldown");
  return ok;
}

static bool testCooldownModifierCancelableResetsIfConditionDrops() {
  MockEngine engine;
  auto mod = makeMod<CooldownModifier>(
      R"(
name = "Cooldown Cancelable"
type = "cooldown"
applies_to = [ "CAMERA_X" ]
while = [ "aiming" ]
start_type = "cancelable"
time_on = 0.004
time_off = 0.006
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(camera_x != nullptr && aim != nullptr,
              "cooldown-cancelable test inputs should exist");
  if (!camera_x || !aim) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);  // UNTRIGGERED -> ALLOW
  usleep(2000);
  mod->_update(false);  // ALLOW (partial progress)

  engine.applyEvent(commandEvent(engine, "AIM", 0));
  usleep(2000);
  mod->_update(false);  // ALLOW -> UNTRIGGERED (cancel)
  usleep(5000);
  mod->_update(false);  // should remain untriggered

  ok &= check(engine.set_off_calls.empty(),
              "cancelable cooldown should not enter block after condition drops early");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);  // UNTRIGGERED -> ALLOW
  usleep(6000);
  mod->_update(false);  // ALLOW -> BLOCK

  ok &= check(sawName(engine.set_off_calls, "CAMERA_X"),
              "cancelable cooldown should block after uninterrupted allow period");
  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 555);
  ok &= check(!mod->tweak(blocked),
              "cancelable cooldown should block configured command during cooldown");
  return ok;
}

static bool testDelayModifierQueuesAndReplays() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Camera X"
type = "delay"
applies_to = [ "CAMERA_X" ]
delay = 0.01
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "delay test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  DeviceEvent delayed = commandEvent(engine, "CAMERA_X", 1234);
  ok &= check(!mod->tweak(delayed), "target event should be queued and blocked");

  mod->_update(false);
  ok &= check(engine.pipelined_events.empty(), "event should not replay before delay elapses");

  usleep(13000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 1, "event should replay once after delay");
  if (engine.pipelined_events.size() == 1) {
    const auto& replay = engine.pipelined_events[0];
    ok &= check(replay.type == camera_x->getButtonType() && replay.id == camera_x->getID() &&
                    replay.value == 1234,
                "replayed event should match the queued event");
  }

  DeviceEvent passthrough = commandEvent(engine, "MOVE_X", 222);
  ok &= check(mod->tweak(passthrough), "non-target event should pass through");
  return ok;
}

static bool testDelayModifierAppliesToAllCommands() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay ALL"
type = "delay"
applies_to = "ALL"
delay = 0.005
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto jump = commandInput(engine, "JUMP");
  bool ok = true;
  ok &= check(camera_x != nullptr && jump != nullptr, "delay-all test inputs should exist");
  if (!camera_x || !jump) {
    return false;
  }

  DeviceEvent camera_evt = commandEvent(engine, "CAMERA_X", 321);
  DeviceEvent jump_evt = commandEvent(engine, "JUMP", 1);
  ok &= check(!mod->tweak(camera_evt), "delay-all should queue and block axis events");
  ok &= check(!mod->tweak(jump_evt), "delay-all should queue and block button events");

  usleep(7000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "delay-all should replay all queued events");
  return ok;
}

static bool testDelayModifierReplaysMultipleSequentialEvents() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Camera X Multiple"
type = "delay"
applies_to = [ "CAMERA_X" ]
delay = 0.01
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "delay-multi test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent first = commandEvent(engine, "CAMERA_X", 100);
  DeviceEvent second = commandEvent(engine, "CAMERA_X", -80);
  ok &= check(!mod->tweak(first), "first delayed event should be queued and blocked");
  ok &= check(!mod->tweak(second), "second delayed event should be queued and blocked");

  usleep(13000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "delay modifier should replay multiple queued events");
  if (engine.pipelined_events.size() == 2) {
    ok &= check(engine.pipelined_events[0].id == camera_x->getID() &&
                    engine.pipelined_events[0].type == camera_x->getButtonType() &&
                    engine.pipelined_events[0].value == 100,
                "first replayed delayed event should preserve ordering");
    ok &= check(engine.pipelined_events[1].id == camera_x->getID() &&
                    engine.pipelined_events[1].type == camera_x->getButtonType() &&
                    engine.pipelined_events[1].value == -80,
                "second replayed delayed event should preserve ordering");
  }
  return ok;
}

static bool testDelayModifierDelaysHybridButtonAndAxis() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Aim Hybrid"
type = "delay"
applies_to = [ "AIM" ]
delay = 0.01
)",
      engine);
  mod->_begin();

  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(aim != nullptr && aim->getType() == ControllerSignalType::HYBRID,
              "delay-hybrid test input should exist and be hybrid");
  if (!aim || aim->getType() != ControllerSignalType::HYBRID) {
    return false;
  }

  DeviceEvent aim_button_press = commandEvent(engine, "AIM", 1);
  DeviceEvent aim_axis_press = commandHybridAxisEvent(engine, "AIM", JOYSTICK_MAX);
  ok &= check(!mod->tweak(aim_button_press), "delay-hybrid should queue and block AIM button");
  ok &= check(!mod->tweak(aim_axis_press), "delay-hybrid should queue and block AIM axis");

  mod->_update(false);
  ok &= check(engine.pipelined_events.empty(),
              "delay-hybrid should not replay before delay elapses");

  usleep(13000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "delay-hybrid should replay both button and axis components");
  if (engine.pipelined_events.size() == 2) {
    ok &= check(engine.pipelined_events[0].type == TYPE_BUTTON &&
                    engine.pipelined_events[0].id == aim->getID() &&
                    engine.pipelined_events[0].value == 1,
                "delay-hybrid should replay button component first");
    ok &= check(engine.pipelined_events[1].type == TYPE_AXIS &&
                    engine.pipelined_events[1].id == aim->getHybridAxis() &&
                    engine.pipelined_events[1].value == JOYSTICK_MAX,
                "delay-hybrid should replay axis component second");
  }
  return ok;
}

static bool testDelayModifierClearsQueueAcrossLifecycle() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Queue Reset"
type = "delay"
applies_to = [ "CAMERA_X" ]
delay = 0.005
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "delay-reset test input should exist");
  if (!camera_x) {
    return false;
  }

  mod->_begin();
  DeviceEvent stale = commandEvent(engine, "CAMERA_X", 55);
  ok &= check(!mod->tweak(stale), "stale delayed event should queue while mod is active");
  mod->_finish();

  engine.pipelined_events.clear();
  mod->_begin();
  DeviceEvent fresh = commandEvent(engine, "CAMERA_X", -33);
  ok &= check(!mod->tweak(fresh), "fresh delayed event should queue after restart");

  usleep(7000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 1,
              "delay queue should be cleared between activations");
  if (engine.pipelined_events.size() == 1) {
    ok &= check(engine.pipelined_events[0].id == camera_x->getID() &&
                    engine.pipelined_events[0].type == camera_x->getButtonType() &&
                    engine.pipelined_events[0].value == -33,
                "only the fresh delayed event should replay after restart");
  }
  return ok;
}

static bool testDelayModifierStressRapidDodgeEvents() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Dodge Stress"
type = "delay"
applies_to = [ "DODGE" ]
delay = 0.002
)",
      engine);
  mod->_begin();

  auto dodge = commandInput(engine, "DODGE");
  bool ok = true;
  ok &= check(dodge != nullptr, "delay-dodge-stress input should exist");
  if (!dodge) {
    return false;
  }

  std::vector<short> expected_values;
  expected_values.reserve(120);
  for (int i = 0; i < 120; ++i) {
    short value = static_cast<short>((i % 2) == 0 ? 1 : 0);
    expected_values.push_back(value);
    DeviceEvent evt = commandEvent(engine, "DODGE", value);
    ok &= check(!mod->tweak(evt), "dodge stress event should be queued and blocked");
    if ((i % 8) == 0) {
      mod->_update(false);
    }
  }

  for (int poll = 0; poll < 80 && engine.pipelined_events.size() < expected_values.size(); ++poll) {
    usleep(1500);
    mod->_update(false);
  }

  ok &= check(engine.pipelined_events.size() == expected_values.size(),
              "dodge stress should replay all delayed events");
  if (engine.pipelined_events.size() == expected_values.size()) {
    for (size_t i = 0; i < expected_values.size(); ++i) {
      const auto& ev = engine.pipelined_events[i];
      ok &= check(ev.id == dodge->getID() && ev.type == dodge->getButtonType(),
                  "dodge stress replay should preserve command binding");
      ok &= check(ev.value == expected_values[i],
                  "dodge stress replay should preserve ordering and value");
    }
  }
  return ok;
}

static bool testDelayModifierStressInterleavedJoystickEvents() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Joystick Stress"
type = "delay"
applies_to = [ "CAMERA_X", "CAMERA_Y", "MOVE_X", "MOVE_Y" ]
delay = 0.002
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto camera_y = commandInput(engine, "CAMERA_Y");
  auto move_x = commandInput(engine, "MOVE_X");
  auto move_y = commandInput(engine, "MOVE_Y");
  bool ok = true;
  ok &= check(camera_x != nullptr && camera_y != nullptr && move_x != nullptr && move_y != nullptr,
              "delay-joystick-stress inputs should exist");
  if (!camera_x || !camera_y || !move_x || !move_y) {
    return false;
  }

  std::vector<DeviceEvent> expected;
  expected.reserve(160);
  for (int i = 0; i < 40; ++i) {
    const short base = static_cast<short>((i % 2 == 0) ? (20 + i) : -(20 + i));
    expected.push_back(commandEvent(engine, "MOVE_X", base));
    expected.push_back(commandEvent(engine, "MOVE_Y", static_cast<short>(-base)));
    expected.push_back(commandEvent(engine, "CAMERA_X", static_cast<short>(base / 2)));
    expected.push_back(commandEvent(engine, "CAMERA_Y", static_cast<short>(-base / 2)));
  }

  for (size_t i = 0; i < expected.size(); ++i) {
    DeviceEvent evt = expected[i];
    ok &= check(!mod->tweak(evt), "joystick stress event should be queued and blocked");
    if ((i % 10) == 0) {
      mod->_update(false);
    }
  }

  for (int poll = 0; poll < 100 && engine.pipelined_events.size() < expected.size(); ++poll) {
    usleep(1500);
    mod->_update(false);
  }

  ok &= check(engine.pipelined_events.size() == expected.size(),
              "joystick stress should replay all delayed joystick events");
  if (engine.pipelined_events.size() == expected.size()) {
    for (size_t i = 0; i < expected.size(); ++i) {
      const auto& replay = engine.pipelined_events[i];
      const auto& original = expected[i];
      ok &= check(replay.id == original.id && replay.type == original.type && replay.value == original.value,
                  "joystick stress replay should preserve event identity and ordering");
    }
  }
  return ok;
}

static bool testDisableDefaultFilterBlocksConfiguredCommandOnly() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Camera X"
type = "disable"
applies_to = [ "CAMERA_X" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "disable test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  DeviceEvent camera_pos = commandEvent(engine, "CAMERA_X", 777);
  ok &= check(mod->tweak(camera_pos), "disable tweak should return true for target event");
  ok &= check(camera_pos.value == 0, "default disable filter should block positive target values");

  DeviceEvent camera_neg = commandEvent(engine, "CAMERA_X", -777);
  ok &= check(mod->tweak(camera_neg), "disable tweak should return true for negative target event");
  ok &= check(camera_neg.value == 0, "default disable filter should block negative target values");

  DeviceEvent move_evt = commandEvent(engine, "MOVE_X", 555);
  ok &= check(mod->tweak(move_evt), "disable tweak should return true for non-target event");
  ok &= check(move_evt.value == 555, "events outside applies_to should remain unchanged");
  return ok;
}

static bool testDisableRespectsWhileCondition() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable While Shooting"
type = "disable"
applies_to = [ "CAMERA_X" ]
while = [ "shooting" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  auto fire = commandInput(engine, "FIRE");
  bool ok = true;
  ok &= check(camera_x != nullptr && aim != nullptr && fire != nullptr,
              "disable-while test inputs should exist");
  if (!camera_x || !aim || !fire) {
    return false;
  }

  DeviceEvent before = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(before), "disable-while should accept target event");
  ok &= check(before.value == 700, "disable should not apply while condition is false");

  engine.applyEvent(commandEvent(engine, "FIRE", 1));
  DeviceEvent during_fire_only = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(during_fire_only), "disable-while should accept event with only FIRE");
  ok &= check(during_fire_only.value == 700,
              "disable should not apply until both AIM and FIRE are active");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  DeviceEvent during = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(during), "disable-while should accept event when active");
  ok &= check(during.value == 0, "disable should apply while condition is true");

  engine.applyEvent(commandEvent(engine, "AIM", 0));
  DeviceEvent after = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(after), "disable-while should accept event after release");
  ok &= check(after.value == 700, "disable should stop applying when condition is false");
  return ok;
}

static bool testDisableRespectsDistanceMovementCondition() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Camera While Moving"
type = "disable"
applies_to = [ "CAMERA_X" ]
while = [ "movement" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "disable-distance test input should exist");
  if (!camera_x) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "MOVE_X", 10));
  engine.applyEvent(commandEvent(engine, "MOVE_Y", 0));
  DeviceEvent below_thresh = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(below_thresh), "disable-distance should accept event below threshold");
  ok &= check(below_thresh.value == 700,
              "distance condition should remain false below movement threshold");

  engine.applyEvent(commandEvent(engine, "MOVE_X", 40));
  engine.applyEvent(commandEvent(engine, "MOVE_Y", 0));
  DeviceEvent above_thresh = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(above_thresh), "disable-distance should accept event above threshold");
  ok &= check(above_thresh.value == 0,
              "distance condition should activate disable above movement threshold");
  return ok;
}

static bool testDisableRespectsPersistentCondition() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Camera While Gun Selected"
type = "disable"
applies_to = [ "CAMERA_X" ]
while = [ "gun_selected" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "disable-persistent test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent before = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(before), "disable-persistent should accept event before trigger");
  ok &= check(before.value == 700, "persistent condition should start false");

  engine.applyEvent(commandEvent(engine, "GET_GUN", 1));
  DeviceEvent active = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(active), "disable-persistent should accept event after trigger");
  ok &= check(active.value == 0, "persistent condition should stay active after GET_GUN");

  engine.applyEvent(commandEvent(engine, "GET_GUN", 0));
  DeviceEvent still_active = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(still_active), "disable-persistent should accept event while latched");
  ok &= check(still_active.value == 0,
              "persistent condition should remain true until clear_on command occurs");

  engine.applyEvent(commandEvent(engine, "GET_CONSUMABLE", 1));
  DeviceEvent cleared = commandEvent(engine, "CAMERA_X", 700);
  ok &= check(mod->tweak(cleared), "disable-persistent should accept event after clear");
  ok &= check(cleared.value == 700, "persistent condition should clear on GET_CONSUMABLE");
  return ok;
}

static bool testDisableFilterAboveBlocksOnlyPositiveValues() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Positive Camera X"
type = "disable"
applies_to = [ "CAMERA_X" ]
filter = "above"
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "disable-above test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent pos = commandEvent(engine, "CAMERA_X", 900);
  ok &= check(mod->tweak(pos), "disable-above should accept positive event");
  ok &= check(pos.value == 0, "disable-above should block positive values");

  DeviceEvent neg = commandEvent(engine, "CAMERA_X", -900);
  ok &= check(mod->tweak(neg), "disable-above should accept negative event");
  ok &= check(neg.value == -900, "disable-above should preserve negative values");
  return ok;
}

static bool testDisableFilterBelowBlocksOnlyNegativeValues() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Negative Camera X"
type = "disable"
applies_to = [ "CAMERA_X" ]
filter = "below"
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "disable-below test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent pos = commandEvent(engine, "CAMERA_X", 900);
  ok &= check(mod->tweak(pos), "disable-below should accept positive event");
  ok &= check(pos.value == 900, "disable-below should preserve positive values");

  DeviceEvent neg = commandEvent(engine, "CAMERA_X", -900);
  ok &= check(mod->tweak(neg), "disable-below should accept negative event");
  ok &= check(neg.value == 0, "disable-below should block negative values");
  return ok;
}

static bool testDisableFilterDirectionsOnThreeStateSelectWeapon() {
  MockEngine engine;
  auto mod_above = makeMod<DisableModifier>(
      R"(
name = "No Short Guns Style"
type = "disable"
applies_to = [ "GET_GUN" ]
filter = "above"
)",
      engine);
  auto mod_below = makeMod<DisableModifier>(
      R"(
name = "No Long Guns Style"
type = "disable"
applies_to = [ "GET_GUN" ]
filter = "below"
)",
      engine);

  auto get_gun = commandInput(engine, "GET_GUN");
  bool ok = true;
  ok &= check(get_gun != nullptr, "three-state get-gun input should exist");
  if (!get_gun) {
    return false;
  }

  DeviceEvent dx_pos = commandEvent(engine, "GET_GUN", 1);
  ok &= check(mod_above->tweak(dx_pos), "disable-above three-state should accept positive event");
  ok &= check(dx_pos.value == 0, "disable-above should block positive DX events");

  DeviceEvent dx_neg_for_above = commandEvent(engine, "GET_GUN", -1);
  ok &= check(mod_above->tweak(dx_neg_for_above), "disable-above three-state should accept negative event");
  ok &= check(dx_neg_for_above.value == -1, "disable-above should preserve negative DX events");

  DeviceEvent dx_neg = commandEvent(engine, "GET_GUN", -1);
  ok &= check(mod_below->tweak(dx_neg), "disable-below three-state should accept negative event");
  ok &= check(dx_neg.value == 0, "disable-below should block negative DX events");

  DeviceEvent dx_pos_for_below = commandEvent(engine, "GET_GUN", 1);
  ok &= check(mod_below->tweak(dx_pos_for_below), "disable-below three-state should accept positive event");
  ok &= check(dx_pos_for_below.value == 1, "disable-below should preserve positive DX events");

  return ok;
}

static bool testDisableBlocksAllConfiguredCommandsIncludingHybridAxis() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Pacifist Style Disable"
type = "disable"
applies_to = [ "MELEE", "FIRE" ]
)",
      engine);

  auto melee = commandInput(engine, "MELEE");
  auto fire = commandInput(engine, "FIRE");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(melee != nullptr && fire != nullptr && move_x != nullptr,
              "disable-multi test inputs should exist");
  if (!melee || !fire || !move_x) {
    return false;
  }

  DeviceEvent melee_evt = commandEvent(engine, "MELEE", 1);
  ok &= check(mod->tweak(melee_evt), "disable-multi should accept MELEE event");
  ok &= check(melee_evt.value == 0, "disable-multi should block first applies_to command");

  DeviceEvent fire_axis_evt = commandHybridAxisEvent(engine, "FIRE", JOYSTICK_MAX);
  ok &= check(mod->tweak(fire_axis_evt), "disable-multi should accept FIRE axis event");
  ok &= check(fire_axis_evt.value == JOYSTICK_MIN,
              "disable-multi should block hybrid-axis event for later applies_to command");

  DeviceEvent unaffected = commandEvent(engine, "MOVE_X", 700);
  ok &= check(mod->tweak(unaffected), "disable-multi should accept non-target event");
  ok &= check(unaffected.value == 700, "disable-multi should leave non-target command unchanged");
  return ok;
}

static bool testDisableNoAimingStyleBlocksHybridButtonAndAxis() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "No Aiming Style Disable"
type = "disable"
applies_to = [ "AIM" ]
)",
      engine);

  auto aim = commandInput(engine, "AIM");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(aim != nullptr && move_x != nullptr,
              "no-aiming disable test inputs should exist");
  if (!aim || !move_x) {
    return false;
  }

  DeviceEvent aim_button = commandEvent(engine, "AIM", 1);
  ok &= check(mod->tweak(aim_button), "no-aiming disable should accept AIM button event");
  ok &= check(aim_button.value == 0,
              "no-aiming disable should block AIM button component");

  DeviceEvent aim_axis = commandHybridAxisEvent(engine, "AIM", JOYSTICK_MAX);
  ok &= check(mod->tweak(aim_axis), "no-aiming disable should accept AIM axis event");
  ok &= check(aim_axis.value == JOYSTICK_MIN,
              "no-aiming disable should block AIM axis component to JOYSTICK_MIN");

  DeviceEvent unaffected = commandEvent(engine, "MOVE_X", 700);
  ok &= check(mod->tweak(unaffected), "no-aiming disable should accept non-target event");
  ok &= check(unaffected.value == 700,
              "no-aiming disable should not alter non-target command values");
  return ok;
}

static bool testFormulaModifierOffsetsAndRestores() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Pair"
type = "formula"
applies_to = [ "CAMERA_X", "MOVE_X" ]
formula_type = "circle"
amplitude = 0.25
period_length = 1.0
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "formula test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "CAMERA_X", 1000));
  engine.applyEvent(commandEvent(engine, "MOVE_X", -2000));

  mod->_begin();
  mod->_update(false);

  ok &= check(engine.pipelined_events.size() == 2, "formula update should emit one event per command");
  bool saw_aim = false;
  bool saw_move = false;
  for (const auto& ev : engine.pipelined_events) {
    if (ev.id == camera_x->getID()) {
      saw_aim = true;
      ok &= check(ev.value != 1000, "formula should offset CAMERA_X from base value");
    } else if (ev.id == move_x->getID()) {
      saw_move = true;
    }
  }
  ok &= check(saw_aim && saw_move, "formula update should emit both configured commands");

  DeviceEvent live = commandEvent(engine, "CAMERA_X", 500);
  ok &= check(mod->tweak(live), "formula tweak should accept live events");
  ok &= check(live.value != 500, "formula tweak should apply current offset to matching signal");

  mod->_finish();
  ok &= check(engine.pipelined_events.size() >= 4,
              "formula finish should append restoration events for each command");
  bool restored_aim = false;
  bool restored_move = false;
  for (size_t i = 2; i < engine.pipelined_events.size(); ++i) {
    const auto& ev = engine.pipelined_events[i];
    if (ev.id == camera_x->getID() && ev.value == 500) {
      restored_aim = true;
    }
    if (ev.id == move_x->getID() && ev.value == -2000) {
      restored_move = true;
    }
  }
  ok &= check(restored_aim && restored_move,
              "formula finish should restore latest non-skewed command values");
  return ok;
}

static bool testFormulaModifierJankyHonorsWhileCondition() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Janky While"
type = "formula"
applies_to = [ "CAMERA_X" ]
formula_type = "janky"
amplitude = 0.6
period_length = 1.0
while = [ "aiming" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(camera_x != nullptr && aim != nullptr, "formula-janky test inputs should exist");
  if (!camera_x || !aim) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "CAMERA_X", 40));
  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.empty(),
              "formula should not emit events while while-condition is false");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);
  ok &= check(!engine.pipelined_events.empty(),
              "formula should emit events while while-condition is true");
  return ok;
}

static bool testFormulaModifierSupportsEightCurveType() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Eight Curve"
type = "formula"
applies_to = [ "CAMERA_X", "MOVE_X" ]
formula_type = "eight_curve"
amplitude = 0.5
period_length = 1.0
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "formula-eight test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "eight_curve formula should emit one event per configured command");
  return ok;
}

static bool testFormulaCircleAssignsSinThenCosByAxisOrder() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Circle Ordered"
type = "formula"
applies_to = [ "CAMERA_X", "CAMERA_Y" ]
formula_type = "circle"
amplitude = 1.0
period_length = 1000.0
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto camera_y = commandInput(engine, "CAMERA_Y");
  bool ok = true;
  ok &= check(camera_x != nullptr && camera_y != nullptr, "formula-circle test inputs should exist");
  if (!camera_x || !camera_y) {
    return false;
  }

  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "formula-circle update should emit one event per configured command");
  if (engine.pipelined_events.size() < 2) {
    return false;
  }

  short x = 0;
  short y = 0;
  bool saw_x = false;
  bool saw_y = false;
  for (const auto& ev : engine.pipelined_events) {
    if (ev.id == camera_x->getID()) {
      x = ev.value;
      saw_x = true;
    } else if (ev.id == camera_y->getID()) {
      y = ev.value;
      saw_y = true;
    }
  }
  ok &= check(saw_x && saw_y, "formula-circle should emit both configured axis events");

  const short abs_x = static_cast<short>((x < 0) ? -x : x);
  ok &= check(abs_x <= 2, "first circle axis should start near sin(t)=0");
  ok &= check(y >= 120, "second circle axis should start near cos(t)=1");
  return ok;
}

static bool testFormulaEightCurveAlternatesAcrossAxisPairs() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Eight Pairing"
type = "formula"
applies_to = [ "CAMERA_X", "CAMERA_Y", "MOVE_X", "MOVE_Y" ]
formula_type = "eight_curve"
amplitude = 1.0
period_length = 2.0
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto camera_y = commandInput(engine, "CAMERA_Y");
  auto move_x = commandInput(engine, "MOVE_X");
  auto move_y = commandInput(engine, "MOVE_Y");
  bool ok = true;
  ok &= check(camera_x != nullptr && camera_y != nullptr && move_x != nullptr && move_y != nullptr,
              "formula-eight-pair test inputs should exist");
  if (!camera_x || !camera_y || !move_x || !move_y) {
    return false;
  }

  mod->_begin();
  usleep(250000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 4,
              "formula-eight-pair update should emit one event per configured axis");
  if (engine.pipelined_events.size() < 4) {
    return false;
  }

  short cam_x = 0;
  short cam_y = 0;
  short mov_x = 0;
  short mov_y = 0;
  bool saw_cam_x = false;
  bool saw_cam_y = false;
  bool saw_mov_x = false;
  bool saw_mov_y = false;
  for (const auto& ev : engine.pipelined_events) {
    if (ev.id == camera_x->getID()) {
      cam_x = ev.value;
      saw_cam_x = true;
    } else if (ev.id == camera_y->getID()) {
      cam_y = ev.value;
      saw_cam_y = true;
    } else if (ev.id == move_x->getID()) {
      mov_x = ev.value;
      saw_mov_x = true;
    } else if (ev.id == move_y->getID()) {
      mov_y = ev.value;
      saw_mov_y = true;
    }
  }
  ok &= check(saw_cam_x && saw_cam_y && saw_mov_x && saw_mov_y,
              "formula-eight-pair should emit all configured axis events");

  auto magnitude = [](short value) { return (value < 0) ? -value : value; };
  ok &= check(magnitude(cam_x) <= magnitude(cam_y),
              "first axis in each pair should use sin(t)*cos(t) component");
  ok &= check(magnitude(mov_x) <= magnitude(mov_y),
              "first axis in second pair should use sin(t)*cos(t) component");
  return ok;
}

static bool testScalingModifierScalesAndClamps() {
  MockEngine engine;
  auto mod = makeMod<ScalingModifier>(
      R"(
name = "Scale Camera X"
type = "scaling"
applies_to = [ "CAMERA_X" ]
amplitude = 2.0
offset = 100
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "scaling test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent normal = commandEvent(engine, "CAMERA_X", 10);
  ok &= check(mod->tweak(normal), "scaling tweak should accept matching event");
  ok &= check(normal.value == 120,
              "scaling should apply amplitude and offset (got " + std::to_string(normal.value) + ")");

  DeviceEvent clamp_hi = commandEvent(engine, "CAMERA_X", 120);
  ok &= check(mod->tweak(clamp_hi), "scaling tweak should accept high event");
  ok &= check(clamp_hi.value == JOYSTICK_MAX, "scaled value should clamp at JOYSTICK_MAX");
  return ok;
}

static bool testScalingModifierSupportsNegativeAmplitudeAndOffset() {
  MockEngine engine;
  bool ok = true;

  auto invert = makeMod<ScalingModifier>(
      R"(
name = "Invert"
type = "scaling"
applies_to = [ "CAMERA_X" ]
amplitude = -1
)",
      engine);
  auto camera_x = commandInput(engine, "CAMERA_X");
  ok &= check(camera_x != nullptr, "scaling-negative test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent inverted = commandEvent(engine, "CAMERA_X", 25);
  ok &= check(invert->tweak(inverted), "negative-amplitude scaling should accept event");
  ok &= check(inverted.value == -25, "negative amplitude should invert axis direction");

  auto drift = makeMod<ScalingModifier>(
      R"(
name = "Drift"
type = "scaling"
applies_to = [ "CAMERA_X" ]
offset = -30
)",
      engine);
  DeviceEvent shifted = commandEvent(engine, "CAMERA_X", 10);
  ok &= check(drift->tweak(shifted), "offset scaling should accept event");
  ok &= check(shifted.value == -20, "negative offset should shift value downward");
  return ok;
}

static bool testScalingModifierInvertsVerticalCameraAxis() {
  MockEngine engine;
  auto mod = makeMod<ScalingModifier>(
      R"(
name = "Inverted"
type = "scaling"
applies_to = [ "CAMERA_Y" ]
amplitude = -1
)",
      engine);

  bool ok = true;
  auto camera_y = commandInput(engine, "CAMERA_Y");
  ok &= check(camera_y != nullptr, "vertical-camera scaling test input should exist");
  if (!camera_y) {
    return false;
  }

  DeviceEvent up = commandEvent(engine, "CAMERA_Y", -64);
  ok &= check(mod->tweak(up), "vertical-camera scaling should accept negative axis event");
  ok &= check(up.value == 64, "negative vertical camera input should invert to positive output");

  DeviceEvent down = commandEvent(engine, "CAMERA_Y", 64);
  ok &= check(mod->tweak(down), "vertical-camera scaling should accept positive axis event");
  ok &= check(down.value == -64, "positive vertical camera input should invert to negative output");

  return ok;
}

static bool testRepeatModifierForcesAndBlocksWhileOn() {
  MockEngine engine;
  auto mod = makeMod<RepeatModifier>(
      R"(
name = "Repeat Fire"
type = "repeat"
applies_to = [ "FIRE" ]
time_on = 0.001
time_off = 0.001
repeat = 1
cycle_delay = 0.01
force_on = [ 77 ]
block_while_busy = [ "MOVE_X" ]
)",
      engine);
  mod->_begin();

  auto fire = commandInput(engine, "FIRE");
  auto move = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(fire != nullptr && move != nullptr, "repeat test inputs should exist");
  if (!fire || !move) {
    return false;
  }

  usleep(2500);
  mod->_update(false);
  ok &= check(!engine.set_value_calls.empty(), "repeat should force command value on first ON phase");
  if (!engine.set_value_calls.empty()) {
    const auto& call = engine.set_value_calls.back();
    ok &= check(call.first == "FIRE" && call.second == 77,
                "repeat should set FIRE to configured force_on value");
  }

  DeviceEvent fire_evt = commandEvent(engine, "FIRE", 1);
  ok &= check(mod->tweak(fire_evt), "repeat should keep forced command events");
  ok &= check(fire_evt.value == 77, "repeat should override FIRE value while on");

  DeviceEvent blocked_evt = commandEvent(engine, "MOVE_X", 500);
  ok &= check(!mod->tweak(blocked_evt), "repeat should block configured block_while command while on");

  usleep(2500);
  mod->_update(false);
  ok &= check(sawName(engine.set_off_calls, "FIRE"),
              "repeat should turn FIRE off after time_on elapses");
  return ok;
}

static bool testRepeatModifierDefaultCycleAndRepeatReset() {
  MockEngine engine;
  auto mod = makeMod<RepeatModifier>(
      R"(
name = "Repeat Simple"
type = "repeat"
applies_to = [ "FIRE" ]
time_on = 0.001
time_off = 0.001
repeat = 2
cycle_delay = 0.003
)",
      engine);
  mod->_begin();

  bool ok = true;
  usleep(1500);
  mod->_update(false); // ON
  usleep(1500);
  mod->_update(false); // OFF (repeat_count 1)
  usleep(1500);
  mod->_update(false); // ON
  usleep(1500);
  mod->_update(false); // OFF (repeat_count 2)

  size_t on_before_reset = engine.set_on_calls.size();
  ok &= check(on_before_reset >= 2, "repeat should turn command on during each cycle");

  usleep(4000);
  mod->_update(false); // reset repeat_count after cycle_delay
  usleep(1500);
  mod->_update(false); // ON again after reset
  ok &= check(engine.set_on_calls.size() > on_before_reset,
              "repeat should restart ON/OFF cycling after cycle_delay reset");
  return ok;
}

static bool testRepeatModifierSupportsMultipleForceOnValues() {
  MockEngine engine;
  auto mod = makeMod<RepeatModifier>(
      R"(
name = "Repeat Multi Force"
type = "repeat"
applies_to = [ "FIRE", "MOVE_X" ]
time_on = 0.001
time_off = 0.001
force_on = [ 1, -128 ]
block_while_busy = [ "MOVE_X" ]
)",
      engine);
  mod->_begin();

  bool ok = true;
  usleep(2500);
  mod->_update(false); // ON
  ok &= check(engine.set_value_calls.size() >= 2,
              "repeat should apply force_on values for each configured command");

  bool saw_fire = false;
  bool saw_move = false;
  for (const auto& call : engine.set_value_calls) {
    if (call.first == "FIRE" && call.second == 1) {
      saw_fire = true;
    } else if (call.first == "MOVE_X" && call.second == -128) {
      saw_move = true;
    }
  }
  ok &= check(saw_fire && saw_move,
              "repeat should map force_on array entries to command order");
  return ok;
}

static bool testRepeatModifierClipsOutOfRangeAxisForceValues() {
  MockEngine engine;
  auto mod = makeMod<RepeatModifier>(
      R"(
name = "Repeat Axis Clip"
type = "repeat"
applies_to = [ "MOVE_X" ]
time_on = 0.001
time_off = 0.001
force_on = [ 128 ]
force_off = [ -999 ]
)",
      engine);
  mod->_begin();

  bool ok = true;
  usleep(2500);
  mod->_update(false); // ON
  ok &= check(!engine.set_value_calls.empty(),
              "repeat axis clip should set configured force_on value");
  if (!engine.set_value_calls.empty()) {
    ok &= check(engine.set_value_calls[0].first == "MOVE_X" &&
                    engine.set_value_calls[0].second == JOYSTICK_MAX,
                "repeat should clip out-of-range positive axis force_on to JOYSTICK_MAX");
  }

  DeviceEvent forced = commandEvent(engine, "MOVE_X", 0);
  ok &= check(mod->tweak(forced), "repeat axis clip tweak should accept target event");
  ok &= check(forced.value == JOYSTICK_MAX,
              "repeat tweak should enforce clipped force_on axis value");

  usleep(2500);
  mod->_update(false); // OFF
  ok &= check(engine.set_value_calls.size() >= 2,
              "repeat axis clip should set configured force_off value");
  if (engine.set_value_calls.size() >= 2) {
    ok &= check(engine.set_value_calls[1].first == "MOVE_X" &&
                    engine.set_value_calls[1].second == JOYSTICK_MIN,
                "repeat should clip out-of-range negative axis force_off to JOYSTICK_MIN");
  }
  return ok;
}

static bool testSequenceModifierTriggersAndBlocksDuringSequence() {
  MockEngine engine;
  auto mod = makeMod<SequenceModifier>(
      R"(
name = "Sequence Shoot Assist"
type = "sequence"
trigger = [ "FIRE" ]
block_while_busy = [ "CAMERA_X" ]
start_delay = 0.0
cycle_delay = 0.0
repeat_sequence = [ { event = "press", command = "FIRE" } ]
)",
      engine);
  mod->_begin();

  auto fire = commandInput(engine, "FIRE");
  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(fire != nullptr && camera_x != nullptr, "sequence test inputs should exist");
  if (!fire || !camera_x) {
    return false;
  }

  DeviceEvent trigger_evt = commandEvent(engine, "FIRE", 1);
  ok &= check(mod->tweak(trigger_evt), "trigger event should be accepted");

  mod->_update(false); // STARTING -> IN_SEQUENCE
  mod->_update(false); // IN_SEQUENCE send starts, should still be in sequence due event delay

  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 600);
  ok &= check(!mod->tweak(blocked), "sequence should block configured events while in sequence");

  usleep(12000);
  mod->_update(false); // sequence should be able to complete now
  DeviceEvent after = commandEvent(engine, "CAMERA_X", 600);
  ok &= check(mod->tweak(after), "blocked event should pass after sequence completion");
  return ok;
}

static bool testSequenceModifierLockAllBlocksAllSignals() {
  MockEngine engine;
  auto mod = makeMod<SequenceModifier>(
      R"(
name = "Sequence Lock All"
type = "sequence"
trigger = [ "FIRE" ]
block_while_busy = "ALL"
repeat_sequence = [ { event = "press", command = "FIRE" } ]
)",
      engine);
  mod->_begin();

  auto fire = commandInput(engine, "FIRE");
  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(fire != nullptr && camera_x != nullptr, "sequence-lock-all test inputs should exist");
  if (!fire || !camera_x) {
    return false;
  }

  DeviceEvent trigger_evt = commandEvent(engine, "FIRE", 1);
  ok &= check(mod->tweak(trigger_evt), "lock-all trigger event should be accepted");
  mod->_update(false);
  mod->_update(false);

  DeviceEvent camera_evt = commandEvent(engine, "CAMERA_X", 10);
  DeviceEvent fire_evt = commandEvent(engine, "FIRE", 1);
  ok &= check(!mod->tweak(camera_evt), "lock-all should block axis event while in sequence");
  ok &= check(!mod->tweak(fire_evt), "lock-all should block button event while in sequence");
  return ok;
}

static bool testSequenceModifierAutoStartsWithoutTrigger() {
  MockEngine engine;
  auto mod = makeMod<SequenceModifier>(
      R"(
name = "Sequence Auto Start"
type = "sequence"
block_while_busy = [ "CAMERA_X" ]
repeat_sequence = [ { event = "press", command = "FIRE" } ]
)",
      engine);
  mod->_begin();

  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(camera_x != nullptr, "sequence-auto test input should exist");
  if (!camera_x) {
    return false;
  }

  mod->_update(false); // UNTRIGGERED -> STARTING
  mod->_update(false); // STARTING -> IN_SEQUENCE
  mod->_update(false); // IN_SEQUENCE
  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 300);
  ok &= check(!mod->tweak(blocked),
              "sequence without trigger should auto-start and block configured signals");
  return ok;
}

static bool testSequenceParallelHandlesDelayBetweenEvents() {
  MockEngine engine;
  auto listen = commandInput(engine, "LISTEN");
  bool ok = true;
  ok &= check(listen != nullptr, "sequence-delay test input should exist");
  if (!listen) {
    return false;
  }

  Sequence seq(engine.controller);
  seq.addPress(listen, 1);
  seq.addDelay(2000);
  seq.addPress(listen, 1);

  ok &= check(!seq.sendParallel(0.0),
              "sequence with delay should remain pending at t=0");
  ok &= check(!seq.sendParallel(0.001),
              "sequence should remain pending before delay elapses");
  ok &= check(seq.sendParallel(0.003),
              "sequence should finish once delay has elapsed");
  ok &= check(!seq.sendParallel(0.0),
              "sequence should reset to start after completion");
  return ok;
}

static bool testSequenceModifierRetriggersImmediatelyWhenCycleDelayZero() {
  MockEngine engine;
  auto mod = makeMod<SequenceModifier>(
      R"(
name = "Sequence Retrigger Zero Delay"
type = "sequence"
trigger = [ "FIRE" ]
block_while_busy = [ "CAMERA_X" ]
start_delay = 0.0
cycle_delay = 0.0
repeat_sequence = [
  { event = "press", command = "FIRE", delay = 0.002 },
  { event = "press", command = "FIRE" }
]
)",
      engine);
  mod->_begin();

  auto fire = commandInput(engine, "FIRE");
  auto camera_x = commandInput(engine, "CAMERA_X");
  bool ok = true;
  ok &= check(fire != nullptr && camera_x != nullptr, "sequence-retrigger test inputs should exist");
  if (!fire || !camera_x) {
    return false;
  }

  DeviceEvent trigger_evt = commandEvent(engine, "FIRE", 1);
  ok &= check(mod->tweak(trigger_evt), "first trigger should be accepted");
  mod->_update(false); // STARTING -> IN_SEQUENCE
  mod->_update(false); // begin sequence

  usleep(12000);
  mod->_update(false); // sequence completes and should re-arm immediately (cycle_delay=0)

  DeviceEvent second_trigger = commandEvent(engine, "FIRE", 1);
  ok &= check(mod->tweak(second_trigger), "second trigger should be accepted immediately after completion");
  mod->_update(false); // STARTING -> IN_SEQUENCE again

  DeviceEvent blocked = commandEvent(engine, "CAMERA_X", 321);
  ok &= check(!mod->tweak(blocked),
              "second trigger should start a new sequence cycle that blocks configured commands");
  return ok;
}

static bool testMenuModifierAppliesAndRestoresMenuState() {
  MockEngine engine;
  auto menu_item = std::make_shared<MenuItem>(engine.menu_iface, "HUD", 0, 0, 0, false, true, true, false,
                                              false, nullptr, nullptr, nullptr, CounterAction::NONE);
  engine.menu_iface.items["HUD"] = menu_item;

  auto mod = makeMod<MenuModifier>(
      R"(
name = "Toggle HUD"
type = "menu"
menu_items = [ { entry = "HUD", value = 2 } ]
reset_on_finish = true
)",
      engine);

  bool ok = true;
  mod->begin();
  ok &= check(engine.menu_set_calls.size() == 1, "menu modifier begin should set one menu item");
  if (engine.menu_set_calls.size() == 1) {
    ok &= check(engine.menu_set_calls[0].first == "HUD" && engine.menu_set_calls[0].second == 2,
                "menu modifier should set HUD to configured value");
  }

  mod->finish();
  ok &= check(engine.menu_restore_calls.size() == 1, "menu modifier finish should restore one menu item");
  if (engine.menu_restore_calls.size() == 1) {
    ok &= check(engine.menu_restore_calls[0] == "HUD", "menu modifier should restore HUD");
  }
  return ok;
}

static bool testMenuModifierSupportsDefaultAndMultipleEntries() {
  MockEngine engine;
  auto hud = std::make_shared<MenuItem>(engine.menu_iface, "HUD", 0, 0, 0, false, true, true, false, false,
                                        nullptr, nullptr, nullptr, CounterAction::NONE);
  auto reticle = std::make_shared<MenuItem>(engine.menu_iface, "RETICLE", 0, 0, 1, false, true, true, false, false,
                                            nullptr, nullptr, nullptr, CounterAction::NONE);
  engine.menu_iface.items["HUD"] = hud;
  engine.menu_iface.items["RETICLE"] = reticle;

  auto mod = makeMod<MenuModifier>(
      R"(
name = "Menu Multi"
type = "menu"
menu_items = [ { entry = "HUD" }, { entry = "RETICLE", value = 2 } ]
)",
      engine);

  bool ok = true;
  mod->begin();
  ok &= check(engine.menu_set_calls.size() == 2,
              "menu modifier should apply each menu_items entry");
  bool saw_default = false;
  bool saw_explicit = false;
  for (const auto& call : engine.menu_set_calls) {
    if (call.first == "HUD" && call.second == 0) {
      saw_default = true;
    }
    if (call.first == "RETICLE" && call.second == 2) {
      saw_explicit = true;
    }
  }
  ok &= check(saw_default, "menu entry without value should default to 0");
  ok &= check(saw_explicit, "menu entry with value should use explicit value");
  return ok;
}

static bool testParentModifierForwardsLifecycleAndTweak() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child1 = makeMod<ProbeChildModifier>(
      R"(
name = "child1"
type = "probe_child"
delta = 1
)",
      engine);
  auto child2 = makeMod<ProbeChildModifier>(
      R"(
name = "child2"
type = "probe_child"
delta = 2
)",
      engine);

  engine.modifier_map["child1"] = child1;
  engine.modifier_map["child2"] = child2;

  auto parent = makeMod<ParentModifier>(
      R"(
name = "parent"
type = "parent"
children = [ "child1", "child2" ]
)",
      engine);

  bool ok = true;
  parent->begin();
  ok &= check(ProbeChildModifier::begin_calls["child1"] == 1 &&
                  ProbeChildModifier::begin_calls["child2"] == 1,
              "parent begin should invoke begin on all children");

  parent->update();
  ok &= check(ProbeChildModifier::update_calls["child1"] == 1 &&
                  ProbeChildModifier::update_calls["child2"] == 1,
              "parent update should invoke update on all children");

  auto camera_x = commandInput(engine, "CAMERA_X");
  ok &= check(camera_x != nullptr, "parent test input should exist");
  if (!camera_x) {
    return false;
  }
  DeviceEvent event = commandEvent(engine, "CAMERA_X", 10);
  ok &= check(parent->tweak(event), "parent tweak should return true when children do not block");
  ok &= check(event.value == 13, "parent tweak should apply child tweaks in sequence");

  parent->finish();
  ok &= check(ProbeChildModifier::finish_calls["child1"] == 1 &&
                  ProbeChildModifier::finish_calls["child2"] == 1,
              "parent finish should invoke finish on all children");
  return ok;
}

static bool testParentModifierRandomSelectionChoosesRequestedCount() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child1 = makeMod<ProbeChildModifier>(
      R"(
name = "rand_child1"
type = "probe_child"
)",
      engine);
  auto child2 = makeMod<ProbeChildModifier>(
      R"(
name = "rand_child2"
type = "probe_child"
)",
      engine);
  auto child3 = makeMod<ProbeChildModifier>(
      R"(
name = "rand_child3"
type = "probe_child"
)",
      engine);

  engine.modifier_map["rand_child1"] = child1;
  engine.modifier_map["rand_child2"] = child2;
  engine.modifier_map["rand_child3"] = child3;

  auto parent = makeMod<ParentModifier>(
      R"(
name = "random_parent"
type = "parent"
random = true
value = 2
)",
      engine);

  parent->begin();
  int begin_total = ProbeChildModifier::begin_calls["rand_child1"] +
                    ProbeChildModifier::begin_calls["rand_child2"] +
                    ProbeChildModifier::begin_calls["rand_child3"];
  bool ok = true;
  ok &= check(begin_total == 2,
              "random parent should begin exactly the requested number of children");

  parent->finish();
  int finish_total = ProbeChildModifier::finish_calls["rand_child1"] +
                     ProbeChildModifier::finish_calls["rand_child2"] +
                     ProbeChildModifier::finish_calls["rand_child3"];
  ok &= check(finish_total == 2,
              "random parent should finish exactly the selected children");
  return ok;
}

int main() {
  bool ok = true;
  ok &= testCooldownModifierBlocksAfterTrigger();
  ok &= testCooldownModifierRequiresWhileConditionWhenCumulativeStartType();
  ok &= testCooldownModifierCancelableResetsIfConditionDrops();
  ok &= testDelayModifierQueuesAndReplays();
  ok &= testDelayModifierAppliesToAllCommands();
  ok &= testDelayModifierReplaysMultipleSequentialEvents();
  ok &= testDelayModifierDelaysHybridButtonAndAxis();
  ok &= testDelayModifierClearsQueueAcrossLifecycle();
  ok &= testDelayModifierStressRapidDodgeEvents();
  ok &= testDelayModifierStressInterleavedJoystickEvents();
  ok &= testDisableDefaultFilterBlocksConfiguredCommandOnly();
  ok &= testDisableRespectsWhileCondition();
  ok &= testDisableRespectsDistanceMovementCondition();
  ok &= testDisableRespectsPersistentCondition();
  ok &= testDisableFilterAboveBlocksOnlyPositiveValues();
  ok &= testDisableFilterBelowBlocksOnlyNegativeValues();
  ok &= testDisableFilterDirectionsOnThreeStateSelectWeapon();
  ok &= testDisableBlocksAllConfiguredCommandsIncludingHybridAxis();
  ok &= testDisableNoAimingStyleBlocksHybridButtonAndAxis();
  ok &= testFormulaModifierOffsetsAndRestores();
  ok &= testFormulaModifierJankyHonorsWhileCondition();
  ok &= testFormulaModifierSupportsEightCurveType();
  ok &= testFormulaCircleAssignsSinThenCosByAxisOrder();
  ok &= testFormulaEightCurveAlternatesAcrossAxisPairs();
  ok &= testScalingModifierScalesAndClamps();
  ok &= testScalingModifierSupportsNegativeAmplitudeAndOffset();
  ok &= testScalingModifierInvertsVerticalCameraAxis();
  ok &= testRepeatModifierForcesAndBlocksWhileOn();
  ok &= testRepeatModifierDefaultCycleAndRepeatReset();
  ok &= testRepeatModifierSupportsMultipleForceOnValues();
  ok &= testRepeatModifierClipsOutOfRangeAxisForceValues();
  ok &= testSequenceModifierTriggersAndBlocksDuringSequence();
  ok &= testSequenceModifierLockAllBlocksAllSignals();
  ok &= testSequenceModifierAutoStartsWithoutTrigger();
  ok &= testSequenceParallelHandlesDelayBetweenEvents();
  ok &= testSequenceModifierRetriggersImmediatelyWhenCycleDelayZero();
  ok &= testMenuModifierAppliesAndRestoresMenuState();
  ok &= testMenuModifierSupportsDefaultAndMultipleEntries();
  ok &= testParentModifierForwardsLifecycleAndTweak();
  ok &= testParentModifierRandomSelectionChoosesRequestedCount();

  if (!ok) {
    return 1;
  }
  std::cout << "PASS: modifier type regression tests\n";
  return 0;
}
