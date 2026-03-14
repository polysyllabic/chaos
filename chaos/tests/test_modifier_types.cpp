#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <functional>
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
#include <Game.hpp>
#include <MenuInterface.hpp>
#include <MenuItem.hpp>
#include <MenuModifier.hpp>
#include <RemapModifier.hpp>
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

static std::string makeHeader(const std::string& name, const std::string& type) {
  return "name = \"" + name + "\"\n" + "type = \"" + type + "\"\n";
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

static bool testGameDefaultsToNoErrorsBeforeLoad() {
  Controller controller;
  Game game(controller);
  bool ok = true;
  ok &= check(game.getErrors() == 0, "new Game should start with zero parse errors before any load");
  return ok;
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

static bool testCooldownRestoresLatestBlockedCommandStateOnExpiry() {
  MockEngine engine;
  auto mod = makeMod<CooldownModifier>(
      R"(
name = "Cooldown Restore Held Sprint"
type = "cooldown"
applies_to = [ "DODGE" ]
while = [ "movement", "sprint" ]
start_type = "cumulative"
time_on = 0.002
time_off = 0.004
)",
      engine);
  mod->_begin();

  auto dodge = commandInput(engine, "DODGE");
  auto move_x = commandInput(engine, "MOVE_X");
  auto move_y = commandInput(engine, "MOVE_Y");
  bool ok = true;
  ok &= check(dodge != nullptr && move_x != nullptr && move_y != nullptr,
              "cooldown restore-held test inputs should exist");
  if (!dodge || !move_x || !move_y) {
    return false;
  }

  // Hold sprint + movement before cooldown starts.
  engine.applyEvent(commandEvent(engine, "DODGE", 1));
  engine.applyEvent(commandEvent(engine, "MOVE_X", JOYSTICK_MAX));
  engine.applyEvent(commandEvent(engine, "MOVE_Y", 0));

  mod->_update(false); // UNTRIGGERED -> ALLOW
  usleep(6000);
  mod->_update(false); // ALLOW -> BLOCK
  if (!sawName(engine.set_off_calls, "DODGE")) {
    usleep(6000);
    mod->_update(false);
  }
  ok &= check(sawName(engine.set_off_calls, "DODGE"),
              "cooldown should disable DODGE when block starts");
  ok &= check(engine.getState(dodge->getID(), dodge->getButtonType()) == 0,
              "DODGE should be forced off during cooldown block");

  // Simulate held sprint reports while blocked; these should be restored on expiry.
  DeviceEvent held_dodge = commandEvent(engine, "DODGE", 1);
  ok &= check(!mod->tweak(held_dodge),
              "held DODGE reports should be blocked during cooldown");

  usleep(7000);
  mod->_update(false); // BLOCK -> UNTRIGGERED; should restore latest blocked value
  ok &= check(engine.getState(dodge->getID(), dodge->getButtonType()) == 1,
              "DODGE should restore to the latest blocked held value on cooldown expiry");
  return ok;
}

static bool testCooldownCancelableHybridReleaseDoesNotRePressOnExpiry() {
  MockEngine engine;
  auto mod = makeMod<CooldownModifier>(
      R"(
name = "Cooldown Cancelable Snapshot Regression"
type = "cooldown"
applies_to = [ "AIM" ]
while = [ "aiming" ]
start_type = "cancelable"
time_on = 0.002
time_off = 0.004
)",
      engine);
  mod->_begin();

  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(aim != nullptr, "cancelable-hybrid restore test input should exist");
  if (!aim) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false); // UNTRIGGERED -> ALLOW
  usleep(6000);
  mod->_update(false); // ALLOW -> BLOCK

  ok &= check(sawName(engine.set_off_calls, "AIM"),
              "cancelable-hybrid restore test should enter cooldown block");
  ok &= check(engine.getState(aim->getID(), aim->getButtonType()) == 0,
              "AIM should be forced off during cooldown block");

  DeviceEvent aim_axis_release = commandHybridAxisEvent(engine, "AIM", JOYSTICK_MIN);
  ok &= check(!mod->tweak(aim_axis_release),
              "hybrid release axis report should be blocked during cooldown");

  usleep(7000);
  mod->_update(false); // BLOCK -> UNTRIGGERED; should remain released
  ok &= check(engine.getState(aim->getID(), aim->getButtonType()) == 0,
              "AIM should remain released after cooldown expiry");
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

static bool testDelayModifierClearPendingInjectedEvents() {
  MockEngine engine;
  auto mod = makeMod<DelayModifier>(
      R"(
name = "Delay Queue Manual Flush"
type = "delay"
applies_to = [ "CAMERA_X" ]
delay = 0.005
)",
      engine);

  bool ok = true;
  mod->_begin();
  DeviceEvent delayed = commandEvent(engine, "CAMERA_X", 99);
  ok &= check(!mod->tweak(delayed), "manual flush test should queue delayed event");

  std::size_t cleared = mod->clearPendingInjectedEvents();
  ok &= check(cleared == 1, "clearPendingInjectedEvents should report one discarded delayed event");

  usleep(7000);
  mod->_update(false);
  ok &= check(engine.pipelined_events.empty(),
              "cleared delayed events should not be replayed later");
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

