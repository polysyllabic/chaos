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
log = logging.getLogger(__name__)
from typing import List
import json
from .DataObserver import DataObserver

from .defaults import chaosDefaults

class ChaosData():
  _observers: List[DataObserver] = []

  def init(self, file_name):
    self.chaosConfig = {}
    self.openConfig(file_name)

    # This dictionary holds the volatile data that we don't save
    self.chaosValues = {
      "game_name": "NONE",
      "game_errors": 0,
      "vote_time": 0.5,
      "mod_times": [0.0] * self.get_attribute("active_modifiers"),
      "votes": [0] * self.get_attribute("active_modifiers"),
      "mods": [""] * self.get_attribute("active_modifiers"),
      "active_mods": [""] * self.get_attribute("active_modifiers"),
      "all_mods": [],
      "paused": True,
      "paused_bright_background": True,
      "connected": False,
      "connected_bright_background": True,
      "reset_softmax": False,
      "tmi_response": "", 
      "should_save": False,
    }

  def notify(self, message) -> None:
    for observer in self._observers:
      observer.update(message)
  
  def get_attribute(self, key):
    return self._get(self.chaosConfig, key, chaosDefaults)

  def get_value(self, key):
    return self._get(self.chaosValues, key, None)

  def _get(self, data, key, defaults):
    if key in data:
      return data[key]
    # Nothing found, return default setting if there is one
    if defaults & key in defaults:
      return defaults[key]
    log.error(f"No default for '{key}'")
    return None

  def set_attribute(self, key, new_value):
    if self._set(self.chaosConfig, key, new_value):
      self.chaosConfig()

  def set_value(self, key, new_value):
    self._set(self.chaosValues, key, new_value)

  def _set(self, data, key, new_value) -> bool:
    if key not in data:
      log.error(f"Unknown data key '{key}'")
    if data[key] != new_value:
      data[key] = new_value
      self.notify(key)
      return True
    return False

  def openConfig(self, configFile):
    try:
      self.chaosConfigFile = configFile
      log.info("ChaosRelay initializing with file: " + self.chaosConfigFile)
      with open(self.chaosConfigFile) as json_data_file:
        self.chaosConfig = json.load(json_data_file)
    except Exception as e:
      log.error("openConfig(): Error in opening file: " + self.chaosConfigFile)
      self.chaosConfig = chaosDefaults
      self.saveConfig()

  def saveConfig(self):
    with open(self.chaosConfigFile, 'w') as outfile:
      json.dump(self.chaosConfig, outfile)

