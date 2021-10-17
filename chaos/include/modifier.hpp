/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021 The Twitch Controls Chaos developers. See the COPYRIGHT
 * file at the top-level directory of this distribution.
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
#include <functional>
#include <unordered_map>
#include <json/json.h>
#include <mogi/math/systems.h>

#include "modifierFactory.hpp"

#include "controller.hpp"
#include "gameCommand.hpp"
#include "deviceTypes.hpp"

namespace Chaos {

  class ChaosEngine;

  /**
   * \brief Definition of the abstract base Modifier class for TCC.
   *
   * A modifier is a specific alteration of a game's operation, for example disabling a particular
   * control, or inverting the direction of a joystick movement. Specific classes of modifiers
   * are implemented as subclasses.
   *
   * Individual modifiers are objects of a particular child class of Modifier and are created
   * during initialization based on a definition in the TOML configuration file.
   *
   * Within the TOML file, each mod is defined in a [[modifier]] table.
   * The following keys are required for all modifiers:
   *
   * - name: A unique string identifying this mod.
   * - description: An explanatation of the mod for use by the chat bot.
   * - type: the subtype of modifier.
   *
   * The following modifier types are currently defined:
   *
   * - cooldown: Allow a command for a set amount of time and then force a recharge period
   *   before it can be used again.
   * - delay: Wait for a set amount of time before sending a command.
   * - disable: Block a command or commands from being sent to the console.
   * - formula: Alter the magnitude and/or offset of the signal through a formula
   * - menu: Apply a setting from the game's menus.
   * - remap: Change which controls issue which commands.
   * - sequence: Apply an arbitrary sequence of actions.
   *
   * Each type of modifier accepts a different subset of additional parameters. See the
   * specific classes for details. 
   * 
   * Example TOML definitions:
   *
   *    [[modifier]]
   *    name = "Moose"
   *    description = "The moose is dead. The moose does not move. (Disables movement)"
   *    type = "disable"
   *    appliesTo = [ "move forward/back", "move sideways" ]
   * 
   *    [[modifier]]
   *    name = "Moonwalk"
   *    description = "Be like Michael Jackson! Trying to walk forward will actually make you go backward"
   *    type = "formula"
   *    magnitude = -1
   *    appliesTo = [ "move forward/back" ]
   *
   * Specific types of modifiers are built in a self-registering factory. In order to ensure that each
   * child class is registered properly, that class cannot inherit directly from this one. Instead, you
   * must define the constructor to invoke Modifier::Register<ChildClass>.
   *
   */
  struct Modifier : Factory<Modifier, Controller*, ChaosEngine*, const toml::table&> {
    friend ChaosEngine;

  protected:
    std::string description;
    
    Mogi::Math::Time timer;

    Controller* controller;
    ChaosEngine* engine;

    /**
     * \brief The list of specific commands affected by this mod.
     *
     * Note that all commands in this list are affected by the same condition (if any). If you want
     * two different commands to have two different conditions, you should create two mods and
     * declare them as children of a parent mod.
     */
    std::vector<GameCommand*> commands;
    
    // Contains "this" except in cases where there is a parent modifier.
    Modifier* me; 
    /**
     * Amount of time the engine has been paused.
     */
    double pauseTimeAccumulator;
    /**
     * Designates a custom lifespan, if necessary.
     */
    double totalLifespan;
    
    Json::Value toJsonObject(const std::string& name);

    /**
     * The map of all the mods defined through the TOML file
     */
    static std::unordered_map<std::string, Modifier*> mod_list;
    
  public:
    static const std::string name;

    Modifier(Passkey) {}
    
    /**
     * Common initialization of the modifier.
     * Parts of the TOML table are used by all child classes, so we provide a common routine
     * to initialize it here.
     *
     */
    void initialize(Controller* controller, ChaosEngine* engine, const toml::table& config);

    static std::string getModList();
	
    inline void setParentModifier(Modifier* parent) { this->me = parent; }
    
    /**
     * \brief Get how long the mod has been active.
     * \return The mod's running time minus the accumulated time we've been paused.
     */
    inline double lifetime() { return timer.runningTime() - pauseTimeAccumulator; }
    double lifespan();
    /**
     * \brief Main entry point into the update loop
     * \param Is the chaos engine currently paused?
     *
     * This function is called directly by the ChaosEngine class. We handle the timer housekeeping and
     * then call the virtual update function implemented by the concrete child class.
     * 
     */
    void _update(bool isPaused);
	
    /**
     * Commands to execute when the mod is first applied. The default implementation does nothing.
     */
    virtual void begin();
    /**
     * Commands to execute at fixed intervals throughout the lifetime of the mod. The default
     * implementation does nothing.
     */
    virtual void update();
    /**
     * Commands to execute when removing the mod from the active-mod list. The default
     * implementation does nothing.
     */
    virtual void finish();

    /**
     * \brief Get a short description of this mod for the Twitch-bot.
     * \return The descrition of the mod.
     *
     * TO DO: Internationalize these strings.
     */
    inline const std::string& getDescription() const noexcept { return description; }

    /**
     * \brief Commands to test (and potentially alter) events as necessary.
     * \param[in,out] event The event coming from the gamepad to test/alter.
     * \return A boolean indication of whether the event is valid.
     *
     * Returning true has the effect of passing the event on to the next modifier (if any) in the
     * list of active modifiers. Returning false effectively dropps the event, and it will not be
     * passed on for other modifiers to affect.
     *
     * The default implementation simply returns true.
     */
    virtual bool tweak( DeviceEvent* event );
	
  };

};