static bool testDisableWhileTransitionClampsHeldAxes() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Only Aim Movement Transition Clamp"
type = "disable"
applies_to = [ "MOVE_Y", "MOVE_X" ]
while = [ "not_aiming" ]
)",
      engine);

  auto move_y = commandInput(engine, "MOVE_Y");
  auto move_x = commandInput(engine, "MOVE_X");
  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(move_y != nullptr && move_x != nullptr && aim != nullptr,
              "disable transition-clamp test inputs should exist");
  if (!move_y || !move_x || !aim) {
    return false;
  }

  // Start in aiming mode so the disable condition is initially inactive.
  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_begin();

  DeviceEvent moving_while_aiming = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(mod->tweak(moving_while_aiming), "MOVE_Y while aiming should pass through");
  ok &= check(moving_while_aiming.value == 900, "disable should be inactive while aiming");
  engine.applyEvent(moving_while_aiming);
  ok &= check(engine.getState(move_y->getID(), move_y->getButtonType()) == 900,
              "MOVE_Y should remain held while aiming");

  // Releasing aim activates the disable condition while MOVE_Y is still held.
  engine.applyEvent(commandEvent(engine, "AIM", 0));
  mod->_update(false);
  ok &= check(engine.getState(move_y->getID(), move_y->getButtonType()) == 0,
              "condition activation should clamp held MOVE_Y to neutral");

  DeviceEvent move_after_release = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(mod->tweak(move_after_release), "MOVE_Y after aiming release should be accepted");
  ok &= check(move_after_release.value == 0,
              "disable should continue to block MOVE_Y while not aiming");
  return ok;
}

static bool testDisableWhileTransitionRestoresHeldAxesWhenConditionClears() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Only Aim Movement Immediate Restore"
type = "disable"
applies_to = [ "MOVE_Y", "MOVE_X" ]
while = [ "not_aiming" ]
)",
      engine);

  auto move_y = commandInput(engine, "MOVE_Y");
  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(move_y != nullptr && aim != nullptr,
              "disable transition-restore test inputs should exist");
  if (!move_y || !aim) {
    return false;
  }

  mod->_begin();

  // While not aiming, movement input is blocked.
  DeviceEvent blocked_move = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(mod->tweak(blocked_move), "MOVE_Y while not aiming should pass through tweak");
  ok &= check(blocked_move.value == 0, "MOVE_Y should be blocked while not aiming");
  engine.applyEvent(blocked_move);
  ok &= check(engine.getState(move_y->getID(), move_y->getButtonType()) == 0,
              "blocked MOVE_Y should keep controller movement neutral");

  // Pressing aim should immediately restore held movement without requiring stick recenter.
  DeviceEvent aim_press = commandEvent(engine, "AIM", 1);
  ok &= check(mod->tweak(aim_press), "AIM press should pass through tweak");
  engine.applyEvent(aim_press);

  ok &= check(engine.getState(move_y->getID(), move_y->getButtonType()) == 900,
              "AIM press should immediately restore held MOVE_Y value");
  return ok;
}

static bool testDisableWhileHybridReleaseDoesNotRePressOnConditionClear() {
  MockEngine engine;
  auto mod = makeMod<DisableModifier>(
      R"(
name = "Disable Hybrid Release Restore Regression"
type = "disable"
applies_to = [ "AIM" ]
while = [ "movement" ]
)",
      engine);

  auto aim = commandInput(engine, "AIM");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(aim != nullptr && move_x != nullptr,
              "disable hybrid-release regression test inputs should exist");
  if (!aim || !move_x) {
    return false;
  }

  // Enter while-condition first so disable is active.
  engine.applyEvent(commandEvent(engine, "MOVE_X", JOYSTICK_MAX));
  mod->_begin();

  DeviceEvent aim_press_axis = commandHybridAxisEvent(engine, "AIM", JOYSTICK_MAX);
  ok &= check(mod->tweak(aim_press_axis),
              "AIM press axis should pass through tweak while disabled");
  engine.applyEvent(aim_press_axis);

  // User releases aim while still blocked.
  DeviceEvent aim_release_axis = commandHybridAxisEvent(engine, "AIM", JOYSTICK_MIN);
  ok &= check(mod->tweak(aim_release_axis),
              "AIM release axis should pass through tweak while disabled");
  engine.applyEvent(aim_release_axis);

  // Clearing while-condition should restore AIM to released state, not re-press it.
  DeviceEvent stop_moving = commandEvent(engine, "MOVE_X", 0);
  ok &= check(mod->tweak(stop_moving), "MOVE_X stop should pass through tweak");
  engine.applyEvent(stop_moving);

  ok &= check(engine.getState(aim->getID(), aim->getButtonType()) == 0,
              "AIM button should remain released after disable condition clears");
  ok &= check(engine.getState(aim->getHybridAxis(), TYPE_AXIS) == JOYSTICK_MIN,
              "AIM axis should remain released after disable condition clears");
  return ok;
}

