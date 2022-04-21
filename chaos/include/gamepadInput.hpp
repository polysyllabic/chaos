/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file
 * in top-level directory of this distribution for a list of the contributers.
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
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <plog/Log.h>

#include "deviceEvent.hpp"

namespace Chaos {

  /**
   * \brief An enumeration of the possible types of gamepad inputs.
   *
   * This provides a unified enumeration of all buttons and axes, as opposed to the separate ones
   * defined in deviceTypes.hpp. This enumeration serves as the index to the vector of game
   * commands. The value NONE is used to indicate that no remapping is in effect.
   * The value NOTHING is effectively equivalent to remapping to /dev/null. In other words, a
   * remap to NOTHING has the effect of dropping the signal rather than remapping it to a different
   * one.
   */
  typedef enum GPInput {
    X, CIRCLE, TRIANGLE, SQUARE,
    L1, R1, L2, R2,
    SHARE, OPTIONS, PS,
    L3, R3,
    TOUCHPAD, TOUCHPAD_ACTIVE, TOUCHPAD_ACTIVE_2,
    LX, LY, RX, RY,
    DX, DY,
    ACCX, ACCY, ACCZ,
    GYRX, GYRY, GYRZ,
    TOUCHPAD_X, TOUCHPAD_Y,
    TOUCHPAD_X_2, TOUCHPAD_Y_2,
    NONE, NOTHING
  } GPInput;

  /**
   * Specific type of gampad input. This is more specific than the basic ButtonType for hints
   * when remapping controls and other occasions where specific subtypes of gamepad inputs
   * require differnt handling.
   */
  enum class GPInputType {
    BUTTON,
    THREE_STATE,
    AXIS,
    HYBRID,
    ACCELEROMETER,
    GYROSCOPE,
    TOUCHPAD
  };

  /**
   * Contains the information to remap signals between the controller and the console.
   */  
  struct SignalRemap {
    /**
     * The basic input type received from the controller
     */
    GPInput from_controller;
    /**
     * \brief The input type that the controller to which the input will be altered before it is
     * sent to the console.
     *
     * If no remapping is defined, this will be set to GPInput::NONE. GPInput::NOTHING is a
     * remapping that drops the signal. For remapping of an axis to multiple buttons, this contains
     * the remap for positive axis values.
     */
    GPInput to_console;
    /**
     * The remapped control used for negative values when mapping one input onto multiple buttons for
     * output.
     */
    GPInput to_negative;
    /**
     * If true, button-to-axis remaps go to the joystick minimum. Otherwise they go to the maximum.
     */
    bool to_min;
    /**
     * If true, invert the value on remapping.
     */
    bool invert;
    /**
     * \brief Axis threshold at which to treat remapped buttons as on.
     *
     * If the absolute value of the incomming axis event equals or exceeds the threshold, the
     * remapped button is treated as on. Values less than the threshold are treated as button-off.
     */
    int threshold;
    /**
     * \brief A sensitivity factor to be applied when remapping a signal.
     *
     * The signal from the controller will be divided by this value. This is used in remapping
     * gyroscope signals to the axes. Question to explore: is it worth making this a general scaling
     * factor, multiplying by a double value? We could then get rid of a separate test for invert
     * and implement it as scale=-1. The down side is that we would need to do the math in floating
     * point and then convert for every signal. Is the generalized functionality worth the
     * additional overhead?
     */
    int sensitivity;
    SignalRemap(GPInput from, GPInput to, GPInput neg_to, bool min, bool inv, int thresh, int sc) :
      from_controller(from), to_console(to), to_negative(neg_to),
      to_min(min), invert(inv), threshold(thresh), sensitivity(sc) {}
  };
  
  /**
   * \brief Defines the nature of a particular signal coming from the controller (i.e., gamepad).
   *
   * Signals are 
   */
  class GamepadInput {
  private:
    
    /**
     * The primary id (the only one for all but hybrid buttons)
     */
    uint8_t button_id;
    /**
     * The axis id for hybrid buttons (L2/R2).
     */
    uint8_t hybrid_axis;
    /**
     * \brief The signal's class.
     *
     * This subdivides button and axis so that we can handle rules for categories of signals that
     * need special treatment (e.g., the touchpad or the hybrid button/axis inputs L2 and R2).
     */
    GPInputType input_type;
    /**
     * \brief The numerical index of the signal.
     *
     * For the hybrid signals L2/R2, this is the button id. For all others, it is their sole id.
     */
    int button_index;
    /**
     * \brief The numerical index of the axis signal for hybrid controls.
     */
    int hybrid_index;
    /**
     * \brief Current remapping state of this signal
     */
    SignalRemap remap;

