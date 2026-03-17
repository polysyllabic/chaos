#include <cmath>
#include <functional>
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

#include <config.hpp>
#include <Controller.hpp>
#include <ControllerInput.hpp>
#include <CooldownModifier.hpp>
#include <DelayModifier.hpp>
#include <DisableModifier.hpp>
#include <DeviceEvent.hpp>
#include <EngineInterface.hpp>
#include <FormulaModifier.hpp>
#include <Game.hpp>
#include <GameCommand.hpp>
#include <MenuItem.hpp>
#include <MenuModifier.hpp>
#include <ParentModifier.hpp>
#include <RemapModifier.hpp>
#include <RepeatModifier.hpp>
#include <ScalingModifier.hpp>
#include <Sequence.hpp>
#include <SequenceModifier.hpp>

using namespace Chaos;

namespace {

static bool check(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    return false;
  }
  return true;
}

class LoadedGameHarness : public EngineInterface {
public:
  Controller controller;
  Game game;

  std::list<std::shared_ptr<Modifier>> active_mods;
  std::vector<DeviceEvent> pipelined_events;
  std::vector<std::string> set_off_calls;
  std::vector<std::string> set_on_calls;
  std::vector<std::pair<std::string, short>> set_value_calls;

  LoadedGameHarness() : game(controller) {}

  bool loadTlou2Config() { return game.loadConfigFile("chaos/examples/tlou2.toml", this); }

  void resetObservations() {
    pipelined_events.clear();
    set_off_calls.clear();
    set_on_calls.clear();
    set_value_calls.clear();
  }

  std::shared_ptr<GameCommand> command(const std::string& name) {
    return game.getCommand(name);
  }

  std::shared_ptr<ControllerInput> input(const std::string& name) {
    return game.getSignalTable().getInput(name);
  }

  DeviceEvent commandEvent(const std::string& name, short value) {
    auto cmd = command(name);
    if (!cmd || !cmd->getInput()) {
      throw std::runtime_error("Unknown command in test event setup: " + name);
    }
    auto signal = cmd->getInput();
    return {0, value, static_cast<uint8_t>(signal->getButtonType()), signal->getID()};
  }

  DeviceEvent commandHybridAxisEvent(const std::string& name, short value) {
    auto cmd = command(name);
    if (!cmd || !cmd->getInput()) {
      throw std::runtime_error("Unknown command in test hybrid event setup: " + name);
    }
    auto signal = cmd->getInput();
    if (signal->getType() != ControllerSignalType::HYBRID) {
      throw std::runtime_error("Command is not hybrid in test hybrid event setup: " + name);
    }
    return {0, value, TYPE_AXIS, signal->getHybridAxis()};
  }

  DeviceEvent signalEvent(const std::string& name, short value) {
    auto signal = input(name);
    if (!signal) {
      throw std::runtime_error("Unknown signal in test event setup: " + name);
    }
    return {0, value, static_cast<uint8_t>(signal->getButtonType()), signal->getID()};
  }

  bool isPaused() override { return false; }