static bool testOnlyAimMovementWorksWithControllerMirrorRemap() {
  MockEngine engine;
  auto mirror = makeMod<RemapModifier>(
      R"(
name = "Mirror Aim Trigger"
type = "remap"
remap = [
  { from = "R2", to = "L2" },
  { from = "L2", to = "R2" }
]
)",
      engine);
  auto only_aim_movement = makeMod<DisableModifier>(
      R"(
name = "Only Aim Movement"
type = "disable"
applies_to = [ "MOVE_Y", "MOVE_X" ]
while = [ "not_aiming" ]
)",
      engine);

  auto move_y = commandInput(engine, "MOVE_Y");
  auto aim = commandInput(engine, "AIM");
  auto r2 = engine.getInput("R2");
  bool ok = true;
  ok &= check(move_y != nullptr && aim != nullptr && r2 != nullptr,
              "mirror + only-aim test inputs should exist");
  if (!move_y || !aim || !r2) {
    return false;
  }

  mirror->_begin();
  only_aim_movement->_begin();

  auto run_pipeline = [&](DeviceEvent& event) {
    bool valid = mirror->remap(event);
    if (valid) {
      valid = only_aim_movement->tweak(event);
    }
    if (valid) {
      engine.applyEvent(event);
    }
    return valid;
  };

  DeviceEvent move_without_aim = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(run_pipeline(move_without_aim), "MOVE_Y without aim should pass through pipeline");
  ok &= check(move_without_aim.value == 0,
              "without aim, only-aim movement should block movement");

  DeviceEvent r2_button_press = {0, 1, TYPE_BUTTON, r2->getID()};
  ok &= check(run_pipeline(r2_button_press), "R2 button press should pass through pipeline");
  ok &= check(r2_button_press.type == TYPE_BUTTON && r2_button_press.id == aim->getID(),
              "R2 button should remap to AIM button");

  DeviceEvent r2_axis_press = {0, JOYSTICK_MAX, TYPE_AXIS, r2->getHybridAxis()};
  ok &= check(run_pipeline(r2_axis_press), "R2 axis press should pass through pipeline");
  ok &= check(r2_axis_press.type == TYPE_AXIS && r2_axis_press.id == aim->getHybridAxis(),
              "R2 axis should remap to AIM axis");

  DeviceEvent move_with_aim = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(run_pipeline(move_with_aim), "MOVE_Y with mirrored aim should pass through pipeline");
  ok &= check(move_with_aim.value == 900,
              "with mirrored aim active, only-aim movement should allow movement");

  DeviceEvent r2_button_release = {0, 0, TYPE_BUTTON, r2->getID()};
  ok &= check(run_pipeline(r2_button_release), "R2 button release should pass through pipeline");

  DeviceEvent r2_axis_release = {0, JOYSTICK_MIN, TYPE_AXIS, r2->getHybridAxis()};
  ok &= check(run_pipeline(r2_axis_release), "R2 axis release should pass through pipeline");
  ok &= check(engine.getState(aim->getID(), aim->getButtonType()) == 0,
              "after mirrored trigger release, AIM button state should return to 0");

  DeviceEvent move_after_release = commandEvent(engine, "MOVE_Y", 900);
  ok &= check(run_pipeline(move_after_release), "MOVE_Y after mirrored aim release should pass pipeline");
  ok &= check(move_after_release.value == 0,
              "after mirrored aim release, only-aim movement should block movement again");

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

static bool testFormulaModifierRestoresOnWhileConditionClear() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Restore On While Clear"
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
  ok &= check(camera_x != nullptr && aim != nullptr,
              "formula restore-on-clear test inputs should exist");
  if (!camera_x || !aim) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "CAMERA_X", 40));
  mod->_begin();

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);
  ok &= check(!engine.pipelined_events.empty(),
              "formula should emit while-condition output while active");
  const std::size_t active_count = engine.pipelined_events.size();
  if (active_count == 0) {
    return false;
  }

  engine.applyEvent(commandEvent(engine, "AIM", 0));
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == active_count + 1,
              "formula should emit one restore event when while-condition clears");
  if (engine.pipelined_events.size() != active_count + 1) {
    return false;
  }

  const auto& restore = engine.pipelined_events.back();
  ok &= check(restore.id == camera_x->getID() && restore.value == 40,
              "formula restore event should return CAMERA_X to latest baseline value");

  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == active_count + 1,
              "formula should not repeatedly emit restore while while-condition remains false");
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

static bool testFormulaRandomOffsetAppliesConstantRangeWithoutDirection() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Random Offset Flat"
type = "formula"
applies_to = [ "CAMERA_X", "MOVE_X" ]
formula_type = "random_offset"
range = [ 12 ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto move_x = commandInput(engine, "MOVE_X");
  bool ok = true;
  ok &= check(camera_x != nullptr && move_x != nullptr, "random_offset flat test inputs should exist");
  if (!camera_x || !move_x) {
    return false;
  }

  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "random_offset should emit one event per configured command");
  if (engine.pipelined_events.size() < 2) {
    return false;
  }

  short first_cam = 0;
  short first_move = 0;
  bool saw_cam = false;
  bool saw_move = false;
  for (const auto& ev : engine.pipelined_events) {
    if (ev.id == camera_x->getID()) {
      first_cam = ev.value;
      saw_cam = true;
    } else if (ev.id == move_x->getID()) {
      first_move = ev.value;
      saw_move = true;
    }
  }
  ok &= check(saw_cam && saw_move, "random_offset should emit both configured axis events");
  ok &= check(first_cam == 12 && first_move == 12,
              "random_offset without direction should apply the same fixed offset to all commands");

  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 4,
              "random_offset should continue emitting fixed offsets across updates");
  if (engine.pipelined_events.size() < 4) {
    return false;
  }

  short second_cam = 0;
  short second_move = 0;
  bool saw_second_cam = false;
  bool saw_second_move = false;
  for (size_t i = 2; i < engine.pipelined_events.size(); ++i) {
    const auto& ev = engine.pipelined_events[i];
    if (ev.id == camera_x->getID()) {
      second_cam = ev.value;
      saw_second_cam = true;
    } else if (ev.id == move_x->getID()) {
      second_move = ev.value;
      saw_second_move = true;
    }
  }
  ok &= check(saw_second_cam && saw_second_move,
              "random_offset second update should emit both configured axis events");
  ok &= check(second_cam == 12 && second_move == 12,
              "random_offset should keep the same fixed offset for the full mod lifetime");
  return ok;
}

