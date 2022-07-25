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
#pragma once
#include <string>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <toml++/toml.h>
#include <plog/Log.h>

#include "DeviceEvent.hpp"
#include "signals.hpp"
#include "Touchpad.hpp"

namespace Chaos {
  
  class Controller;

  /**
   * \brief Defines the nature of a particular signal coming from the controller
   *
   * This class defines the specifics of the actual signals coming from the controller.
   */
  class ControllerInput {
  private:
    /**
     * \brief The canonical input signal received from the controller
     *
     * This is the actual signal received from the controller.
     */
    ControllerSignal actual;

    /**
     * \brief Name for this signal in the TOML file
     */
    std::string name;

    /**
     * \brief The primary low-level id
     *
     * This is the low-level ID number of the signal. All signals except hybrids use only this
     * field to store the id. For the hybrid controls (L2/R2), this is the button ID.
     */
    uint8_t button_id;
    /**
     * \brief The axis id for hybrid buttons (L2/R2).
     *
     * This field is unused except by hybrid signals
     */
    uint8_t hybrid_axis;

    /**
     * \brief The signal's class.
     *
     * This subdivides button and axis so that we can handle rules for categories of signals that
     * need special treatment (e.g., the touchpad or the hybrid button/axis inputs L2 and R2).
     */
    ControllerSignalType input_type;

    /**
     * \brief The numerical index of the signal.
     *
     * This is the index used directly by the For the hybrid signals L2/R2, this is the button id. For all others, it is their sole id.
     */
    int button_index;
    /**
     * \brief The numerical index of the axis signal for hybrid controls.
     *
     * For other control types, this index is ignored.
     */
    int hybrid_index;

    Controller& controller;

  public:
    ControllerInput(Controller& c, const SignalSettings& settings);

    /**
     * \brief Get the name of a particular signal as defined in the TOML file.
     * 
     * \return std::string 
     */
    std::string getName() { return name; }
    
    /**
     * \brief Get the enumeration for this signal 
     * 
     * \return ControllerSignal 
     *
     * This is the un-remapped signal
     */
    ControllerSignal getSignal() { return actual; }

    /**
     * \brief Get the appropriate ID for button or axis.
     * \param type Whether we want a button or an axis for hybrid controls.
     *
     * The type is ignored unless the input is a hybrid control (L2/R2), which sends both a button
     * and an axis signal. In this case, it is used to distinguish whether we want the button or
     * the axis id for hybrid types. For the type, you can pass the appropriate #ButtonType
     * enumeration.
     */
    uint8_t getID(uint8_t type) {
      if (input_type == ControllerSignalType::HYBRID) {
	      return (type == TYPE_BUTTON) ? button_id : hybrid_axis;
      }
      return button_id;
    }
    
    /**
     * \brief Get the button/axis id.
     * \return The ID of the signal as an unsigned integer
     *
     * For hybrid controls (L2/R2), this returns the ID of the button.
     */
    uint8_t getID() { return button_id; }

    /**
     * \brief Get the axis id of a hybrid control
     * \return The ID of the signal as an unsigned integer
     *
     * Used to get the ID of the axis signal for a hybrid control (L2/R2). It is a programming
     * error to call this for other types of controls. In that case, we note the error in the
     * log file and return the button ID, which is probably what we want anyway.
     */
    uint8_t getHybridAxis() { 
      if (input_type != ControllerSignalType::HYBRID) {
        PLOG_ERROR << "Call to getHybridAxis() for non-hybrid condrol. Returning the normal ID.\n";
        return button_id;
      }
      return hybrid_axis; 
      }

    /**
     * \brief Get the extended input type.
     * \return The category of input to which this event belongs.
     *
     * In order to remap different types of controls to one another, we distinguish the controller
     * events with different classes.
     */
    ControllerSignalType getType() { return input_type; }

    /**
     * \brief Translate a DeviceEvent into the extended #ControllerSignalType.
     * \param event The incoming event from the controller.
     *
     * This static function allows is to determine the category into which an incomming event falls
     * (button, axis, accelerometer, etc.).
     */
    static ControllerSignalType getType(const DeviceEvent& event);
    
    /**
     * \brief Get the basic button type (button vs. axis).
     *
     * In the case of hybrid inputs, we return TYPE_BUTTON.
     */
    ButtonType getButtonType() {
      return (input_type == ControllerSignalType::BUTTON || input_type == ControllerSignalType::HYBRID) ? TYPE_BUTTON : TYPE_AXIS;
    }

    /**
     * \brief Get the button index
     * 
     * \return idex
     * 
     * This returns the index to the button that we need to look up the position of the data about
     * this signal in the buffer maintained by ControllerState.
     */
    int getIndex() { return button_index; }

    int getHybridAxisIndex() { return hybrid_index; }
    
    /**
     * \brief Get the minimum value for this signal.
     * \param type Whether the signal is an axis or a button
     *
     * The type distinguishes the case for hybrid controls.
     */
    short getMin(ButtonType type);

    /**
     * \brief Get the maximum value for this signal.
     * \param type Whether the signal is an axis or a button
     *
     * The type distinguishes the case for hybrid controls.
     */
    short getMax(ButtonType type);

    /**
     * \brief Get the maximum value for this signal.
     *
     * For the hybrid controls, we return the maximum of the axis, because that's the value we need
     * to pass when building sequences.
     */
    static short getMax(std::shared_ptr<ControllerInput> signal);

    /**
     * \brief Constrain to joystick limits
     * 
     * \param input The unfiltered signal value
     * \return short The value clipped to the joystick's min/max
     */
    static short joystickLimit(int input) {
      return fmin(fmax(input, JOYSTICK_MIN), JOYSTICK_MAX);
    }

    /**
     * \brief Get the current state of the controller for this signal
     * \param hybrid_axis Return the axis component if this is a hybrid control
     * \return short Live controller state
     *
     * This examines the current state of the controller. If the signal is a HYBRID type and
     * hybrid_axis is false, the button value will be polled. If it is true, the axis value
     * will be polled. This value is ignored for other types.
     */
    short getState(bool hybrid_axis);

    /**
     * \brief Does the command match the incomming device event
     * 
     * \param event Raw device event
     * \return Does this signal match the incomming event.
     * 
     * For hybrid controls, returns true if the signal matches either the button or the axis
     * component.
     */
    bool matches(const DeviceEvent& event);
  };

};