  void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod) override {
    (void) sourceMod;
    pipelined_events.push_back(event);
    controller.applyEvent(event);
  }

  short getState(uint8_t id, uint8_t type) override { return controller.getState(id, type); }

  bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command) override {
    return command && command->getInput() && command->getInput()->matches(event);
  }

  void setOff(std::shared_ptr<GameCommand> command) override {
    if (!command || !command->getInput()) {
      return;
    }
    set_off_calls.push_back(command->getName());
    auto signal = command->getInput();
    DeviceEvent ev = {0, signal->getMin(signal->getButtonType()), static_cast<uint8_t>(signal->getButtonType()),
                      signal->getID()};
    controller.applyEvent(ev);
  }

  void setOn(std::shared_ptr<GameCommand> command) override {
    if (!command || !command->getInput()) {
      return;
    }
    set_on_calls.push_back(command->getName());
    auto signal = command->getInput();
    DeviceEvent ev = {0, signal->getMax(signal->getButtonType()), static_cast<uint8_t>(signal->getButtonType()),
                      signal->getID()};
    controller.applyEvent(ev);
  }

  void setValue(std::shared_ptr<GameCommand> command, short value) override {
    if (!command || !command->getInput()) {
      return;
    }
    set_value_calls.push_back({command->getName(), value});
    auto signal = command->getInput();
    DeviceEvent ev = {0, value, static_cast<uint8_t>(signal->getButtonType()), signal->getID()};
    controller.applyEvent(ev);
  }

  void applyEvent(const DeviceEvent& event) override { controller.applyEvent(event); }

  std::shared_ptr<Modifier> getModifier(const std::string& name) override {
    return game.getModifier(name);
  }

  std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() override {
    return game.getModifierMap();
  }

  std::list<std::shared_ptr<Modifier>>& getActiveMods() override { return active_mods; }

  std::shared_ptr<MenuItem> getMenuItem(const std::string& name) override {
    return game.getMenu().getMenuItem(name);
  }

  void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) override {
    game.getMenu().setState(item, new_val, false, controller);
  }

  void restoreMenuState(std::shared_ptr<MenuItem> item) override {
    game.getMenu().restoreState(item, controller);
  }

  std::shared_ptr<ControllerInput> getInput(const std::string& name) override {
    return game.getSignalTable().getInput(name);
  }

  std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event) override {
    return game.getSignalTable().getInput(event);
  }

  void addControllerInputs(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<ControllerInput>>& vec) override {
    game.getSignalTable().addToVector(config, key, vec);
  }

  std::string getEventName(const DeviceEvent& event) override { return game.getEventName(event); }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<GameCommand>>& vec) override {
    game.addGameCommands(config, key, vec);
  }

  void addGameCommands(const toml::table& config, const std::string& key,
                       std::vector<std::shared_ptr<ControllerInput>>& vec) override {
    game.addGameCommands(config, key, vec);
  }

  void addGameConditions(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCondition>>& vec) override {
    game.addGameConditions(config, key, vec);
  }

  std::shared_ptr<Sequence> createSequence(toml::table& config, const std::string& key,
                                           bool required) override {
    return game.makeSequence(config, key, required);
  }
};

const toml::table& loadTlou2CommonConfig() {
  static const toml::table config = toml::parse_file("chaos/examples/tlou2-common.toml");
  return config;
}

const toml::table* findModifierConfig(const toml::table& config, const std::string& name) {
  const toml::array* modifiers = config["modifier"].as_array();
  if (!modifiers) {
    return nullptr;
  }
  for (const auto& node : *modifiers) {
    const toml::table* mod = node.as_table();
    if (!mod) {
      continue;
    }
    if (mod->contains("name") && mod->at("name").value<std::string>() == name) {
      return mod;
    }
  }
  return nullptr;
}