static bool testFormulaRandomOffsetDirectionDecomposesPairs() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Random Offset Direction"
type = "formula"
applies_to = [ "CAMERA_X", "CAMERA_Y", "MOVE_X", "MOVE_Y" ]
formula_type = "random_offset"
range = [ 20 ]
direction = [ 90 ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto camera_y = commandInput(engine, "CAMERA_Y");
  auto move_x = commandInput(engine, "MOVE_X");
  auto move_y = commandInput(engine, "MOVE_Y");
  bool ok = true;
  ok &= check(camera_x != nullptr && camera_y != nullptr && move_x != nullptr && move_y != nullptr,
              "random_offset direction test inputs should exist");
  if (!camera_x || !camera_y || !move_x || !move_y) {
    return false;
  }

  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 4,
              "random_offset direction should emit one event per configured command");
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
              "random_offset direction should emit all configured axis events");

  auto magnitude = [](short value) { return (value < 0) ? -value : value; };
  ok &= check(magnitude(cam_x) <= 1 && magnitude(mov_x) <= 1,
              "direction 90 should produce near-zero x components");
  ok &= check(cam_y >= 19 && mov_y >= 19,
              "direction 90 should route offset magnitude to y components");
  return ok;
}

static bool testFormulaRandomOffsetHonorsWhileAndRetainsFixedOffset() {
  MockEngine engine;
  auto mod = makeMod<FormulaModifier>(
      R"(
name = "Formula Random Offset While"
type = "formula"
applies_to = [ "CAMERA_X" ]
formula_type = "random_offset"
range = [ 15 ]
while = [ "aiming" ]
)",
      engine);

  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  bool ok = true;
  ok &= check(camera_x != nullptr && aim != nullptr, "random_offset while test inputs should exist");
  if (!camera_x || !aim) {
    return false;
  }

  mod->_begin();
  mod->_update(false);
  ok &= check(engine.pipelined_events.empty(),
              "random_offset should not emit events while while-condition is false");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 1,
              "random_offset should emit when while-condition becomes true");
  if (engine.pipelined_events.size() < 1) {
    return false;
  }
  const short first_val = engine.pipelined_events[0].value;
  ok &= check(first_val == 15, "random_offset should apply configured fixed range value");

  engine.applyEvent(commandEvent(engine, "AIM", 0));
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 2,
              "random_offset should emit one restore event when while-condition becomes false");
  if (engine.pipelined_events.size() < 2) {
    return false;
  }
  ok &= check(engine.pipelined_events[1].value == 0,
              "random_offset restore event should return command to baseline value");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  mod->_update(false);
  ok &= check(engine.pipelined_events.size() == 3,
              "random_offset should emit again when while-condition becomes true again");
  if (engine.pipelined_events.size() < 3) {
    return false;
  }
  ok &= check(engine.pipelined_events[2].value == first_val,
              "random_offset should retain the same fixed offset across while-condition toggles");
  return ok;
}

static bool testRemapModifierRespectsToNegForThreeStateFromDPad() {
  MockEngine engine;
  auto mod = makeMod<RemapModifier>(
      R"(
name = "Remap to_neg ThreeState"
type = "remap"
remap = [
      {from = "DX", to = "CIRCLE", to_neg = "SQUARE"},
      {from = "DY", to = "TRIANGLE", to_neg = "X"}
]
)",
      engine);
  auto circle = commandInput(engine, "CROUCH");
  auto square = commandInput(engine, "MELEE");
  auto triangle = commandInput(engine, "GRAB");
  auto cross = commandInput(engine, "JUMP");
  auto dpad_x = commandInput(engine, "GET_GUN");
  auto dpad_y = commandInput(engine, "GET_CONSUMABLE");
  bool ok = true;
  ok &= check(circle != nullptr && square != nullptr && triangle != nullptr &&
              cross != nullptr && dpad_x != nullptr && dpad_y != nullptr,
              "three-state remap test inputs should exist");
  if (!circle || !square || !triangle || !cross || !dpad_x || !dpad_y) {
    return false;
  }

  mod->_begin();

  DeviceEvent dx_right = commandEvent(engine, "GET_GUN", 1);
  ok &= check(mod->remap(dx_right), "DX positive event should pass through");
  ok &= check(dx_right.id == circle->getID() && dx_right.value == 1,
              "positive DX should remap to configured 'to'");

  DeviceEvent dx_left = commandEvent(engine, "GET_GUN", -1);
  ok &= check(mod->remap(dx_left), "DX negative event should pass through");
  ok &= check(dx_left.id == square->getID() && dx_left.value == 1,
              "negative DX should remap to configured 'to_neg'");

  DeviceEvent dy_up = commandEvent(engine, "GET_CONSUMABLE", 1);
  ok &= check(mod->remap(dy_up), "DY positive event should pass through");
  ok &= check(dy_up.id == triangle->getID() && dy_up.value == 1,
              "positive DY should remap to configured 'to'");

  DeviceEvent dy_down = commandEvent(engine, "GET_CONSUMABLE", -1);
  ok &= check(mod->remap(dy_down), "DY negative event should pass through");
  ok &= check(dy_down.id == cross->getID() && dy_down.value == 1,
              "negative DY should remap to configured 'to_neg'");

  return ok;
}

