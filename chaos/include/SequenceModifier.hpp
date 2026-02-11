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
#include <timer.hpp>

#include "Modifier.hpp"
#include "Sequence.hpp"
#include "EngineInterface.hpp"

namespace Chaos {

  enum class SequenceState {UNTRIGGERED, STARTING, IN_SEQUENCE, ENDING};

  /** 
   * \brief Subclass of modifiers that executes arbitrary sequences of commands once. Seperate
   * sequences can be defined to execute at the start and end of the mod's lifetime.
   *
   * The TOML syntax defining menu items is described in chaosConfigFiles.md
   *
   */
  class SequenceModifier : public Modifier::Registrar<SequenceModifier> {

  protected:
    /**
     * Sequence to execute at intervals throughout the lifetime of the mod.
     */
    std::shared_ptr<Sequence> repeat_sequence;

    /**
     * Conditions to monitor to that will trigger the repeat_sequence
     */
    std::vector<std::shared_ptr<ControllerInput>> trigger;

    /**
     * \brief The state of the repeat sequence
     * 
     * Repeat sequences require a multi-state condition, unlike begin and finish sequences, where
     * whether or not we're in a condition is a boolean condition.
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

    /**
     * Commands to block while executing the sequences
     */
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