bool stringArrayContainsAll(const toml::table& config, const std::string& key,
                            const std::vector<std::string>& values) {
  const toml::array* arr = config.get(key)->as_array();
  if (!arr) {
    return false;
  }
  for (const auto& expected : values) {
    bool found = false;
    for (const auto& node : *arr) {
      if (node.value<std::string>() == expected) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

bool intArrayEquals(const toml::table& config, const std::string& key, const std::vector<int64_t>& values) {
  const toml::array* arr = config.get(key)->as_array();
  if (!arr || arr->size() != values.size()) {
    return false;
  }
  for (size_t i = 0; i < values.size(); ++i) {
    if ((*arr)[i].value<int64_t>() != values[i]) {
      return false;
    }
  }
  return true;
}

bool approxEquals(const toml::table& config, const std::string& key, double expected) {
  std::optional<double> value = config[key].value<double>();
  if (!value) {
    if (std::optional<int64_t> int_value = config[key].value<int64_t>()) {
      value = static_cast<double>(*int_value);
    }
  }
  return value && std::fabs(*value - expected) < 1e-6;
}

bool menuItemsContainEntry(const toml::table& config, const std::string& entry_name,
                           std::optional<int64_t> value = std::nullopt) {
  const toml::array* items = config["menu_items"].as_array();
  if (!items) {
    return false;
  }
  for (const auto& node : *items) {
    const toml::table* item = node.as_table();
    if (!item) {
      continue;
    }
    if (item->at("entry").value<std::string>() != entry_name) {
      continue;
    }
    if (!value) {
      return true;
    }
    return item->contains("value") && item->at("value").value<int64_t>() == *value;
  }
  return false;
}

bool beginSequenceContainsCommand(const toml::table& config, const std::string& key,
                                  const std::string& event_name, const std::string& command_name) {
  const toml::array* seq = config.get(key)->as_array();
  if (!seq) {
    return false;
  }
  for (const auto& node : *seq) {
    const toml::table* step = node.as_table();
    if (!step) {
      continue;
    }
    if (step->at("event").value<std::string>() == event_name &&
        step->at("command").value<std::string>() == command_name) {
      return true;
    }
  }
  return false;
}

bool remapContains(const toml::table& config, const std::string& from, const std::string& to,
                   std::optional<std::string> to_neg = std::nullopt,
                   std::optional<bool> invert = std::nullopt,
                   std::optional<bool> to_min = std::nullopt,
                   std::optional<double> threshold = std::nullopt,
                   std::optional<double> sensitivity = std::nullopt) {
  const toml::array* remaps = config["remap"].as_array();
  if (!remaps) {
    return false;
  }
  for (const auto& node : *remaps) {
    const toml::table* remap = node.as_table();
    if (!remap) {
      continue;
    }
    if (remap->at("from").value<std::string>() != from || remap->at("to").value<std::string>() != to) {
      continue;
    }
    if (to_neg && remap->at("to_neg").value<std::string>() != *to_neg) {
      continue;
    }
    if (invert && remap->at("invert").value_or(false) != *invert) {
      continue;
    }
    if (to_min && remap->at("to_min").value_or(false) != *to_min) {
      continue;
    }
    if (threshold && !approxEquals(*remap, "threshold", *threshold)) {
      continue;
    }
    if (sensitivity && !approxEquals(*remap, "sensitivity", *sensitivity)) {
      continue;
    }
    return true;
  }
  return false;
}

template <typename T>
bool hasRuntimeType(const std::shared_ptr<Modifier>& modifier) {
  return std::dynamic_pointer_cast<T>(modifier) != nullptr;
}

struct RepresentativeSpec {
  const char* mod_name;
  const char* mod_type;
  const char* feature;
  std::function<bool(const toml::table&)> validate_config;
  bool (*validate_runtime_type)(const std::shared_ptr<Modifier>&);
};

std::vector<RepresentativeSpec> representativeSpecs() {
  return {
      {"Sniper Speed", "cooldown", "trigger",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "while", {"shooting"}) && approxEquals(mod, "time_on", 0.1) &&
                approxEquals(mod, "time_off", 5.0) && !mod.contains("start_type");
       },
       hasRuntimeType<CooldownModifier>},
      {"Bad Stamina", "cooldown", "cumulative",
       [](const toml::table& mod) {
         return mod["start_type"].value<std::string>() == "cumulative" &&
                stringArrayContainsAll(mod, "while", {"movement", "sprint"});
       },
       hasRuntimeType<CooldownModifier>},
      {"Snapshot", "cooldown", "cancelable",
       [](const toml::table& mod) {
         return mod["start_type"].value<std::string>() == "cancelable" &&
                stringArrayContainsAll(mod, "while", {"aiming"});
       },
       hasRuntimeType<CooldownModifier>},
      {"Jump Delay", "delay", "button delay",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"jump"}) && approxEquals(mod, "delay", 2.0);
       },
       hasRuntimeType<DelayModifier>},
      {"Aim Delay", "delay", "hybrid trigger delay",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"aiming"}) && approxEquals(mod, "delay", 1.0);
       },
       hasRuntimeType<DelayModifier>},
      {"Joystick Delay", "delay", "joystick delay",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to",
                                       {"horizontal movement", "vertical movement", "horizontal camera",
                                        "vertical camera"}) &&
                approxEquals(mod, "delay", 0.2);
       },
       hasRuntimeType<DelayModifier>},
      {"No Crouch/Prone", "disable", "button disable",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"crouch/prone"}) &&
                beginSequenceContainsCommand(mod, "begin_sequence", "release", "crouch/prone");
       },
       hasRuntimeType<DisableModifier>},
      {"No Aiming", "disable", "hybrid trigger disable",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"aiming"}) &&
                beginSequenceContainsCommand(mod, "begin_sequence", "release", "aiming");
       },
       hasRuntimeType<DisableModifier>},
      {"Only Strafing", "disable", "axis disable",
       [](const toml::table& mod) { return stringArrayContainsAll(mod, "applies_to", {"vertical movement"}); },
       hasRuntimeType<DisableModifier>},
      {"Pacifist%", "disable", "disable multiple controls",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"melee", "reload/toss"});
       },
       hasRuntimeType<DisableModifier>},
      {"No Aim Movement", "disable", "conditional disable",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"vertical movement", "horizontal movement"}) &&
                stringArrayContainsAll(mod, "while", {"aiming"});
       },
       hasRuntimeType<DisableModifier>},
      {"No Reloading", "disable", "disable w/ 2 conditions",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"reload/toss"}) &&
                stringArrayContainsAll(mod, "while", {"gun selected", "not aiming"});
       },
       hasRuntimeType<DisableModifier>},
      {"Mega Scope Sway", "formula", "eight curve",
       [](const toml::table& mod) {
         return mod["formula_type"].value<std::string>() == "eight_curve" &&
                stringArrayContainsAll(mod, "while", {"aiming"});
       },
       hasRuntimeType<FormulaModifier>},
      {"Random Movement", "formula", "janky",
       [](const toml::table& mod) {
         return mod["formula_type"].value<std::string>() == "janky" && approxEquals(mod, "amplitude", 0.5);
       },
       hasRuntimeType<FormulaModifier>},
      {"Left Joystick Drift", "formula", "random offset",
       [](const toml::table& mod) {
         return mod["formula_type"].value<std::string>() == "random_offset" &&
                intArrayEquals(mod, "range", {40, 75}) && intArrayEquals(mod, "direction", {0, 360});
       },
       hasRuntimeType<FormulaModifier>},
      {"Mute Sound Effects", "menu", "set option",
       [](const toml::table& mod) { return menuItemsContainEntry(mod, "effects", 0); },
       hasRuntimeType<MenuModifier>},
      {"Afterlife", "menu", "counter-guarded option",
       [](const toml::table& mod) { return menuItemsContainEntry(mod, "afterlife"); },
       hasRuntimeType<MenuModifier>},
      {"Musical Combat", "menu", "enabling unhides menu item",
       [](const toml::table& mod) { return menuItemsContainEntry(mod, "combat cues", 1); },
       hasRuntimeType<MenuModifier>},
      {"Noir", "menu", "apply while another menu modifier is active",
       [](const toml::table& mod) { return menuItemsContainEntry(mod, "noir"); },
       hasRuntimeType<MenuModifier>},
      {"Restart Checkpoint", "menu", "select w/ confirm",
       [](const toml::table& mod) {
         return menuItemsContainEntry(mod, "restart") && mod["reset_on_finish"].value_or(true) == false;
       },
       hasRuntimeType<MenuModifier>},
      {"Navigation Assistance", "parent", "fixed list",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "children",
                                       {"Enable Navigation Assistance", "Periodic Navigation Assistance"});
       },
       hasRuntimeType<ParentModifier>},
      {"Mystery", "parent", "random selection",
       [](const toml::table& mod) {
         return mod["random"].value_or(false) && mod["value"].value<int64_t>() == 1;
       },
       hasRuntimeType<ParentModifier>},
      {"Controller Flip", "remap", "axis/button/hybrid remaps",
       [](const toml::table& mod) {
         return remapContains(mod, "DY", "DY", std::nullopt, true) && remapContains(mod, "R1", "R2") &&
                remapContains(mod, "X", "TRIANGLE");
       },
       hasRuntimeType<RemapModifier>},
      {"Controller Mirror", "remap", "axis/button/3-way combinations",
       [](const toml::table& mod) {
         return remapContains(mod, "DX", "SQUARE", std::string("CIRCLE")) &&
                remapContains(mod, "DY", "X", std::string("TRIANGLE")) &&
                remapContains(mod, "L2", "R2") && remapContains(mod, "R2", "L2");
       },
       hasRuntimeType<RemapModifier>},
      {"D-pad Rotate", "remap", "3-way-to-3-way swap",
       [](const toml::table& mod) {
         return remapContains(mod, "DX", "DY") && remapContains(mod, "DY", "DX", std::nullopt, true);
       },
       hasRuntimeType<RemapModifier>},
      {"Swap D-pad/Left Joystick", "remap", "3-way-to-axis swap",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "disable_signals", {"LX", "LY"}) && remapContains(mod, "DX", "LX") &&
                remapContains(mod, "LX", "DX");
       },
       hasRuntimeType<RemapModifier>},
      {"Swap Shapes/Right Joystick", "remap", "button-to-axis and axis-to-button",
       [](const toml::table& mod) {
         return remapContains(mod, "RX", "CIRCLE", std::string("SQUARE"), std::nullopt, std::nullopt, 0.5) &&
                remapContains(mod, "SQUARE", "RX", std::nullopt, std::nullopt, true);
       },
       hasRuntimeType<RemapModifier>},
      {"Motion Control Movement", "remap", "accel-to-axis swap",
       [](const toml::table& mod) {
         return remapContains(mod, "ACCX", "LX", std::nullopt, std::nullopt, std::nullopt, std::nullopt, 20.0) &&
                remapContains(mod, "LX", "NOTHING");
       },
       hasRuntimeType<RemapModifier>},
      {"Touchpad Aiming", "parent", "touchpad-to-axis swap",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "children", {"Touchpad to Camera", "Aim Adjust"});
       },
       hasRuntimeType<ParentModifier>},
      {"Factions Pro", "repeat", "basic cycle",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"crouch/prone"}) && approxEquals(mod, "time_on", 0.1) &&
                approxEquals(mod, "time_off", 0.6);
       },
       hasRuntimeType<RepeatModifier>},
      {"Leeroy Jenkins", "repeat", "with force_on",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"dodge/sprint", "vertical movement"}) &&
                intArrayEquals(mod, "force_on", {1, -128});
       },
       hasRuntimeType<RepeatModifier>},
      {"Moonwalk", "scaling", "invert",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "applies_to", {"vertical movement"}) && approxEquals(mod, "amplitude", -1.0);
       },
       hasRuntimeType<ScalingModifier>},
      {"Min Sensitivity", "scaling", "scale axis",
       [](const toml::table& mod) { return approxEquals(mod, "amplitude", 0.4); },
       hasRuntimeType<ScalingModifier>},
      {"Pdub It", "sequence", "begin sequence",
       [](const toml::table& mod) {
         return beginSequenceContainsCommand(mod, "begin_sequence", "sequence", "disable all") &&
                beginSequenceContainsCommand(mod, "begin_sequence", "press", "select consumable") &&
                beginSequenceContainsCommand(mod, "begin_sequence", "press", "reload/toss");
       },
       hasRuntimeType<SequenceModifier>},
      {"Double Tap", "sequence", "triggered sequence",
       [](const toml::table& mod) {
         return stringArrayContainsAll(mod, "while", {"aiming"}) &&
                stringArrayContainsAll(mod, "trigger", {"shoot"}) &&
                beginSequenceContainsCommand(mod, "repeat_sequence", "press", "shoot");
       },
       hasRuntimeType<SequenceModifier>},
      {"Rubbernecking", "sequence", "repeated sequence",
       [](const toml::table& mod) {
         return approxEquals(mod, "start_delay", 6.0) && approxEquals(mod, "cycle_delay", 0.1) &&
                beginSequenceContainsCommand(mod, "repeat_sequence", "hold", "vertical movement");
       },
       hasRuntimeType<SequenceModifier>},
  };
}