static bool testRemapModifierClearsPreviousThreeStateDirection() {
  MockEngine engine;
  auto mod = makeMod<RemapModifier>(
      R"(
name = "Remap three-state release"
type = "remap"
remap = [
      {from = "DX", to = "CIRCLE", to_neg = "SQUARE"}
]
)",
      engine);
  auto circle = commandInput(engine, "CROUCH");
  auto square = commandInput(engine, "MELEE");
  auto dpad_x = commandInput(engine, "GET_GUN");
  bool ok = true;
  ok &= check(circle != nullptr && square != nullptr && dpad_x != nullptr,
              "three-state release test inputs should exist");
  if (!circle || !square || !dpad_x) {
    return false;
  }

  mod->_begin();

  DeviceEvent left = commandEvent(engine, "GET_GUN", -1);
  ok &= check(mod->remap(left), "negative three-state event should pass through");
  engine.applyEvent(left);
  ok &= check(engine.getState(square->getID(), square->getButtonType()) == 1 &&
              engine.getState(circle->getID(), circle->getButtonType()) == 0,
              "negative three-state remap should press negative target");

  DeviceEvent right = commandEvent(engine, "GET_GUN", 1);
  ok &= check(mod->remap(right), "positive three-state event should pass through");
  engine.applyEvent(right);
  ok &= check(engine.getState(circle->getID(), circle->getButtonType()) == 1,
              "positive three-state remap should press positive target");
  ok &= check(engine.getState(square->getID(), square->getButtonType()) == 0,
              "positive three-state remap should release negative target");

  DeviceEvent neutral = commandEvent(engine, "GET_GUN", 0);
  ok &= check(mod->remap(neutral), "neutral three-state event should pass through");
  engine.applyEvent(neutral);
  ok &= check(engine.getState(square->getID(), square->getButtonType()) == 0 &&
              engine.getState(circle->getID(), circle->getButtonType()) == 0,
              "neutral three-state remap should clear pressed target");

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

static bool testScalingModifierUsesWhileAmplitudeWhenConditionTrue() {
  MockEngine engine;
  auto mod = makeMod<ScalingModifier>(
      R"(
name = "Scoped Camera"
type = "scaling"
applies_to = [ "CAMERA_X" ]
amplitude = 1.0
while = [ "aiming" ]
while_amplitude = 0.5
)",
      engine);

  bool ok = true;
  auto camera_x = commandInput(engine, "CAMERA_X");
  auto aim = commandInput(engine, "AIM");
  ok &= check(camera_x != nullptr && aim != nullptr,
              "conditional scaling test inputs should exist");
  if (!camera_x || !aim) {
    return false;
  }

  DeviceEvent hip_fire = commandEvent(engine, "CAMERA_X", 60);
  ok &= check(mod->tweak(hip_fire), "conditional scaling should accept default event");
  ok &= check(hip_fire.value == 60,
              "default amplitude should apply when while condition is false");

  engine.applyEvent(commandEvent(engine, "AIM", 1));
  DeviceEvent aimed = commandEvent(engine, "CAMERA_X", 60);
  ok &= check(mod->tweak(aimed), "conditional scaling should accept aimed event");
  ok &= check(aimed.value == 30,
              "while_amplitude should apply when while condition is true");

  return ok;
}

static bool testScalingModifierIgnoresWhileAmplitudeWithoutWhileCondition() {
  MockEngine engine;
  auto mod = makeMod<ScalingModifier>(
      R"(
name = "Unconditional Camera"
type = "scaling"
applies_to = [ "CAMERA_X" ]
amplitude = 1.0
while_amplitude = 0.5
)",
      engine);

  bool ok = true;
  auto camera_x = commandInput(engine, "CAMERA_X");
  ok &= check(camera_x != nullptr, "unconditional scaling test input should exist");
  if (!camera_x) {
    return false;
  }

  DeviceEvent event = commandEvent(engine, "CAMERA_X", 60);
  ok &= check(mod->tweak(event), "scaling without while should accept event");
  ok &= check(event.value == 60,
              "while_amplitude should not apply without a while condition");

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

