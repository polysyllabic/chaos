/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributers.
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
#include <list>
#include <plog/Log.h>
#include <random.hpp>

#include "RemapModifier.hpp"
#include <EngineInterface.hpp>
#include <TOMLUtils.hpp>

using namespace Chaos;

const std::string RemapModifier::mod_type = "remap";

RemapModifier::RemapModifier(toml::table& config, EngineInterface* e) {

  TOMLUtils::checkValid(config, std::vector<std::string>{
      "name", "description", "type", "groups", "signals", "disable_signals", "remap", "random_remap", "unlisted"});

  initialize(config, e);
  
  engine->addControllerInputs(config, "signals", signals);

  disable_signals = config["disable_signals"].value_or(false);

  // remap
  if (config.contains("remap")) {
    if (config.contains("random_remap")) {
      throw std::runtime_error("Use either the 'remap' or 'random_remap' keys, not both.");
    }
    random = false;
    const toml::array* remap_list = config.get("remap")->as_array();
    if (! remap_list) {
      throw std::runtime_error("Expect 'remap' to contain an array of remappings.");
    }
    PLOG_DEBUG << "Processing remap list";
    for (auto& elem : *remap_list) {
      const toml::table* remapping = elem.as_table();
      if (! remapping) {
	      throw std::runtime_error("Remapping instructions must be formatted as a table");
      }
      TOMLUtils::checkValid(*remapping, std::vector<std::string>{"from", "to", "to_neg", "to_min", "invert",
                 "threshold", "sensitivity"}, "remap config");
      
      std::shared_ptr<ControllerInput> from = lookupInput(*remapping, "from", true);
      ControllerSignalType from_type = from->getType();

      std::shared_ptr<ControllerInput> to = lookupInput(*remapping, "to", true);
      ControllerSignalType to_type = to->getType();

      std::shared_ptr<ControllerInput> to_neg = lookupInput(*remapping, "to_neg", false);

      // Check for unsupported remappings
      if (from_type == ControllerSignalType::DUMMY) {
        throw std::runtime_error("Cannot map from NONE or NOTHING");
      } else if (from_type != to_type) {
        if ((to_type == ControllerSignalType::ACCELEROMETER ||
            to_type == ControllerSignalType::GYROSCOPE || 
            to_type == ControllerSignalType::TOUCHPAD) || (to_neg && (
            to_neg->getType() == ControllerSignalType::ACCELEROMETER || 
            to_neg->getType() == ControllerSignalType::GYROSCOPE || 
            to_neg->getType() == ControllerSignalType::TOUCHPAD
            ))) {
          throw std::runtime_error("Cross-type remapping not supported going to the accelerometer, gyroscope, or touchpad.");
        }
        if (to_neg && to_neg->getType() != ControllerSignalType::DUMMY && to_neg->getType() != to_type) {
          PLOG_WARNING << "The 'to' and 'to_neg' signals belong to different classes. Are you sure this is what you want?";
        }
      }
      bool to_min = (*remapping)["to_min"].value_or(false);
      bool invert = (*remapping)["invert"].value_or(false);
      if (invert && (from_type == ControllerSignalType::BUTTON || from_type == ControllerSignalType::HYBRID)) {
        PLOG_WARNING << "Inverting the signal only makes sense for axes. Ignored.";
        invert = false;
      }

      double thresh_proportion = (*remapping)["threshold"].value_or(0);
      short threshold = 1;
      if (thresh_proportion < 0 || thresh_proportion > 1) {
        PLOG_WARNING << "Threshold proportion = " << thresh_proportion << ": must be between 0 and 1";
      } else {
        threshold = (int) (thresh_proportion * JOYSTICK_MAX);
      }

      double sensitivity = (*remapping)["sensitivity"].value_or(1);
      if (sensitivity == 0) {
        PLOG_ERROR << "The sensitivity cannot be 0. Using 1 instead.";
        sensitivity = 1;
      }
      remaps.insert({from, {to, to_neg, to_min, invert, threshold, sensitivity}});
    }
  }

  // Note: Random remapping mmay break if axes and buttons are included in the same list.
  // Currently we don't check for this.
  if (config.contains("random_remap")) {
    random = true;
    const toml::array* remap_list = config.get("random_remap")-> as_array();
    if (! remap_list || !remap_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("random_remap must be an array of strings");
    }
    PLOG_DEBUG << "Adding list of random remaps for " << name;
    for (auto& elem : *remap_list) {
      std::optional<std::string> signame = elem.value<std::string>();
      // the is_homogenous test above should ensure that signame always has a value
      assert(signame);
      PLOG_DEBUG << "Processing " << *signame;
      std::shared_ptr<ControllerInput> sig = engine->getInput(*signame);
      if (! sig) {
	      throw std::runtime_error("Controller input for random remap '" + *signame + "' is not defined");
      }
      remaps.insert({sig, {nullptr, nullptr, false, false, 0, 1}});
    }
  }
  PLOG_DEBUG << name << ": random = " << random;
  for (auto r : remaps) {
    PLOG_DEBUG << "from " << (r.first)->getName();
  }
}

std::shared_ptr<ControllerInput> RemapModifier::lookupInput(const toml::table& config, const std::string& key, bool required) {
  std::optional<std::string> inp = config[key].value<std::string>();
  std::shared_ptr<ControllerInput> rval = nullptr;
  if (inp) {
    rval = engine->getInput(*inp);
    if (!rval) {
      throw std::runtime_error(*inp + " is not a defined signal");
    }
  } else if (required) {
    throw std::runtime_error("Missing Required '" + key + "' key in remap table");
  } 
  return rval;
}