bool testRepresentativeConfigCoverageMatrix() {
  const toml::table& config = loadTlou2CommonConfig();
  bool ok = true;

  for (const auto& spec : representativeSpecs()) {
    const toml::table* mod = findModifierConfig(config, spec.mod_name);
    ok &= check(mod != nullptr,
                std::string("missing representative modifier '") + spec.mod_name + "' for feature '" + spec.feature +
                    "'");
    if (!mod) {
      continue;
    }
    ok &= check(mod->at("type").value<std::string>() == spec.mod_type,
                std::string("modifier '") + spec.mod_name + "' should have type '" + spec.mod_type + "'");
    ok &= check(spec.validate_config(*mod),
                std::string("modifier '") + spec.mod_name + "' should express feature '" + spec.feature +
                    "' in tlou2-common.toml");
  }

  return ok;
}

bool testRepresentativeRuntimeTypesLoadFromTlou2() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "tlou2.toml should load successfully");
  ok &= check(harness.game.getErrors() == 0, "tlou2.toml should load without parse errors");
  if (!ok) {
    return false;
  }

  for (const auto& spec : representativeSpecs()) {
    std::shared_ptr<Modifier> mod = harness.getModifier(spec.mod_name);
    ok &= check(mod != nullptr,
                std::string("runtime load should provide modifier '") + spec.mod_name + "'");
    if (!mod) {
      continue;
    }
    ok &= check(spec.validate_runtime_type(mod),
                std::string("modifier '") + spec.mod_name + "' should instantiate the expected runtime class");
  }
  return ok;
}

bool testSnapshotCancelableCooldownSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "snapshot smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<CooldownModifier>(harness.getModifier("Snapshot"));
  ok &= check(mod != nullptr, "snapshot smoke: Snapshot should be a cooldown modifier");
  if (!ok || !mod) {
    return false;
  }

  mod->_begin();
  harness.applyEvent(harness.commandEvent("aiming", 1));
  mod->_update(false);
  usleep(500000);
  mod->_update(false);
  harness.applyEvent(harness.commandEvent("aiming", 0));
  mod->_update(false);
  usleep(700000);
  mod->_update(false);

  DeviceEvent aim_press = harness.commandEvent("aiming", 1);
  ok &= check(mod->_tweak(aim_press), "snapshot smoke: cancelable cooldown should reset after aim is released");
  ok &= check(harness.set_off_calls.empty(),
              "snapshot smoke: cancelable cooldown should not force aiming off after early release");
  return ok;
}

bool testJoystickDelaySmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "joystick delay smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<DelayModifier>(harness.getModifier("Joystick Delay"));
  ok &= check(mod != nullptr, "joystick delay smoke: Joystick Delay should be a delay modifier");
  if (!ok || !mod) {
    return false;
  }

  mod->_begin();
  harness.resetObservations();
  DeviceEvent move = harness.commandEvent("horizontal movement", 48);
  ok &= check(!mod->_tweak(move), "joystick delay smoke: matching axis event should be queued and blocked");
  ok &= check(harness.pipelined_events.empty(),
              "joystick delay smoke: no pipelined events should be emitted before the delay elapses");
  usleep(260000);
  mod->_update(false);
  ok &= check(!harness.pipelined_events.empty(),
              "joystick delay smoke: delayed axis event should replay after the configured delay");
  if (!harness.pipelined_events.empty()) {
    const DeviceEvent& replayed = harness.pipelined_events.back();
    ok &= check(replayed.id == move.id && replayed.type == move.type && replayed.value == move.value,
                "joystick delay smoke: replayed event should match the original delayed axis event");
  }
  return ok;
}

