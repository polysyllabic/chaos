/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
 * file at the top-level directory of this distribution for details of the
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
#include <unordered_map>
#include <unordered_set>

#include <json/json.h>
#include <mogi/math/systems.h>
#include <toml++/toml.h>

#include "factory.hpp"

#include "controller.hpp"
#include "deviceEvent.hpp"
#include "gameCommand.hpp"
#include "gameCondition.hpp"
#include "touchpad.hpp"
#include "sequence.hpp"

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
   * The following keys are defined for all modifiers:
   *
   * - name: A unique string identifying this mod (_Required_)
   * - description: An explanatation of the mod for use by the chat bot (_Required_)
   * - type: The class of modifier (_Required_). The following modifier classes are currently implemented:
   *       - cooldown: Allow a command for a set time then force a recharge period before it can be used again.
   *       - delay: Wait for a set amount of time before sending a command.
   *       - disable: Block commands from being sent to the console.
   *       - formula: Alter the magnitude and/or offset of the signal through a formula
   *       - invert: Invert the sign of the signal.
   *       - menu: Apply a setting from the game's menus.
   *       - parent: A mod that contains other mods as children.
   *       - remap: Change which controls issue which commands.
   *       - repeat: Repeat a particular command at intervals.
   *       - sequence: Issue an arbitrary sequence of actions.
   *       .
   * - group: A group (separate from the type) to classify the mod for voting by the chatbot. IF
   * omitted, the mod will be assigned to the default "general" group. (_Optional_)
   *
   * Each type of modifier accepts a different subset of additional parameters. Some parameters,
   * however, are parsed by the general initialization routine called by all child classes and so
   * are available for any class that needs to use them. Note that the parent Modifier class parses
   * these entries, and provides variables to store the result and functions to perform the most
   * common actions, but child classes must implement the logic for handling this information and
   * determining its consequences for manipulating the signal stream.
   *
   * The following parameters are available for use by all child mods. Whether they are required,
   * optional, or unused by the child mods depends on the type of modifier:
   *
   * - appliesTo: A list of GameCommand objects affected by the mod.
   * - condition: A list of GameCondition objects whose total evaluation returns true or false.
   * Mods that use conditions should do something when the evaluation is true.
   * - unless: A list of GameCondition objects whose total evaluation returns true or false. The
   * evaluation of conditions occurs the same way Mods
   * that use condiditions should do something when the evaluation is false.
   * take place. The test of listed commands functions exactly as 'condition', but the commands are
   * disabled only if test of the conditions returns false.
   * - conditionTest: Type of test to perform on the conditions in the array.
   * - unlessTest: Type of test to perform on the conditions in the unless array.
   * - sequenceBegin:  A sequence of commands to issue when the mod is activated. Mods that want
   * to use this command can call sendBeginSequence() in their #begin routine. 
   * - sequenceFinish: A sequence of commands to issue when the mod is deactivated. Mods that want
   * to use this command can call sendFinishSequence() in their #finish routine.
   *
   * Both conditionTest and unlessTest accept the following values:
   * - all: All conditions must be true (for condition) or false (for unless) simultaneously
   * - any: Any one condition must be true (for condition) or false (for unless) at one time
   * - none: None of the defined conditions must be true (for condition) or false (for unless)
   * 
   * Additional parameters that are specific to individual types of mods are listed in the
   * documentation for those classes.
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
   *    description = "Be like Michael Jackson! Trying to walk forward will actually make you go backward."
   *    type = "invert"
   *    appliesTo = [ "move forward/back" ]
   *
   *    [[modifier]]
   *    name = "Touchpad Aiming"
   *    description = "No more aiming with the joystick. Finally making use of the touchpad!"
   *    type = "remap"
   *    signals = [ "RX", "RY" ]
   *    remap = [
   *          {from = "TOUCHPAD_ACTIVE", to="NOTHING"},
   *          {from = "TOUCHPAD_X", to="RX"},
   *          {from = "TOUCHPAD_Y", to="RY"},
   *          {from = "RX", to="NOTHING"},
   *          {from = "RY", to="NOTHING"}
   *          ]
   *
   * Specific types of modifiers are built in a self-registering factory. In order to ensure that
   * each child class is registered properly, the child class is prevented from inherited directly
   * from this one. Instead, you must inherit from the templated class Modifier::Registrar, putting
   * the name of the child class in the template.
   * 
   * The child class must have a public constructor that takes two parameters:
   * - A reference to a toml::table containing the TOML table that defines the modifier being
   * created
   * - A reference to a #Game object, providing access to game-data objects that have already been
   * initialized.
   * 
   * Modifiers are initialized last, so all general configuration parameters, the GameCommand,
   * GameCondition, #Sequence, and #Menu items, as well as the data on any modifiers already
   * initialized, are available to the constructor.
   * 
   * A modifier class also must declare a public static string named #mod_type, whose value is
   * declared in the source module and is unique among the different modifiers. This string will be
   * used in the value of the type field in TOML modifier definitions.
   * 
   * The the #Modifier class provides an #initialize() routine to perform initializaiton tasks from
   * the TOML configuration file that are used by many mod types. Because of the idiom used to create
   * the self-registering factory, this routine cannot be evoked with an explicit parent constructor
   * taking the toml::table object as a parameter. Instead, you should call this routine explicitly
   * from your constructor.
   *
   * The utility function checkValid() can be used to warn a user who uses an invalid key for the
   * TOML table you are checking. You must provide the calling function the toml table and a vector
   * of the legal key names for that table. If there are any key names in the table that are not
   * listed in this vector, a warning will be generated in the log file.
   * 
   *     // Sample header file samplemod.hpp for a modifier class
   *     #pragma once
   *     #include "modifier.hpp"
   *     #include "game.hpp"
   * 
   *     namespace Chaos {
   *       class SampleModifier : public Modifier::Registrar<SampleModifier> {
   *       public:
   *         static const std::string mod_type;
   *         SampleModifier(toml::table& config, Game& game);
   *       };
   *     };
   *
   *     // Sample source module samplemod.cpp for a modifier class 
   *     #include "samplemod.hpp"
   *     #include "tomlUtils.hpp"
   *     using namespace Chaos;
   * 
   *     SampleModifier::SampleModifier(toml::table& config, Game& game) {
   *       checkValid(config, std::vector<std::string>{"name", "description", "type", "other",
   *                  "keys", "to", "recognize"});
   *       initialize(config);
   *       // Initialization specific to this modifier goes here
   *     }
   */
  struct Modifier : Factory<Modifier, toml::table&, Game&>, public std::enable_shared_from_this<Modifier> {
    friend ChaosEngine;

  private:
    /**
     * \brief The parent modifier, if any
     * 
     * \todo Move this to ParentMod and overload getptr()
     */
    std::shared_ptr<Modifier> parent; 

  protected:
    /**
     * \brief Name of the mod as defined in the TOML file
     */
    std::string name;

    /**
     * Description of this mod for the use of the chat bot.
     */
    std::string description;

    /**
     * \brief A list of groups to which the mod belongs.
     *
     * Groups are used by the chaosface user interface to allow for more sophisticated mod
     * selections, for example to limit the number of mods in a particular group to 1 at a time.
     * The default, if no groups are specified in the TOML file, is the mod's type. Group names
     * can be arbitrary, and there is no encapsulation by mod type. In other words, you can
     * groupings within or across mod types.
     */
    std::unordered_set<std::string> groups;
    
    /**
     * \brief Running count of how long this mod has been active.
     */
    Mogi::Math::Time timer;

    /**
     * \brief The list of specific commands affected by this mod.
     *
     * This vector holds pointers to the list of GameCommand objects lised in the appliesTo
     * parameter of the TOML file. Note that all commands in this list are affected by the same
     * condition (if any). If you want two different commands to have two different conditions,
     * you should create two mods and declare them as children of a parent mod.
     */
    std::vector<std::shared_ptr<GameCommand>> commands;

    /**
     * \brief Flag to indicate mod applies to all commands.
     *
     * Instead of listing all commands separately, a mod can announce that it affects all game
     * commands. This allows us to avoid testing an entire list when the result will always be
     * true. This flag can be set from the TOML file by using the special keyword "ALL" in place
     * of an array of commands.
     */
    bool applies_to_all;

    /**
     * \brief A sequence of commands to execute when initializing the mod.
     *
     * The TOML key for this parameter is beginSequence, and the value must be an array of inline
     * arrays, each of which contains a sequence command. This can be used, for example, to ensure
     * that certain commands are set to 0. The format for sequences is described in the
     * documentation for the SequenceModifier class.
     */
    std::shared_ptr<Sequence> on_begin;
    /**
     * \brief A sequence of commands to execute when unloading the mod.
     *
     * The TOML key for this parameter is finishSequence, and the value must be an array of inline
     * arrays, each of which contains a sequence command. This can be used, for example, to ensure
     * that certain commands are set to 0. The format for sequences is described in the
     * documentation for the SequenceModifier class.
     */
    std::shared_ptr<Sequence> on_finish;

    /**
     * \brief The current state of any begin or finish sequence that is beeing issued
     */
    bool in_sequence;

    /**
     * \brief If true, lock out all events listed in appliesTo while sending a sequence.
     */
    bool lock_while_busy;

    /**
     * \brief If true, do not list the mod to the chatbot
     * 
     * This is used to declare child mods that should not appear independently for voting.
     */
    bool unlisted;
    /**
     * \brief List of game conditions to test
     *
     * The list of conditions is tested according to the rule in condition_test, and mods are expected
     * to take action if the result of the test is true.
     */
    std::vector<std::shared_ptr<GameCondition>> conditions;

    /**
     * \brief Rule to apply for the test of the #conditions list.
     *
     * Currently supports all, any, or none true. 
     */
    ConditionCheck condition_test;

    /**
     * \brief List of game conditions to test
     *
     * The list of conditions is tested according to the rule in unless_test, and mods are expected
     * to take action if the result of the test is true.
     */
    std::vector<std::shared_ptr<GameCondition>> unless_conditions;

    /**
     * \brief Rule to apply for the test of the #unless_conditions list.
     *
     * Currently supports all, any, or none true. 
     */
    ConditionCheck unless_test;

    /**
     * \brief Amount of time the engine has been paused.
     */
    double pauseTimeAccumulator;

    /**
     * \brief Designates a custom lifespan, if necessary.
     */
    double totalLifespan;
    
    static Controller& controller;
    static std::shared_ptr<ChaosEngine> engine;

    /**
     * \brief Common initialization.
     *
     * Because the registrar factory method is designed to prevent classes from invoking this
     * class as a direct base class, we can't put common initialization in the constructor here.
     * Instead, child classes should call this initialization routine in their own constructors.
     *
     * This will parse those parts of the TOML table definition that are used by all or many
     * mods. The fields handled here (which need not be re-parsed by the child class) are the
     * following:
     *
     * - description
     * - appliesTo
     * - condition
     * - gamestate
     *
     */
    void initialize(toml::table& config);

    /**
     * \brief Send any sequence intended to issue when the mod initializes.
     * 
     * Mods that support begining sequences should put a call to this function in their begin()
     * routine. If the sequence is empty, this will do nothing.
     */
    void sendBeginSequence();

    /**
     * \brief Send any sequence intended to issue when the mod is finishing.
     * 
     * Mods that support begining sequences should put a call to this function in their finish()
     * routine. If the sequence is empty, this will do nothing.
     */
    void sendFinishSequence();

/**
     * \brief Get a ConditionTest object corresponding to the type name in the TOML file
     * 
     * \param config Table that contains the condition test as a key/value pair
     * \param key Name of the key within the table
     * \return ConditionCheck 
     *
     */
    ConditionCheck getConditionTest(const toml::table& config, const std::string& key);

    /**
     * \brief Test if the current controller state matches the defined conditions
     * 
     * \param conditions A vector of the conditions to check
     * \param cond_type The type of test to apply to the conditions (all, any, none)
     * \return true 
     * \return false 
     */
    bool testConditions(std::vector<std::shared_ptr<GameCondition>> conditions, ConditionCheck cond_type);

    /**
     * \brief Update the state of any persistent game conditions.
     * 
     * This function should be called from the tweak routines of child modifiers that can track
     * persistent states.
     */
    void updatePersistent();

  public:
    /**
     * Name by which this mod type will be identified in the TOML file
     */
    static const std::string mod_type;
    /**
     * This constructor can only be invoked by the Registrar class
     */
    Modifier(Passkey) {}
    
    static void setEngine(std::shared_ptr<ChaosEngine> e) { engine = e; }
    static void setController(Controller& c) { controller = c; }

    /**
     * \brief Get the pointer to this mod if it's an ordinary mod, or its parent if it has one
     * 
     * \return std::shared_ptr<Modifier> 
     */
    std::shared_ptr<Modifier> getptr() {
      return parent ? parent : shared_from_this();
    }

    void setParentModifier(std::shared_ptr<Modifier> new_parent) { parent = new_parent; }
    
    bool isUnlisted() { return unlisted; }
    
    /**
     * \brief Get how long the mod has been active.
     * \return The mod's running time minus the accumulated time we've been paused.
     */
    double lifetime() { return timer.runningTime() - pauseTimeAccumulator; }
    double lifespan() { return totalLifespan; }
    /**
     * \brief Main entry point into the update loop
     * \param wasPaused Are we calling update for the first time after a pause?
     *
     * This function is called directly by the ChaosEngine class. We handle the timer housekeeping
     * and then call the virtual update function implemented by the concrete child class.
     */
    void _update(bool wasPaused);
  
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
     * \brief Commands to execute after another mod has executed its finish() routine and been
     * removed from the stack of active mods.
     *
     * This is currently used only by remap mods to clean up the remap table in the event that
     * more than one active mod is remapping things.
     */ 
    virtual void apply();

    /**
     * \brief Get the name of this mod as defined in the TOML file
     * 
     * \return std::string& 
     */
    std::string& getName() { return name; }

    /**
     * \brief Get a short description of this mod for the Twitch-bot.
     * \return The descrition of the mod
     *
     * \todo Internationalize the description strings.
     */
    std::string& getDescription() { return description; }

    /**
     * \brief Get the list of groups to which this modifier belongs is Json
     * 
     * \return Json::Value list of groups as a Json array
     */
    Json::Value getGroups();
    
    /**
     * \brief Commands to test (and potentially alter) events as necessary.
     * \param[in,out] event The event coming from the controller to test/alter.
     * \return true if the event is valid and should be passed on to other mods and the controller
     * \return false if the event should be dropped and not sent to other mods or the controller
     *
     * This command will be called for every event coming from the controller after any events have
     * been remapped.
     */
    virtual bool tweak(DeviceEvent& event);

    /**
     * \brief Checks the list of game conditions 
     * 
     * \return true if we're in the defined state, or if the list is empty
     * \return false if we're not in the defined state
     *
     * Traverses the list of GameCondition objects stored in #conditions. The state of 
     * #conditionTest determines the logic for how to chain together multiple conditions.
     *
     * If the #conditions list is empty, always returns true. In other words, defining no
     * conditions is equivalent to "always do this".
     */
    bool inCondition();
    
    /**
     * \brief Checks the list of negative game conditions 
     * 
     * \return true if we're in the defined state
     * \return false if we're not in the defined state, or if the list is empty
     *
     * Traverses the list of GameCondition objects stored in #unless_conditions. The state of 
     * #unlessTest determines the logic for how to chain together multiple conditions.
     *
     * Note that the test on a non-empty list is conducted the same way as the one for
     * inCondition(). For example, if #unlessTest is set to ConditionCheck::ALL and all
     * conditions are true, this function returns true. You should think of the return value
     * as answering the question "Are we in the state defined in this list?" and not "should we
     * take action?" The answer to the second question is one to be determined by the child
     * mod that makes use of this function.
     * 
     * If the #unless_conditions list is empty, always returns false.
     */
    bool inUnless();
    
    /**
     * \brief Get the metadata about this mod as a Json object
     * 
     * \return Json::Value 
     */
    Json::Value toJsonObject();

  };

};