static bool testMenuModifiersDoNotBlockLaterDisableModifier() {
  MockEngine engine;
  auto menu_a = std::make_shared<MenuItem>(engine.menu_iface, "FX_A", 0, 0, 0, false, true, true, false, false,
                                           nullptr, nullptr, nullptr, CounterAction::NONE);
  auto menu_b = std::make_shared<MenuItem>(engine.menu_iface, "FX_B", 0, 0, 0, false, true, true, false, false,
                                           nullptr, nullptr, nullptr, CounterAction::NONE);
  engine.menu_iface.items["FX_A"] = menu_a;
  engine.menu_iface.items["FX_B"] = menu_b;

  auto menu1 = makeMod<MenuModifier>(
      R"(
name = "Demons"
type = "menu"
menu_items = [ { entry = "FX_A", value = 1 } ]
)",
      engine);
  auto menu2 = makeMod<MenuModifier>(
      R"(
name = "Desert Fog"
type = "menu"
menu_items = [ { entry = "FX_B", value = 2 } ]
)",
      engine);
  auto disable = makeMod<DisableModifier>(
      R"(
name = "Disable Camera"
type = "disable"
applies_to = [ "CAMERA_Y" ]
)",
      engine);

  menu1->_begin();
  menu2->_begin();
  disable->_begin();

  DeviceEvent camera_evt = commandEvent(engine, "CAMERA_Y", 777);
  bool ok = true;
  bool valid = true;
  std::vector<std::shared_ptr<Modifier>> chain{menu1, menu2, disable};
  for (auto& mod : chain) {
    valid = mod->remap(camera_evt);
    if (!valid) {
      break;
    }
  }
  if (valid) {
    for (auto& mod : chain) {
      valid = mod->tweak(camera_evt);
      if (!valid) {
        break;
      }
    }
  }

  ok &= check(valid, "menu modifiers should not block normal gameplay events while idle");
  ok &= check(camera_evt.value == 0, "disable modifier should still apply after active menu modifiers");
  return ok;
}

static bool testAllDifferingModifierPairOrdersStayStableWhenIdle() {
  struct ModifierFactory {
    std::string label;
    std::function<std::shared_ptr<Modifier>(MockEngine&, int)> create;
  };

  const std::vector<ModifierFactory> factories{
      {"cooldown", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_cooldown_" + std::to_string(idx), "cooldown") +
                            "applies_to = [ \"FIRE\" ]\n"
                            "trigger = [ \"FIRE\" ]\n"
                            "time_on = 0.01\n"
                            "time_off = 0.01\n";
         return makeMod<CooldownModifier>(toml, engine);
       }},
      {"delay", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_delay_" + std::to_string(idx), "delay") +
                            "applies_to = [ \"FIRE\" ]\n"
                            "delay = 0.01\n";
         return makeMod<DelayModifier>(toml, engine);
       }},
      {"disable", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_disable_" + std::to_string(idx), "disable") +
                            "applies_to = [ \"FIRE\" ]\n";
         return makeMod<DisableModifier>(toml, engine);
       }},
      {"formula", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_formula_" + std::to_string(idx), "formula") +
                            "applies_to = [ \"FIRE\" ]\n"
                            "formula_type = \"janky\"\n"
                            "amplitude = 0.2\n"
                            "period_length = 1.0\n";
         return makeMod<FormulaModifier>(toml, engine);
       }},
      {"menu", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_menu_" + std::to_string(idx), "menu") +
                            "menu_items = [ { entry = \"PAIR_MENU\", value = 1 } ]\n";
         return makeMod<MenuModifier>(toml, engine);
       }},
      {"remap", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_remap_" + std::to_string(idx), "remap") +
                            "remap = [ { from = \"LX\", to = \"LY\" } ]\n";
         return makeMod<RemapModifier>(toml, engine);
       }},
      {"random_remap", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_random_remap_" + std::to_string(idx), "remap") +
                            "random_remap = [ \"X\", \"SQUARE\" ]\n";
         return makeMod<RemapModifier>(toml, engine);
       }},
      {"repeat", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_repeat_" + std::to_string(idx), "repeat") +
                            "applies_to = [ \"FIRE\" ]\n"
                            "time_on = 0.01\n"
                            "time_off = 0.01\n";
         return makeMod<RepeatModifier>(toml, engine);
       }},
      {"scaling", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_scaling_" + std::to_string(idx), "scaling") +
                            "applies_to = [ \"FIRE\" ]\n"
                            "amplitude = 2.0\n";
         return makeMod<ScalingModifier>(toml, engine);
       }},
      {"sequence", [](MockEngine& engine, int idx) {
         std::string toml = makeHeader("pair_sequence_" + std::to_string(idx), "sequence") +
                            "trigger = [ \"FIRE\" ]\n"
                            "block_while_busy = [ \"MOVE_X\" ]\n"
                            "repeat_sequence = [ { event = \"press\", command = \"FIRE\" } ]\n";
         return makeMod<SequenceModifier>(toml, engine);
       }},
  };

  bool ok = true;
  int case_id = 0;
  for (size_t i = 0; i < factories.size(); ++i) {
    for (size_t j = 0; j < factories.size(); ++j) {
      if (i == j) {
        continue;
      }
      ++case_id;
      MockEngine engine;
      auto menu_item = std::make_shared<MenuItem>(engine.menu_iface, "PAIR_MENU", 0, 0, 0, false, true, true,
                                                  false, false, nullptr, nullptr, nullptr, CounterAction::NONE);
      engine.menu_iface.items["PAIR_MENU"] = menu_item;

      auto first = factories[i].create(engine, case_id * 2);
      auto second = factories[j].create(engine, case_id * 2 + 1);
      first->_begin();
      second->_begin();

      DeviceEvent event = commandEvent(engine, "CAMERA_X", 321);
      DeviceEvent original = event;
      bool valid = true;

      valid = first->remap(event);
      if (valid) {
        valid = second->remap(event);
      }
      if (valid) {
        valid = first->tweak(event);
      }
      if (valid) {
        valid = second->tweak(event);
      }

      const std::string pair_label = factories[i].label + " -> " + factories[j].label;
      ok &= check(valid, "pair order should not block unrelated CAMERA_X event while idle: " + pair_label);
      ok &= check(event.id == original.id && event.type == original.type && event.value == original.value,
                  "pair order should not mutate unrelated CAMERA_X event while idle: " + pair_label);
    }
  }
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