bool testNoAimingDisableSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "no aiming smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<DisableModifier>(harness.getModifier("No Aiming"));
  ok &= check(mod != nullptr, "no aiming smoke: No Aiming should be a disable modifier");
  if (!ok || !mod) {
    return false;
  }

  mod->_begin();
  DeviceEvent aim_button = harness.commandEvent("aiming", 1);
  ok &= check(mod->_tweak(aim_button), "no aiming smoke: modifier should keep processing matching events");
  ok &= check(aim_button.value == 0, "no aiming smoke: aiming button should be forced off");

  DeviceEvent aim_axis = harness.commandHybridAxisEvent("aiming", JOYSTICK_MAX);
  ok &= check(mod->_tweak(aim_axis), "no aiming smoke: hybrid axis event should still be processed");
  ok &= check(aim_axis.value == JOYSTICK_MIN, "no aiming smoke: aiming axis should be forced to its released value");
  return ok;
}

bool testMegaScopeSwayFormulaSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "mega scope sway smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<FormulaModifier>(harness.getModifier("Mega Scope Sway"));
  ok &= check(mod != nullptr, "mega scope sway smoke: Mega Scope Sway should be a formula modifier");
  if (!ok || !mod) {
    return false;
  }

  mod->_begin();
  harness.resetObservations();
  mod->_update(false);
  ok &= check(harness.pipelined_events.empty(),
              "mega scope sway smoke: formula should stay idle until the aiming condition is active");

  harness.applyEvent(harness.commandEvent("aiming", 1));
  usleep(50000);
  mod->_update(false);
  ok &= check(!harness.pipelined_events.empty(),
              "mega scope sway smoke: formula should inject offsets once aiming is active");
  return ok;
}