    /**
     * Map names of buttons to their enumeration (for TOML file)
     */
    static std::unordered_map<std::string, GPInput> inputNames;
    
    /**
     * Vector to look up signals coming from the controller.
     */
    static std::vector<std::shared_ptr<GamepadInput>> inputs;

    /**
     * Map to lookup the remapping information for this input by DeviceEvent index.
     */
    static std::unordered_map<int, GPInput> by_index;

  public:
    GamepadInput(GPInput gp, GPInputType gp_type, uint8_t id, uint8_t hybrid_id);
    GamepadInput(GPInput gp, GPInputType gp_type, uint8_t id) : GamepadInput(gp, gp_type, id, 0) {}

    GPInput getSignal() { return remap.from_controller; }

    /**
     * \brief Accessor for the class object identified by name.
     * \param name Name of the input
     * \return The gamepad input identified by the given string
     *
     * Used for processing the TOML file.
     */
    static std::shared_ptr<GamepadInput> get(const std::string& name);
    
    /**
     * \brief Get the GamepadInput object for the incoming signal from the controller.
     * \param gp The enumeration of the gamepad signal
     */
    static std::shared_ptr<GamepadInput> get(GPInput gp) { return inputs[gp]; }
    
    /**
     * \brief Get the GamepadInput object for the incoming signal from the controller.
     * \param event The event coming from the controller.
     */
    static std::shared_ptr<GamepadInput> get(const DeviceEvent& event) {
      GPInput gp = by_index[(int)event.index()];
      return inputs[gp];
    }
    
    void setID(uint8_t new_id) { button_id = new_id; }
    
    void setID(uint8_t new_button, uint8_t new_axis) {
      button_id = new_button;
      hybrid_axis = new_axis;
    }

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
      if (input_type == GPInputType::HYBRID) {
	      return (type == TYPE_BUTTON) ? button_id : hybrid_axis;
      }
      return button_id;
    }
    
    /**
     * \brief Get the button/axis id.
     * \return The ID of the signal as an unsigned integer
     *
     * For hybrid controls (L2/R2) this returnd the ID of the button.
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
      if (input_type != GPInputType::HYBRID) {
        PLOG_ERROR << "Call to getHybridAxis() for non-hybrid condrol. Returning the normal ID.\n";
        return button_id;
      }
      return hybrid_axis; 
      }

    /**
     * \brief Get the extended input type.
     * \return The category of input to which this event belongs.
     *
     * In order to remap different types of controls to one another, we distinguish the gamepad events
     * into different classes.
     */
    GPInputType getType() { return input_type; }

    /**
     * \brief Translate a DeviceEvent into the extended #GPInputType.
     * \param event The incoming event from the controller.
     *
     * This static function allows is to determine the category into which an incomming event falls
     * (button, axis, accelerometer, etc.).
     */
    static GPInputType getType(const DeviceEvent& event);
    
    /**
     * \brief Get the name of a particular signal as defined in the TOML file.
     * 
     * \param signal The enumeration of the signal coming from the controller
     * \return std::string 
     *
     *
     */
    static std::string getName(GPInput signal);

    std::string getName();
    
    /**
     * Get the basic button type (button vs. axis). In the case of hybrid
     * inputs, we return the button.
     */
    ButtonType getButtonType() {
      return (input_type == GPInputType::BUTTON || input_type == GPInputType::HYBRID) ? TYPE_BUTTON : TYPE_AXIS;
    }

    int getIndex() { return button_index; }
    int getHybridAxisIndex() { return hybrid_index; }
    
    GPInput getRemap() { return remap.to_console; }
    GPInput getNegRemap() { return remap.to_negative; }
    bool remapInverted() { return remap.invert; }
    bool toMin() { return remap.to_min;}
    int getRemapThreshold() { return remap.threshold; }
    int getRemapSensitivity() { return remap.sensitivity; }

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
    static short getMax(std::shared_ptr<GamepadInput> signal);
    /**
     * \brief Directly set the remapping signal only.
     * \param remapping The signal that should go to the console
     */
    void setRemap(GPInput remapping) { remap.to_console = remapping; }
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
     * \return Whether event id and type match the definition for this gamepad input.
     *
     * Remapping should be complete before the ordinary (non-remapping) mods see the event, so
     * it is safe to test for the events that the console expects.
     */
    static bool matchesID(const DeviceEvent& event, GPInput gp);

    /**
     * \brief Get the current state of the controller for this signal
     * 
     * \return short 
     *
     * When checking the current state, we look at the remapped signal. That is, we case about what
     * the user is actually doing, not what signal is going to the console.
     */
    short int getState();
  };

};
