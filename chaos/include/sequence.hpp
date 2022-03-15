/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the AUTHORS file at
 * the top-level directory of this distribution for details of the contributers.
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
#include <vector>
#include <toml++/toml.h>
#include "controller.hpp"

namespace Chaos {

  /**
   * \brief Class to hold a sequence of emulated gampepad inputs that are sent as a batch.
   *
   * In a TOML file, sequences are used by various mod types. In each case, the sequence is defined
   * as an array of inline tables.
   *
   * The valid keys within each sequence step are the following:
   * - event: The type of signal event.
   * - command: The command that should be sent.
   */
  class Sequence {
  protected:
    std::vector<DeviceEvent> events;

    static unsigned int press_time;
    static unsigned int release_time;
    
    /**
     * \brief Adds a stack of events to set all the buttons and axes to zero.
     *
     * Needed (probably) to ensure that the user can't interfere with menuing selection.
     */
    void disableControls();
    
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
    void addPress(std::shared_ptr<GamepadInput> signal, short value);
    
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
    void addHold(std::shared_ptr<GamepadInput> signal, short value, unsigned int hold_time);
    /**
     * \brief Turns off the gamapad input signal.
     * \param signal The input signal the console expects to receive
     * \param hold_time Time to hold the signal in microseconds.
     */
    void addRelease(std::shared_ptr<GamepadInput> signal, unsigned int release_time);
    /**
     * \brief Add a delay
     * \param delay Time to delay in microseconds.
     *
     */
    void addDelay(unsigned int delay);

    static std::unordered_map<std::string, std::shared_ptr<Sequence>> sequences;

  public:

    Sequence() {}
    Sequence(toml::array& config);

    /**
     * \brief Main factory
     * 
     * \param config Fully parsed TOML object of the configuration file.
     *
     * Initializing the pre-defined sequences that can be referenced from other sequences and
     * the global accessor functions, e.g., for menu operations.
     */
    static void initialize(toml::table& config);

    static std::shared_ptr<Sequence> get(std::string& name);
    
    /**
     * Test if the event queue is empty
     */
    bool empty();
    /**
     * Send the precomposed sequence of events to the console.
     */
    virtual void send();
    /**
     * Send the sequence of events to the console.
     */
    virtual void send_incremental();
    /**
     * Empty the event queue
     */
    void clear();

    std::vector<DeviceEvent>& getEvents() { return events; }

    static int timePerPress() {
      return press_time;
    }
    
    static int timePerRelease() {
      return release_time;
    }
    
  };

  class SequenceRelative : public Sequence {
  private:
    int tickTime;

  public:
    SequenceRelative();

    void send();
    void setMinimumTickInMicroseconds(int minTickTime);
  };

  class SequenceAbsolute : public Sequence {
  private:
    int tickTime;

  public:
    SequenceAbsolute();

    void send();
    void setMinimumTickInMicroseconds(int minTickTime);
  };

};