bool testMusicalCombatMenuSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "musical combat smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<MenuModifier>(harness.getModifier("Musical Combat"));
  auto audio_cues = harness.getMenuItem("audio cues");
  auto combat_cues = harness.getMenuItem("combat cues");
  ok &= check(mod != nullptr, "musical combat smoke: Musical Combat should be a menu modifier");
  ok &= check(audio_cues != nullptr && combat_cues != nullptr,
              "musical combat smoke: required menu items should exist");
  if (!ok || !mod || !audio_cues || !combat_cues) {
    return false;
  }

  ok &= check(audio_cues->isHidden(), "musical combat smoke: audio cues should start hidden");
  ok &= check(combat_cues->getState() == 0, "musical combat smoke: combat cues should start disabled");

  mod->_begin();
  ok &= check(!audio_cues->isHidden(), "musical combat smoke: enabling combat cues should reveal audio cues");
  ok &= check(combat_cues->getState() == 1, "musical combat smoke: combat cues should be enabled on begin");

  mod->_finish();
  ok &= check(audio_cues->isHidden(), "musical combat smoke: restoring combat cues should hide audio cues again");
  ok &= check(combat_cues->getState() == 0, "musical combat smoke: combat cues should restore to default");
  return ok;
}

bool testMysteryParentSelectionSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "mystery smoke: tlou2.toml should load");
  auto parent = std::dynamic_pointer_cast<ParentModifier>(harness.getModifier("Mystery"));
  ok &= check(parent != nullptr, "mystery smoke: Mystery should be a parent modifier");
  if (!ok || !parent) {
    return false;
  }

  parent->_begin();
  int selected_children = 0;
  for (const auto& [name, mod] : harness.getModifierMap()) {
    if (mod && mod.get() != parent.get() && mod->getptr().get() == parent.get()) {
      ++selected_children;
    }
  }
  ok &= check(selected_children == 1, "mystery smoke: random parent should select exactly one child");
  parent->_finish();
  return ok;
}

bool testDPadRotateRemapSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "d-pad rotate smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<RemapModifier>(harness.getModifier("D-pad Rotate"));
  ok &= check(mod != nullptr, "d-pad rotate smoke: D-pad Rotate should be a remap modifier");
  if (!ok || !mod) {
    return false;
  }

  DeviceEvent right = harness.signalEvent("DX", 1);
  ok &= check(mod->remap(right), "d-pad rotate smoke: remap should preserve the rotated event");
  auto rotated = harness.getInput(right);
  ok &= check(rotated != nullptr && rotated->getName() == "DY",
              "d-pad rotate smoke: DX should rotate into the DY signal");
  ok &= check(right.value == 1, "d-pad rotate smoke: positive D-pad input should stay positive after rotation");
  return ok;
}

bool testMoonwalkScalingSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "moonwalk smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<ScalingModifier>(harness.getModifier("Moonwalk"));
  ok &= check(mod != nullptr, "moonwalk smoke: Moonwalk should be a scaling modifier");
  if (!ok || !mod) {
    return false;
  }

  DeviceEvent forward = harness.commandEvent("vertical movement", 90);
  ok &= check(mod->_tweak(forward), "moonwalk smoke: scaling modifier should process matching movement");
  ok &= check(forward.value == -90, "moonwalk smoke: Moonwalk should invert forward movement");
  return ok;
}

bool testFactionsProRepeatSmoke() {
  LoadedGameHarness harness;
  bool ok = true;
  ok &= check(harness.loadTlou2Config(), "factions pro smoke: tlou2.toml should load");
  auto mod = std::dynamic_pointer_cast<RepeatModifier>(harness.getModifier("Factions Pro"));
  ok &= check(mod != nullptr, "factions pro smoke: Factions Pro should be a repeat modifier");
  if (!ok || !mod) {
    return false;
  }

  mod->_begin();
  harness.resetObservations();
  usleep(650000);
  mod->_update(false);
  bool saw_crouch = false;
  for (const auto& call : harness.set_on_calls) {
    if (call == "crouch/prone") {
      saw_crouch = true;
      break;
    }
  }
  ok &= check(saw_crouch, "factions pro smoke: repeat modifier should press crouch/prone during its cycle");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok &= testRepresentativeConfigCoverageMatrix();
  ok &= testRepresentativeRuntimeTypesLoadFromTlou2();
  ok &= testSnapshotCancelableCooldownSmoke();
  ok &= testJoystickDelaySmoke();
  ok &= testNoAimingDisableSmoke();
  ok &= testMegaScopeSwayFormulaSmoke();
  ok &= testMusicalCombatMenuSmoke();
  ok &= testMysteryParentSelectionSmoke();
  ok &= testDPadRotateRemapSmoke();
  ok &= testMoonwalkScalingSmoke();
  ok &= testFactionsProRepeatSmoke();

  if (!ok) {
    return 1;
  }
  std::cout << "PASS: tlou2 representative modifier coverage\n";
  return 0;
}