static bool testParentModifierForwardsRemapIntoLaterChildTweaks() {
  MockEngine engine;

  auto remap_child = makeMod<RemapModifier>(
      R"(
name = "child_remap"
type = "remap"
remap = [ { from = "RX", to = "LX" } ]
)",
      engine);
  auto scale_child = makeMod<ScalingModifier>(
      R"(
name = "child_scale"
type = "scaling"
applies_to = [ "MOVE_X" ]
amplitude = 0.5
)",
      engine);

  engine.modifier_map["child_remap"] = remap_child;
  engine.modifier_map["child_scale"] = scale_child;

  auto parent = makeMod<ParentModifier>(
      R"(
name = "parent"
type = "parent"
children = [ "child_remap", "child_scale" ]
)",
      engine);

  bool ok = true;
  parent->begin();

  DeviceEvent event = commandEvent(engine, "CAMERA_X", 60);
  ok &= check(parent->remap(event), "parent remap should accept remapped child event");
  auto move_x = commandInput(engine, "MOVE_X");
  ok &= check(move_x != nullptr, "parent remap forwarding test input should exist");
  if (!move_x) {
    return false;
  }

  ok &= check(event.id == move_x->getID() && event.type == move_x->getButtonType(),
              "parent remap should forward event through child remap before tweaks");
  ok &= check(parent->tweak(event), "parent tweak should accept remapped child event");
  ok &= check(event.value == 30,
              "later child tweak should see remapped signal and scale its value");

  parent->finish();
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

static bool testParentModifierRandomSelectFromRestrictsPool() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child1 = makeMod<ProbeChildModifier>(
      R"(
name = "select_from_child1"
type = "probe_child"
)",
      engine);
  auto child2 = makeMod<ProbeChildModifier>(
      R"(
name = "select_from_child2"
type = "probe_child"
)",
      engine);
  auto child3 = makeMod<ProbeChildModifier>(
      R"(
name = "select_from_child3"
type = "probe_child"
)",
      engine);

  engine.modifier_map["select_from_child1"] = child1;
  engine.modifier_map["select_from_child2"] = child2;
  engine.modifier_map["select_from_child3"] = child3;

  auto parent = makeMod<ParentModifier>(
      R"(
name = "select_from_parent"
type = "parent"
random = true
value = 2
select_from = [ "select_from_child1", "select_from_child2" ]
)",
      engine);

  parent->begin();
  bool ok = true;
  ok &= check(ProbeChildModifier::begin_calls["select_from_child1"] == 1,
              "select_from should include listed child1");
  ok &= check(ProbeChildModifier::begin_calls["select_from_child2"] == 1,
              "select_from should include listed child2");
  ok &= check(ProbeChildModifier::begin_calls["select_from_child3"] == 0,
              "select_from should exclude children not in the configured list");
  return ok;
}

static bool testParentModifierRandomSelectFromRejectsParentEntries() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child = makeMod<ProbeChildModifier>(
      R"(
name = "parent_select_child"
type = "probe_child"
)",
      engine);
  engine.modifier_map["parent_select_child"] = child;

  auto nested_parent = makeMod<ParentModifier>(
      R"(
name = "nested_parent"
type = "parent"
children = [ "parent_select_child" ]
)",
      engine);
  engine.modifier_map["nested_parent"] = nested_parent;

  bool threw = false;
  try {
    (void) makeMod<ParentModifier>(
        R"(
name = "invalid_select_from_parent"
type = "parent"
random = true
select_from = [ "nested_parent" ]
)",
        engine);
  } catch (const std::runtime_error&) {
    threw = true;
  }

  return check(threw, "select_from should reject parent modifiers at parse time");
}

static bool testParentModifierRandomSelectGroupsRestrictsPool() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child1 = makeMod<ProbeChildModifier>(
      R"(
name = "select_groups_child1"
type = "probe_child"
groups = [ "group_a" ]
)",
      engine);
  auto child2 = makeMod<ProbeChildModifier>(
      R"(
name = "select_groups_child2"
type = "probe_child"
groups = [ "group_b" ]
)",
      engine);
  auto child3 = makeMod<ProbeChildModifier>(
      R"(
name = "select_groups_child3"
type = "probe_child"
groups = [ "group_c" ]
)",
      engine);

  engine.modifier_map["select_groups_child1"] = child1;
  engine.modifier_map["select_groups_child2"] = child2;
  engine.modifier_map["select_groups_child3"] = child3;

  auto parent = makeMod<ParentModifier>(
      R"(
name = "select_groups_parent"
type = "parent"
random = true
value = 2
select_groups = [ "group_a", "group_b" ]
)",
      engine);

  parent->begin();
  bool ok = true;
  ok &= check(ProbeChildModifier::begin_calls["select_groups_child1"] == 1,
              "select_groups should include child in group_a");
  ok &= check(ProbeChildModifier::begin_calls["select_groups_child2"] == 1,
              "select_groups should include child in group_b");
  ok &= check(ProbeChildModifier::begin_calls["select_groups_child3"] == 0,
              "select_groups should exclude children outside selected groups");
  return ok;
}

