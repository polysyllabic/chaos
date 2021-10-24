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
#include <memory>
//#include <functional>
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
   * Specific types of modifiers are built in a self-registering factory. In order to ensure that
   * each child class is registered properly, that class cannot inherit directly from this one.
   * Instead, you must inherit from the Modifier::Registrar, with the name of the child class in
   * the template.:
   *
   *     class ChildModifier : public Modifier::Registrar<ChildModifier>
   *
   * The child class's public constructor must take a const refernce to a toml table as the
   * only parameter, and it should pass this to the parent constructor with the same
   * parameter.
   *
   *    ChildModifier::DisableModifier(const toml::table& config) : Modifier(config) {
   *      // Initialization specific to the child modifier here
   *    }
   *
   * The parent constructor will initialize the data for the parts of the modifier that are common
   * to multiple types. The following fields are recognized and need not be reprocessed by the
   * child class:
   * - description
   * - appliesTo
   */
  struct Modifier : Factory<Modifier, const toml::table&>, public std::enable_shared_from_this<Modifier> {
    friend ChaosEngine;

  protected:
    std::string description;
    
    Mogi::Math::Time timer;

    inline static Controller* controller;
    inline static ChaosEngine* engine;

    /**
     * \brief The list of specific commands affected by this mod.
     *
     * Note that all commands in this list are affected by the same condition (if any). If you want
     * two different commands to have two different conditions, you should create two mods and
     * declare them as children of a parent mod.
     */
    std::vector<std::shared_ptr<GameCommand>> commands;
    
    /**
     * Contains "this" except in cases where there is a parent modifier.
     */
    std::shared_ptr<Modifier> me; 
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
    inline static std::unordered_map<std::string, std::shared_ptr<Modifier>> mod_list;

    /**
     * \brief Common initialization.
     *
     * Because of the registrar factory method is designed to prevent classes from invoking this
     * class as a direct base class, we can't put common initialization in the constructor here.
     * Instead, child classes should call this initialization routine in their own constructors.
     *
     * This will parse those parts of the TOML table definition that are used by all or many
     * mods. The fields handled here (which need not be re-parsed by the child class) are the
     * following:
     *
     * - description
     * - appliesTo
     *
     */
    void initialize(const toml::table& config);
    
  public:
    static const std::string name;
    /**
     * This constructor can only be invoked by the Registrar class
     */
    Modifier(Passkey) {}
    
    static void setController(Controller* contr);
    static void setEngine(ChaosEngine* eng);
    
    /**
     * Create the overall list of mods from the TOML file.
     */
    static void buildModList(toml::table& config);
    
    static std::string getModList();
	
    inline void setParentModifier(std::shared_ptr<Modifier> parent) { me = parent; }
    
    /**
     * \brief Get how long the mod has been active.
     * \return The mod's running time minus the accumulated time we've been paused.
     */
    inline double lifetime() { return timer.runningTime() - pauseTimeAccumulator; }
    inline double lifespan() { return totalLifespan; }
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
     * \brief Commands to execute when the mod is first applied. 
     */
    virtual void begin();
    /**
     * \brief Commands to execute at fixed intervals throughout the lifetime of the mod.
     */
    virtual void update();
    /**
     * \brief Commands to execute when removing the mod from the active-mod list.
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
     * passed on for other modifiers to alter.
     *
     */
    virtual bool tweak(DeviceEvent* event);
	
  };

};
