"""
  Twitch Controls Chaos (TCC)
  Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
  file at the top-level directory of this distribution for details of the
  contributers.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""
import logging
from typing import List
import json

from .DataObserver import DataObserver

# We use these values when we can't read a value from the json file, typically because it hasn't
# been created yet.
chaosDefaults = {
  "active_modifiers": 3,
  "modifier_time": 180.0,
  "softmax_factor": 33,
  "irc_host": "irc.twitch.tv",
  "irc_port": 6667,
  "bot_name": "your_bot",
  "bot_oauth": "oauth:",
  "channel_name": "your_channel",
  "chat_rate": 0.67,
  "pi_host": "localhost",
  "listen_port": 5555,
  "talk_port": 5556,
  "announce_mods": False,
  "ui_rate": 20.0,
  "ui_port": 80,
}

class ChaosData():
  _observers: List[DataObserver] = []

  def init(self, file_name):
    self.chaosConfig = {}
    self.openConfig(file_name)

    self.keepGoing = True
    self.game_name = "NONE"
    self.game_errors = 0
    self.vote_time = 0.5
    self.mod_times = [0.0] * self.get_attribute("active_modifiers")
    self.votes = [0] * self.get_attribute("active_modifiers")
    self.mods = [""] * self.get_attribute("active_modifiers")
    self.active_mods = [""] * self.get_attribute("active_modifiers")
    self.all_mods = []
    self.paused = True
    self.paused_bright_background = True
    self.connected = False
    self.connected_bright_background = True
    self.reset_softmax = False
    self.tmi_response = ""
    self.should_save = False
    self.reset_voting = False

  def notify(self, message) -> None:
    for observer in self._observers:
      observer.update(message)
  
  def attach(self, observer: DataObserver) -> None:
    self._observers.append(observer)
  
  def detach(self, observer: DataObserver) -> None:
    self._observers.remove(observer)

  def get_attribute(self, key):
    return self._get(self.chaosConfig, key, chaosDefaults)

  def set_game(self, gamedata: dict):
    logging.debug("Initializing game data")
    self.game_name = gamedata["game"]
    self.game_errors = gamedata["errors"]
    self.set_attribute('active_modifiers', gamedata["nmods"])
    self.set_attribute('modifier_time', gamedata["modtime"])
    self.modifierData = {}
    logging.debug(f"Initializing modifier data:")
    for mod in gamedata["mods"][0]:
      logging.debug(f'Raw mod info: {mod}')
      modName = mod["name"]
      self.modifierData[modName] = {}
      self.modifierData[modName]["desc"] = mod["desc"]
      self.modifierData[modName]["groups"] = mod["groups"]

    self.notify('game-data')

  def set_pausedBrightBackground(self, isBright):
    self.paused_bright_background = isBright
    self.notify('paused_bright_background')

  def set_connectedBrightBackground(self, isBright):
    self.connected_bright_background = isBright
    self.notify('connected_bright_background')

  def _get(self, data, key, defaults):
    if key in data:
      return data[key]
    # Nothing found, return default setting if there is one
    if defaults & key in defaults:
      return defaults[key]
    logging.error(f"No default for '{key}'")
    return None

  def set_attribute(self, key, new_value):
    if self._set(self.chaosConfig, key, new_value):
      self.saveConfig()

  def _set(self, data, key, new_value) -> bool:
    if key not in data:
      logging.error(f"Unknown data key '{key}'")
    if data[key] != new_value:
      data[key] = new_value
      self.notify(key)
      return True
    return False

  def openConfig(self, configFile):
    try:
      self.chaosConfigFile = configFile
      logging.info("ChaosRelay initializing from " + self.chaosConfigFile)
      with open(self.chaosConfigFile) as json_data_file:
        self.chaosConfig = json.load(json_data_file)
    except Exception as e:
      logging.error("Error opening file " + self.chaosConfigFile)
      self.chaosConfig = chaosDefaults
      self.saveConfig()

  def saveConfig(self):
    with open(self.chaosConfigFile, 'w') as outfile:
      json.dump(self.chaosConfig, outfile)