static bool testParentModifierRandomSelectGroupsExcludesParents() {
  ProbeChildModifier::reset();
  MockEngine engine;

  auto child = makeMod<ProbeChildModifier>(
      R"(
name = "groups_parent_child"
type = "probe_child"
)",
      engine);
  engine.modifier_map["groups_parent_child"] = child;

  auto plain_parent = makeMod<ParentModifier>(
      R"(
name = "groups_plain_parent"
type = "parent"
children = [ "groups_parent_child" ]
)",
      engine);
  engine.modifier_map["groups_plain_parent"] = plain_parent;

  auto random_parent = makeMod<ParentModifier>(
      R"(
name = "groups_random_parent"
type = "parent"
random = true
value = 1
select_groups = [ "parent" ]
)",
      engine);

  random_parent->begin();
  bool ok = true;
  ok &= check(ProbeChildModifier::begin_calls["groups_parent_child"] == 0,
              "select_groups should not choose parent modifiers");
  return ok;
}

int main() {
  bool ok = true;
  ok &= testGameDefaultsToNoErrorsBeforeLoad();
  ok &= testCooldownModifierBlocksAfterTrigger();
  ok &= testCooldownModifierRequiresWhileConditionWhenCumulativeStartType();
  ok &= testCooldownModifierCancelableResetsIfConditionDrops();
  ok &= testCooldownRestoresLatestBlockedCommandStateOnExpiry();
  ok &= testCooldownCancelableHybridReleaseDoesNotRePressOnExpiry();
  ok &= testDelayModifierQueuesAndReplays();
  ok &= testDelayModifierAppliesToAllCommands();
  ok &= testDelayModifierReplaysMultipleSequentialEvents();
  ok &= testDelayModifierDelaysHybridButtonAndAxis();
  ok &= testDelayModifierClearsQueueAcrossLifecycle();
  ok &= testDelayModifierClearPendingInjectedEvents();
  ok &= testDelayModifierStressRapidDodgeEvents();
  ok &= testDelayModifierStressInterleavedJoystickEvents();
  ok &= testDisableDefaultFilterBlocksConfiguredCommandOnly();
  ok &= testDisableRespectsWhileCondition();
  ok &= testDisableWhileTransitionClampsHeldAxes();
  ok &= testDisableWhileTransitionRestoresHeldAxesWhenConditionClears();
  ok &= testDisableWhileHybridReleaseDoesNotRePressOnConditionClear();
  ok &= testOnlyAimMovementWorksWithControllerMirrorRemap();
  ok &= testDisableRespectsDistanceMovementCondition();
  ok &= testDisableRespectsPersistentCondition();
  ok &= testDisableFilterAboveBlocksOnlyPositiveValues();
  ok &= testDisableFilterBelowBlocksOnlyNegativeValues();
  ok &= testDisableFilterDirectionsOnThreeStateSelectWeapon();
  ok &= testDisableBlocksAllConfiguredCommandsIncludingHybridAxis();
  ok &= testDisableNoAimingStyleBlocksHybridButtonAndAxis();
  ok &= testFormulaModifierOffsetsAndRestores();
  ok &= testFormulaModifierJankyHonorsWhileCondition();
  ok &= testFormulaModifierRestoresOnWhileConditionClear();
  ok &= testFormulaModifierSupportsEightCurveType();
  ok &= testFormulaCircleAssignsSinThenCosByAxisOrder();
  ok &= testFormulaEightCurveAlternatesAcrossAxisPairs();
  ok &= testFormulaRandomOffsetAppliesConstantRangeWithoutDirection();
  ok &= testFormulaRandomOffsetDirectionDecomposesPairs();
  ok &= testFormulaRandomOffsetHonorsWhileAndRetainsFixedOffset();
  ok &= testRemapModifierRespectsToNegForThreeStateFromDPad();
  ok &= testRemapModifierClearsPreviousThreeStateDirection();
  ok &= testScalingModifierScalesAndClamps();
  ok &= testScalingModifierSupportsNegativeAmplitudeAndOffset();
  ok &= testScalingModifierInvertsVerticalCameraAxis();
  ok &= testScalingModifierUsesWhileAmplitudeWhenConditionTrue();
  ok &= testScalingModifierIgnoresWhileAmplitudeWithoutWhileCondition();
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
  ok &= testMenuModifiersDoNotBlockLaterDisableModifier();
  ok &= testAllDifferingModifierPairOrdersStayStableWhenIdle();
  ok &= testParentModifierForwardsLifecycleAndTweak();
  ok &= testParentModifierForwardsRemapIntoLaterChildTweaks();
  ok &= testParentModifierRandomSelectionChoosesRequestedCount();
  ok &= testParentModifierRandomSelectFromRestrictsPool();
  ok &= testParentModifierRandomSelectFromRejectsParentEntries();
  ok &= testParentModifierRandomSelectGroupsRestrictsPool();
  ok &= testParentModifierRandomSelectGroupsExcludesParents();

  if (!ok) {
    return 1;
  }
  std::cout << "PASS: modifier type regression tests\n";
  return 0;
}