void RemapModifier::begin() {
  DeviceEvent event{};
  if (random) {
    // Generate a new remapping
    Random rng;
    std::vector<std::shared_ptr<ControllerInput>> buttons;
    // Collect a list of the signals that we're going to remap
    for (auto& [key, value] : remaps) {
      buttons.push_back(key);
    }
    // Iterate through the list of signals we're remapping and assign a random signal to it
    for (auto& [key, value] : remaps) {
      int index = floor(rng.uniform(0, buttons.size()-0.01));
      auto it = buttons.begin();
      std::advance(it, index);
      value.to_console =  *it;
      PLOG_DEBUG << key->getName() << " remapped to " << (value.to_console)->getName();
      buttons.erase(it);
    }
    PLOG_DEBUG << "Verify assignments:";
    for (auto [key, value] : remaps) {
      PLOG_DEBUG << key->getName() << " remapped to " << (value.to_console)->getName();
    }
  }
  if (disable_signals) {
    for (auto& sig : signals) {
      event.value = 0;
      event.id = sig->getID();
      event.type = sig->getButtonType();
      engine->applyEvent(event);
      if (sig->getType() == ControllerSignalType::HYBRID) {
        event.id = sig->getHybridAxisIndex();
        event.type = TYPE_AXIS;
        event.value = JOYSTICK_MIN;
        engine->applyEvent(event);
      }
    }
  }
}

bool RemapModifier::remap(DeviceEvent& event) {
  DeviceEvent new_event;
  auto signal = engine->getInput(event);
  short orig_val = event.value;
  // Is this event in our list of remaps?
  if (auto it{remaps.find(signal)}; it != std::end(remaps)) {
    const auto[from, remap]{*it};

    // Touchpad active requires special setup and tear-down
    if (from->getSignal() == ControllerSignal::TOUCHPAD_ACTIVE) {
      if (! touchpad.isActive() && event.value == 0) {
        PLOG_DEBUG << "Begin touchpad use";
        touchpad.clearPrior();
        touchpad.setActive(true);
      } else if (touchpad.isActive() && event.value) {
        PLOG_DEBUG << "End touchpad use";
        touchpad.setActive(false);
        // We're stopping touchpad use. Zero out all axes in use.
        // NB: If we ever do support other controllers, we'll need to get the indirect signal type, not
        // this low-level value
        for (auto s : signals) {
          new_event = {0, 0, s->getID(), TYPE_AXIS};
          engine->fakePipelinedEvent(new_event, getptr());
        }
      }
      return true;
    }

    auto to_console = remap.to_console;
    // When mapping from any type of axis to buttons/hybrids, the choice of button is determined
    // by the sign of the value.
    if (event.value < 0 && event.id == TYPE_AXIS && to_console->getButtonType() == TYPE_BUTTON) {
      to_console = remap.to_negative;
      if (!to_console) {
        PLOG_ERROR << name << " is missing remap for negative values of " << from->getName();
        return true;
      }
    }

    // If we are remapping to NOTHING, we drop this signal.
    if (to_console->getSignal() == ControllerSignal::NOTHING) {
      PLOG_DEBUG << name << " remapping " << from->getName() << " to NOTHING";
      return false;
    }

    // Now handle cases that require additional actions
    switch (from->getType()) {
    case ControllerSignalType::HYBRID:
      // Going from hybrid to button, we drop the axis component
      if (to_console->getType() == ControllerSignalType::BUTTON && event.type == TYPE_AXIS) {
 	      return false;
      }
    case ControllerSignalType::BUTTON:
      // From button to hybrid, we need to insert a new event for the axis
      if (to_console->getType() == ControllerSignalType::HYBRID) {
   	    new_event.id = to_console->getHybridAxis();
        new_event.type = TYPE_AXIS;
   	    new_event.value = event.value ? JOYSTICK_MAX : JOYSTICK_MIN;
        engine->applyEvent(new_event);
      }
      break;
    case ControllerSignalType::AXIS:
      // From axis to button, we need a new event to zero out the second button when the value is
      // below the threshold. The first button will get a 0 value from the changed regular value
      // At this point, to_console has been set with the negative remap value.
      if (to_console->getButtonType() == TYPE_BUTTON && abs(event.value) < remap.threshold) {
        new_event.id = to_console->getID();
        new_event.value = 0;
        new_event.type = TYPE_BUTTON;
        engine->applyEvent(new_event);
      }
    } // end switch

    // Update the event
    event.type = to_console->getButtonType();
    event.id = to_console->getID(event.type);

    // If we're remapping across button classes, we also need to update the value
    event.value = (from->getType() == ControllerSignalType::TOUCHPAD) ?
      touchpadToAxis(from->getSignal(), event.value) : 
      from->remapValue(to_console, event.value);
    if (event.value) {
     PLOG_DEBUG << name << ": " << from->getName() << ":" << orig_val << " to " << to_console->getName() << ":" << event.value;
    }
  }
  return true;
}

short RemapModifier::touchpadToAxis(ControllerSignal tp_axis, short value) {
  short ret;
  // Use the touchpad value to update the running derivative count
  short derivativeValue = touchpad.getVelocity(tp_axis, value) * touchpad.getScale();

  if (derivativeValue > 0) {
    ret = derivativeValue + touchpad.getSkew();
  }
  else if (derivativeValue < 0) {
    ret = derivativeValue - touchpad.getSkew();
  }
  PLOG_DEBUG << "Derivative: " << derivativeValue << ", skew = " << touchpad.getSkew() << ", returning " << ret;
  return ret;
}
