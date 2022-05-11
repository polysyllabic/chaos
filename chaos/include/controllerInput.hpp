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

#include "config.hpp"
#include "deviceEvent.hpp"
#include "signals.hpp"
#include "signalRemap.hpp"
#include "touchpad.hpp"

namespace Chaos {
  
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
     * This is the actual signal received from the controller. The GameCommand class defines a
     * mapping between a game command and this signal as being the controller signal that the
     * game actually expects to see.
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

    /**
     * \brief Current remapping state of this input
     */
    SignalRemap remap;

    /**
     * \brief Look up signal by enumeration
     */
    static std::unordered_map<ControllerSignal, std::shared_ptr<ControllerInput>> inputs;

    /**
     * \brief Lookup signal by TOML file name.
     */
    static std::unordered_map<std::string, std::shared_ptr<ControllerInput>> by_name;

    /**
     * \brief Lookup signal by DeviceEvent index.
     */
    static std::unordered_map<int, std::shared_ptr<ControllerInput>> by_index;

  public:
    ControllerInput(const SignalSettings& settings);

    /**
     * \brief Initialize the global maps and variables
     */
    static void initialize(const toml::table& config);

    /**
     * \brief Accessor for the class object identified by name.
     * \param name Name of the input
     * \return The ControllerInput object identified by the given string
     *
     * Used for processing the TOML file.
     */
    static std::shared_ptr<ControllerInput> get(const std::string& name);
    
    /**
     * \brief Get the ControllerInput object
     * \param signal The enumeration of the controller signal
     */
    static std::shared_ptr<ControllerInput> get(ControllerSignal signal);
    
    /**
     * \brief Get the ControllerInput object for the incoming signal from the controller.
     * \param event The event coming from the controller.
     */
    static std::shared_ptr<ControllerInput> get(const DeviceEvent& event);

    /**
     * \brief Get the name of a particular signal as defined in the TOML file.
     * 
     * \param signal The enumeration of the signal coming from the controller
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

/*
    void setID(uint8_t new_id) { button_id = new_id; }
    
    void setID(uint8_t new_button, uint8_t new_axis) {
      button_id = new_button;
      hybrid_axis = new_axis;
    } */

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

    int getIndex() { return button_index; }
    int getHybridAxisIndex() { return hybrid_index; }
    
    ControllerSignal getRemap() { return remap.to_console; }
    ControllerSignal getNegRemap() { return remap.to_negative; }

    short getThreshold() { return remap.threshold; }
    double getRemapSensitivity() { return remap.scale; }    
    bool remapInverted() { return remap.invert; }

    bool toMin() { return remap.to_min;}

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

    short joystickLimit(int input) {
      return fmin(fmax(input, JOYSTICK_MIN), JOYSTICK_MAX);
    }

    /**
     * \brief Directly set the remapping signal only.
     * \param remapping The signal that should go to the console
     */
    void setRemap(ControllerSignal remapping) { remap.to_console = remapping; }

    /**
     * Directly set all remapping information for this signal.
     * \param remapping The complete rmapping information for the signal to the console.
     */
    void setRemap(const SignalRemap& remapping) { remap = remapping; }

    /**
     * \brief Set a cascading remap.
     * \param remapping The complete rmapping information for the signal to the console.
     *
     * Before setting the remap, checks to see if the remapping signal is already the to-signal from
     * some other input. If the to part of the remap from portion of the remapped signal is already
     * in use, we cascade the remap by changing making the to-signal of the other input the to-signal
     * for the new one. For example, if a previous mod maps ACCX -> LX, and the new mod maps LX -> RX,
     * the unified map will be, in effect, ACCX -> LX -> RX, simplified to ACCX -> RX.
     */
    void setCascadingRemap(const SignalRemap& remapping);

    /**
     * \brief Tests if an event matches this signal
     * \param event The incomming event from the controller
     * \param gp The imput signal we're testing for
     * \return Whether event id and type match the definition for this controller input.
     *
     * Remapping should be complete before the ordinary (non-remapping) mods see the event, so
     * it is safe to test for the actual events that the console expects.
     */
    static bool matchesID(const DeviceEvent& event, ControllerSignal gp);

    /**
     * \brief Get the current state of the controller for this signal
     * 
     * \return short Live controller state
     *
     * This examines the current state of the controller for the actual signal, not its remapped
     * state.
     */
    short getState();

    /**
     * \brief Get the current state of the remapped signal
     * 
     * \return short 
     *
     * We check the current state of the remapped signal or signals for this controller. The value
     * of the remapped state is translated to the equivalent value it would have as the actual
     * signal so that the value here can safely be used to check condition thresholds against the
     * originally established threshold.
     */
    short getRemappedState();

    /**
     * \brief Translate a value for the device event to its remapped value
     * 
     * \param value The value as given by the source signal
     * \return Remapped value
     *
     * Takes a signal value and scales it, if necessary, to the value specified in the remap table.
     */
    short remapValue(short value);

    /**
     * \brief Translate a value for the device event to its remapped value
     * 
     * \param signal The remapped signal to check
     * \param value The value as given by the source signal
     * \return Remapped value
     *
     * Takes a signal value from a specific source signal and scales it, if necessary, to the value
     * it has when remapped to this signal type.
     */
    short remapValue(std::shared_ptr<ControllerInput> source, short value);
  };

};
