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
#include <memory>
#include <vector>
#include "DeviceEvent.hpp"

#define SEC_TO_MICROSEC 1000000.0

namespace Chaos {
  class Controller;
  class ControllerInput;

  /**
   * \brief Class to hold a sequence of emulated gampepad inputs that are sent as a batch.
   *
   * In a TOML file, sequences are used by various mod types. In each case, the sequence is defined
   * as an array of inline tables.
   *
   * The valid keys within each sequence step are the following:
   * - event: The type of signal event.
   * - command: The command that should be sent.
   * 
   * \todo: Use the chrono library to ensure our time units are consistent
   */
  class Sequence {
  protected:
    std::vector<DeviceEvent> events;
    Controller& controller;

    // to track current stage for sending the sequence in parallel
    short current_step = 0;

    // running tracker of elapsed time in microseconds to wait before sending the next sequence
    // in parallel
    unsigned int wait_until = 0;

    // Time in microseconds to hold down a signal for a button press
    static unsigned int press_time;

    // Time in microseconds to release a signal for a button press before going on to the next command.
    static unsigned int release_time;

  public:
    Sequence(Controller& c);

    static void setPressTime(double time);
    static void setReleaseTime(double time);

    /**
     * \brief Directly add an individual event to the sequence stack
     * 
     * \param event 
     */
    void addEvent(DeviceEvent event);
    
    /**
     * \brief Emulates a press-and-release event for a gamapad input signal.
     * \param signal The input signal the console expects to receive
     * \param value The value of the input signal
     *
     * A value of 0 means that the default maximum will be used when emulating the signal press.
     * This is 1 for buttons and the d-pad, and the joystick maximum for other axes. For the hybrid
     * controls L2/R2, a non-zero value is passed to the axis portion of the signal. The button
     * portion is set to 1 regardless. The length of time for the button press and release are set
     * by the default TIME_PER_BUTTON_PRESS and TIME_PER_BUTTON_RELEASE parameters.
     */
    void addPress(std::shared_ptr<ControllerInput> signal, short value);
    
    /**
     * \brief Emulates holding down the gamapad input signal in the fully on position.
     * \param signal The input signal the console expects to receive
     * \param value The value of the input signal.
     * \param hold_time Time to hold the signal in microseconds.
     *
     * A value of 0 means that the default maximum will be used when emulating the signal press.
     * This is 1 for buttons and the d-pad, and the joystick maximum for other axes. For the hybrid
     * controls L2/R2, a non-zero value is passed to the axis portion of the signal. The button
     * portion is set to 1 regardless.
     */
    void addHold(std::shared_ptr<ControllerInput> signal, short value, unsigned int hold_time);
    /**
     * \brief Turns off the gamapad input signal.
     * \param signal The input signal the console expects to receive
     * \param hold_time Time to hold the signal in microseconds.
     */
    void addRelease(std::shared_ptr<ControllerInput> signal, unsigned int release_time);
    /**
     * \brief Add a delay
     * \param delay Time to delay in seconds
     *
     */
    void addDelay(unsigned int delay);
    //void addDelay(double delay);

    void addSequence(std::shared_ptr<Sequence> seq);

    /**
     * Test if the event queue is empty
     */
    bool empty();
    /**
     * Send the precomposed sequence of events to the console.
     */
    virtual void send();
    /**
     * Empty the event queue
     */
    void clear();

    std::vector<DeviceEvent>& getEvents() { return events; }

    /**
     * \brief Return the nth step in the sequence of events
     * 
     * \param sequenceTime 
     * \return true if sequence is done
     * \return false if sequence is still in progress
     */
    bool sendParallel(double sequenceTime);
    
  };

};
