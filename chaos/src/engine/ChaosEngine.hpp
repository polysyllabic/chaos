/*
 * Twitch Controls Chaos (TCC)
 * Copyright 2021-2026 The Twitch Controls Chaos developers. See the AUTHORS
 * file in the top-level directory of this distribution for a list of the
 * contributors.
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
#include <filesystem>
#include <thread.hpp>
#include <atomic>
#include <memory>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <chrono>
#include <json/json.h>
#include <timer.hpp>

#include "ChaosInterface.hpp"
#include "Controller.hpp"
#include "Modifier.hpp"
#include "Game.hpp"

namespace Chaos {

  /**
   * \brief The engine that adds and removes modifiers at the appropraite time.
   * 
   * The chaos engine listens to the Python voting system using ZMQ with zmqpp. When a winning modifier
   * comes in, chaos engine adds the modifier to list of active modifiers. After a set amount of time,
   * the chaos engine will remove the modifier.
   */
  class ChaosEngine : public CommandObserver, public ControllerInjector,
                      public Thread, public EngineInterface {
  private:
    ChaosInterface chaosInterface;

    Controller& controller;	

    Timer time;

    // Data for the game we're playing
    Game game;

    /**
     * The list of currently active modifiers
     */
    std::list<std::shared_ptr<Modifier>> modifiers;

    /**
     * List of modifiers that have been selected but not yet initialized.
     */
    std::list<std::shared_ptr<Modifier>> modifiersThatNeedToStart;

    /**
     * List of active modifiers that have been requested for removal.
     *
     * Removal requests are drained on the engine thread to avoid update/finish overlap.
     */
    std::list<std::shared_ptr<Modifier>> modifiersThatNeedToStop;
	
    std::atomic<bool> keep_going{true};
    std::atomic<bool> pause{true};
    std::atomic<bool> game_ready{false};
    std::atomic<bool> awaiting_game_selection{false};
    std::atomic<bool> paused_for_interface_timeout{false};
    std::atomic<bool> resume_after_interface_reconnect_requested{false};
    std::atomic<bool> menu_navigation_active{false};
    std::atomic<bool> menu_navigation_transitioning{false};
    std::atomic<unsigned int> controller_dispatch_inflight{0};
    bool pausePrimer = false;
    bool pausedPrior = false;
    int primary_mods = 0;
    bool interface_enabled{true};
    std::string default_mod_list_path;
    std::filesystem::path game_config_directory;
    std::string current_game_config_path;
    std::string current_game_mod_list_uri;
    std::unordered_map<std::string, std::string> available_game_configs;
    Json::Value available_games_payload{Json::arrayValue};
    std::chrono::steady_clock::time_point next_game_announcement{};
    std::atomic<bool> awaiting_available_games_ack{false};
    
    // overridden from ControllerInjector
    bool sniffify(const DeviceEvent& input, DeviceEvent& output);
    bool dispatchControllerEvent(const DeviceEvent& event,
                                 const std::function<void(const DeviceEvent&)>& apply,
                                 bool allow_during_menu = false) override;
    bool prefersRawPassthrough() const override { return pause.load(); }

    // overridden from Thread
    void doAction();

    Json::CharReaderBuilder jsonReaderBuilder;
    Json::CharReader* jsonReader;
    Json::StreamWriterBuilder jsonWriterBuilder;	

    void reportGameStatus();
    void reportAvailableGames();
    void reportEngineStatus();
    void reportCommandResult(const std::string& kind, bool ok, const std::string& message,
                             const std::string& command_id = "",
                             const std::string& target = "");
    std::string currentEngineStatusLocked();
    std::string resolveGameConfig(const std::string& selection);
    std::string resolveModListUri(const std::string& configured_uri) const;
    bool handleUploadedGameConfigs(const Json::Value& payload, std::string& message);
    void clearPendingInjectedEventsForMenu();
    void beginMenuNavigation();
    void endMenuNavigation();

    void removeMod(std::shared_ptr<Modifier> mod);

  public:
    /**
     * \brief Construct the chaos engine and bind interface endpoints.
     *
     * \param c Controller instance used for output.
     * \param listener_endpoint Endpoint for inbound interface commands.
     * \param talker_endpoint Endpoint for outbound interface messages.
     * \param enable_interface Whether the external interface should be enabled.
     * \param default_mod_list_uri_base Base URI for resolving relative mod-list links.
     * \param runtime_game_directory Directory used for discoverable game config files.
     */
    ChaosEngine(Controller& c, const std::string& listener_endpoint,
                const std::string& talker_endpoint, bool enable_interface = true,
                const std::string& default_mod_list_uri_base = "",
                const std::string& runtime_game_directory = "");
    
    /**
     * \brief Send a serialized message to the external interface.
     *
     * \param msg Serialized payload.
     */
    void sendInterfaceMessage(const std::string& msg);

    /**
     * \brief Set the list of available game configurations.
     *
     * \param games Pairs of {display_name, config_path}.
     */
    void setAvailableGames(const std::vector<std::pair<std::string, std::string>>& games);

    /**
     * \brief Broadcast the currently available game list to the interface.
     */
    void announceAvailableGames();

    /**
     * \brief Initialize the game data from the supplied configuration file
     * 
     * \param name File name of the TOML configuration file holding the definitions for the game to be played.
     */
    bool setGame(const std::string& name);

    /**
     * \brief Get the list of mods that are currently active
     * 
     * \return std::list<std::shared_ptr<Modifier>> 
     */
    std::list<std::shared_ptr<Modifier>>& getActiveMods() { return modifiers; }

    /**
     * \brief Insert a new event into the event queue
     * 
     * \param event The fake event to insert
     * \param sourceMod The modifier that inserted the event
     * 
     * This command is used to insert a new event in the event pipeline so that other mods in the
     * active list can also act on it.
     */
    void fakePipelinedEvent(DeviceEvent& event, std::shared_ptr<Modifier> sourceMod);

    /**
     * \brief Remove mod with the least time remaining
     * 
     * This removes the oldest mod without consideration for the number of mods on the stack.
     */
    void removeOldestMod();

    /**
     * \brief Read the current raw controller state by type/id.
     */
    short getState(uint8_t id, uint8_t type) { return controller.getState(id, type); }

    /**
     * \brief Is the event an instance of the specified input command?
     */
    bool eventMatches(const DeviceEvent& event, std::shared_ptr<GameCommand> command);

    /**
     * \brief Force a command to its "off" state on the controller.
     */
    void setOff(std::shared_ptr<GameCommand> command);
    
    /**
     * \brief Force a command to its "on" state on the controller.
     */
    void setOn(std::shared_ptr<GameCommand> command);

    /**
     * \brief Set a command to an explicit value on the controller.
     */
    void setValue(std::shared_ptr<GameCommand> command, short value);

    /**
     * \brief Apply a raw event directly to controller output.
     */
    void applyEvent(const DeviceEvent& event) override;

    /**
     * \brief Lookup a modifier by name from the loaded game.
     */
    std::shared_ptr<Modifier> getModifier(const std::string& name) {
      return game.getModifier(name);
    }

    /**
     * \brief Access the game's modifier map.
     */
    std::unordered_map<std::string, std::shared_ptr<Modifier>>& getModifierMap() {
      return game.getModifierMap();
    }

    /**
     * \brief Lookup a menu item by name from the loaded game.
     */
    std::shared_ptr<MenuItem> getMenuItem(const std::string& name) {
      return game.getMenu().getMenuItem(name);
    }

    /**
     * \brief Request a clean engine shutdown from the main loop.
     */
    void requestShutdown() { keep_going.store(false); }

    /**
     * \brief Sets a menu item to the specified value
     * \param item The menu item to change
     * \param new_val The new value of the item
     * 
     * The menu item must be settable (i.e., not a submenu)
    */
    void setMenuState(std::shared_ptr<MenuItem> item, unsigned int new_val) override;

    /**
     * \brief Restores a menu to its default state
     * \param item The menu item to restore
     *
     * The menu item must be settable (i.e., not a submenu)
     */
    void restoreMenuState(std::shared_ptr<MenuItem> item) override;

    /**
     * \brief Lookup a controller input definition by name.
     */
    std::shared_ptr<ControllerInput> getInput(const std::string& name) {
      return game.getSignalTable().getInput(name);
    }

    /**
     * \brief Resolve a raw event to its controller input definition.
     */
    std::shared_ptr<ControllerInput> getInput(const DeviceEvent& event) {
      return game.getSignalTable().getInput(event);
    }

    /**
     * \brief Resolve controller-input references from config into a vector.
     */
    void addControllerInputs(const toml::table& config, const std::string& key,
                                 std::vector<std::shared_ptr<ControllerInput>>& vec) {
      game.getSignalTable().addToVector(config, key, vec);
    }

    /**
     * \brief Resolve game-command references from config into command objects.
     */
    void addGameCommands(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<GameCommand>>& vec) {
      game.addGameCommands(config, key, vec);
    }

    /**
     * \brief Resolve game-command references from config into controller inputs.
     */
    void addGameCommands(const toml::table& config, const std::string& key,
                         std::vector<std::shared_ptr<ControllerInput>>& vec) {
      game.addGameCommands(config, key, vec);
    }

    /**
     * \brief Resolve game-condition references from config into condition objects.
     */
    void addGameConditions(const toml::table& config, const std::string& key,
                           std::vector<std::shared_ptr<GameCondition>>& vec) {
      game.addGameConditions(config, key, vec);
    }

    /**
     * \brief Build a sequence from configuration.
     */
    std::shared_ptr<Sequence> createSequence(toml::table& config, const std::string& key,
                                             bool required) {
      return game.makeSequence(config, key, required);
    }

    /**
     * \brief Get canonical event name for a raw controller event.
     */
    std::string getEventName(const DeviceEvent& event) {
      return game.getEventName(event);
    }

    /**
     * \brief Observer function for input from the interface
     * 
     * \param command A string containing the Json object received from the interface
     * 
     * This routine processes all incoming commands from the interface.
     */
    void newCommand(const std::string& command);

    /**
     * \brief Is the engine paused
     * 
     * \return true if the engine is paused, false if it is running
     * 
     * While paused, all signals are passed through unaltered and modifiers' timers do not tick down.
     */
    bool isPaused() { return pause.load(); }

    /**
     * \brief Test helper to override interface health state.
     */
    void setInterfaceHealthForTest(bool healthy) { chaosInterface.setTalkerHealthyForTest(healthy); }

    /**
     * \brief Should the processing loop keep going
     * 
     * \return true as long as the engine should run
     *
     * When this returns false, the engine will shut down and exit.
     */
    bool keepGoing() { return keep_going.load(); }
  };

};
