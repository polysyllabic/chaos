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
#include <toml++/toml.h>

#include "Modifier.hpp"
#include "Sequence.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  enum class SequenceState {UNTRIGGERED, STARTING, IN_SEQUENCE, ENDING};

  /** 
   * \brief Subclass of modifiers that executes arbitrary sequences of commands once. Seperate
   * sequences can be defined to execute at the start and end of the mod's lifetime.
   *
   * Mods of this type are defined in the TOML configuration file with the following keys:
   *
   * - name: A unique string identifying this mod. (_Required_)
   * - description: An explanatation of the mod for use by the chat bot. (_Required_)
   * - type = "disable" (_Required_)
   * - groups: A list of functional groups to classify the mod for voting. (_Optional_)
   * - type (required): "sequence"
   * - disableOnBegin: Game commands to disable (set to zero) when the mod is initialized.
   *  The value can be either an array of game commands or the single string "ALL". The latter
   *  is a shortcut to disable all defined game commands. (_Optional_)
   * - startSequence: A sequence of commands to execute once when the mod is enabled. This is an
   * array of inline arrays. See below for the format to define individual commands. (_Optional_)
   * - repeatSequence: A sequence of commands to execute repeatedly for the lifetime of the mod
   * This is an array of inline arrays. See below for the format to define individual commands.
   * - blockWhileBusy: Game commands whose signals from the user will be blocked while a sequence
   *  is being sent. This list is used to block signals in all contexts (start, repeat, finish).
   *  The value can be either an array of game commands or the single string "ALL". The latter is a
   *  shortcut to disable all defined game commands. This blocking is functionally different from
   *  the disableOnBegin and disableOnFinish keys in that those two actually set the listed signals
   *  to 0, whereas this list just prevents user input from altering the sequence as it is in the
   *  process of being sent. (_Optional_)
   * - disableOnFinish: Game commands to disable (set to zero) when the mod is removed from the
   *  active list. The value can be either an array of game commands or the single string "ALL".
   * The latter is a shortcut to disable all defined game commands. (_Optional_)
   * - finishSequence: A sequence of commands to execute once when the mod is removed. This is an
   *  array of inline arrays. See below for the format to define individual commands. (_Optional_)
   * - trigger: An array of commands whose state we monitor to trigger the sequence defined by
   * #repeatSequence. The threshold that 
   * commands pass the trigger threshold, an instance of the repeatSequence begins. This field is
   * mutually exclusive with triggerDistance. If neither trigger nor triggerDistance is specified,
   * the repeatSequence starts automatically.
   * - triggerDistance: _Optional_ An array of exactly two commands, assumed to be correspond to
   * the x- and y- axes of a joystick input. This is a mutually exclusive alternative to trigger.
   * If set, the trigger threshold is exceeded when the Pythagorean distance of the two axes
   * exceeds the threshold. If neither trigger nor triggerDistance is specified, the
   * repeatSequence starts automatically.
   * - triggerThreshold: A double, normally in the range 0 <= x <= 1 (but see ). of the absolute value of the signal. An incoming
   * signal must equal or exceed this magnitude to trigger the sequence. If not specified, the
   * threshold is 1.
   * - triggerDelay: Time in seconds to wait after the trigger (or after the timing of a cycle
   * begins if there is no trigger) before sending the repeatSequence. If omitted, the sequence
   * issues immediately after the trigger.
   * - cycleDelay: The time in seconds between the conclusion of one repeatSequence and the time
   * we reset the sequence and begin the next iteration (or begin waiting for the next trigger).
   * If omitted, the next cycle begins immediately.
   *
   * Sequences are build up of individual commands. Each command is defined in an inline table. The
   * following fields are recognized within this table:
   *
   * - event (_required_): The type of event to execute. Legal options are the following:
   *     - hold: turn the signal on indefinitely (emulate holding down a button)
   *     - release: turn of the signal (emulate releasing a button)
   *     - press: hold and release a button for a default amount of time.
   *     - disable: zero out all normal signals (buttons/axes)
   *     - delay: add a delay before the next command. (This delay can also be added to specific
   *       events without creating a new line with the delay field below.
   *     .
   * - command (_required)): The game command to apply the event to. Must be a comand defined in the
   * TOML file.
   * - repeat (_optional): Number of times to issue this command. Default if omitted = 1.
   * - delay (_optional_): Time in seconds to wait after this command before issuing the next one.
   * If the command is repeated, the delay applies to each iteration of the command. If omitted,
   * there is no delay before the next command.
   * - value (_optional_): Specific value to set the signal to for hold/press options, or seconds
   * to delay for the delay command.
   *
   * Example TOML definitions:
   *
   */
  class SequenceModifier : public Modifier::Registrar<SequenceModifier> {

  protected:
    /**
     * Sequence to execute at intervals throughout the lifetime of the mod.
     */
    std::shared_ptr<Sequence> repeat_sequence;

    /**
     * \brief The state of the repeat sequence
     * 
     * Repeat sequences require a multi-state condition, unlike begin and finish sequences, where
     * whether or not we're in a conditino is a boolean condition.
     */
    SequenceState sequence_state;

    /**
     * Time in seconds to wait after a cycle begins before issuing the repeat_sequence.
     */
    double start_delay;
    /**
     * Repeat the cycle after this amount of time (in seconds).
     */
    double repeat_delay;

    std::vector<std::shared_ptr<GameCommand>> block_while;
    
    double sequence_time;
    short sequence_step;

    void processSequence(Sequence& seq);
    
  public:
    SequenceModifier(toml::table& config, EngineInterface* e);

    static const std::string mod_type;
    const std::string& getModType() { return mod_type; }
    
    void begin();
    void update();
    bool tweak(DeviceEvent& event);
  };
};

