/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2023 The Twitch Controls Chaos developers. See the AUTHORS
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
      "name", "description", "type", "groups", "disable_signals", "remap",
      "random_remap", "unlisted"});

  initialize(config, e);
  
  engine->addControllerInputs(config, "disable_signals", signals);

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

      double thresh_proportion = (*remapping)["threshold"].value_or(1.0);
      short threshold = 1;
      if (thresh_proportion < 0 || thresh_proportion > 1) {
        PLOG_WARNING << "Threshold proportion = " << thresh_proportion << ": must be between 0 and 1";
        thresh_proportion = 0.5;
      }
      threshold = (int) ((double) JOYSTICK_MAX * thresh_proportion);

      double sensitivity = (*remapping)["sensitivity"].value_or(1.0);
      if (sensitivity == 0) {
        PLOG_ERROR << "The sensitivity cannot be 0. Using 1 instead.";
        sensitivity = 1;
      }
      remaps.insert({from, {to, to_neg, to_min, invert, threshold, sensitivity}});
    }
  }

  // Note: Random remapping may break if axes and buttons are included in the same list.
  // Currently we don't check for this.
  if (config.contains("random_remap")) {
    random = true;
    const toml::array* remap_list = config.get("random_remap")-> as_array();
    if (! remap_list || !remap_list->is_homogeneous(toml::node_type::string)) {
      throw std::runtime_error("random_remap must be an array of strings");
    }
    for (auto& elem : *remap_list) {
      std::optional<std::string> signame = elem.value<std::string>();
      // the is_homogenous test above should ensure that signame always has a value
      assert(signame);
      std::shared_ptr<ControllerInput> sig = engine->getInput(*signame);
      if (! sig) {
	      throw std::runtime_error("Controller input for random remap '" + *signame + "' is not defined");
      }
      remaps.insert({sig, {nullptr, nullptr, false, false, 1, 1.0}});
    }
  }
  PLOG_DEBUG << getName() << ": random = " << random;
  for (auto [from, r] : remaps) {
    PLOG_DEBUG << "from " << from->getName() << " to " << (r.to_console ? r.to_console->getName() : "TBD") <<
    " to_min = " << r.to_min << " to_negative = " << (r.to_negative ? r.to_negative->getName() : "NONE") <<
    " threshold = " << r.threshold << " invert = " << r.invert;
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
  if (signals.size() > 0) {
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
  DeviceEvent new_event{};
  auto signal = engine->getInput(event);

  // Is this event in our list of remaps?
  if (auto it{remaps.find(signal)}; it != std::end(remaps)) {
    const auto[from, remap]{*it};

    // Touchpad active requires special setup and tear-down
    if (from->getSignal() == ControllerSignal::TOUCHPAD_ACTIVE) {
      if (! touchpad.isActive() && event.value == 0) {
        PLOG_DEBUG << "Begin touchpad use";
        touchpad.firstTouch();
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

    // If we are remapping to NOTHING, we drop this signal.
    if (to_console->getSignal() == ControllerSignal::NOTHING) {
      PLOG_DEBUG << getName() << " remapping " << from->getName() << " to NOTHING";
      return false;
    }

    DeviceEvent modified;
    // Basic remapping
    modified.id = to_console->getID();
    modified.type = to_console->getButtonType();
    modified.value = event.value;

    // Now handle cases that require additional actions
    if (event.value) {
    switch (from->getType()) {
    case ControllerSignalType::BUTTON:
      switch (to_console->getType()) {
      case ControllerSignalType::THREE_STATE:
        // If the source button maps to the negative dpad value, use -1, otherwise, use 1
        modified.value = remap.to_min ? -1 : 1;
        break;
      case ControllerSignalType::AXIS:
        // If the source button maps to the negative axis value, use min, otherwise, use max
        modified.value = remap.to_min ? JOYSTICK_MIN : JOYSTICK_MAX;
        break;
      case ControllerSignalType::HYBRID:
        // From button to hybrid, we need to insert a new event for the axis
   	    new_event.id = to_console->getHybridAxis();
        new_event.type = TYPE_AXIS;
   	    new_event.value = event.value ? JOYSTICK_MAX : JOYSTICK_MIN;
        engine->applyEvent(new_event);
      }
      break;
    case ControllerSignalType::HYBRID:
      switch (to_console->getType()) {
      // Going from hybrid to button, we drop the axis component
      case ControllerSignalType::BUTTON:
        if (event.type == TYPE_AXIS) {
 	        return false;
        }
        break;
      case ControllerSignalType::THREE_STATE:
        modified.value = remap.to_min ? -1 : 1;
      }
      break;
    case ControllerSignalType::THREE_STATE:
      switch (to_console->getType()) {
      case ControllerSignalType::AXIS:
	      // Scale to axis extremes
      	modified.value = ControllerInput::joystickLimit(JOYSTICK_MAX*event.value);
      	break;
      case ControllerSignalType::BUTTON:
      case ControllerSignalType::HYBRID:
        // if we're mapping a -1 dpad onto a button, it should change to 1
        modified.value = 1;
      }
      break;
    case ControllerSignalType::AXIS:
      switch (to_console->getType()) {
      case ControllerSignalType::BUTTON:
      case ControllerSignalType::HYBRID:
      {
        // When mapping from an axis to buttons/hybrids, the choice of button is determined by
        // the sign of the value.
        if (!remap.to_negative) {
          PLOG_ERROR << getName() << " is missing remap for negative values of " << from->getName();
          return true;
        }
        auto other_button = remap.to_negative;
        if (event.value > 0) {
          // if to_negative not set, to_console is the right signal to send to
          modified.value = (event.value >= remap.threshold) ? 1 : 0;
          }
        else if (event.value < 0) {
          to_console = remap.to_negative;
          other_button = remap.to_console;
          modified.id = to_console->getID();
          modified.value = (event.value <= -remap.threshold) ? 1 : 0;
        }
        // Zero out the opposite button
        new_event.id = other_button->getID();
        new_event.value = 0;
        new_event.type = TYPE_BUTTON;
        engine->applyEvent(new_event);
        // PLOG_DEBUG << "Other button zeroed: " << other_button->getName();
        break;
      }
      case ControllerSignalType::THREE_STATE:
        if (event.value > 0) {
          modified.value = (event.value >= remap.threshold) ? 1 : 0;
          }
        else if (event.value < 0) {
          modified.value = (event.value <= -remap.threshold) ? -1 : 0;
        }
        break;
   	  }
      break;
    case ControllerSignalType::ACCELEROMETER:
      if (to_console->getType() == ControllerSignalType::AXIS) {
	      modified.value= ControllerInput::joystickLimit((short) (-event.value/remap.scale));
      }
      break;
    case ControllerSignalType::TOUCHPAD:
      if (to_console->getType() == ControllerSignalType::AXIS) {
	      modified.value = touchpad.getAxisValue(from->getSignal(), event.value);
      }
      break;
    case ControllerSignalType::DUMMY:
      PLOG_WARNING << "Remapping from NONE or NOTHING";
      break;
    } // end switch
    if (remap.invert) {
      modified.value = ControllerInput::joystickLimit(-event.value);
    }
    }
    if (modified.value != 0) {
     PLOG_VERBOSE << getName() << ": " << from->getName() << ":" << event.value << " to " << to_console->getName() << "(" 
     << (int) modified.type << "." << (int) modified.id << "):" << modified.value;
    }
    // Update the event
    event.id = modified.id;
    event.type = modified.type;
    event.value = modified.value;
  }
  return true;
}
