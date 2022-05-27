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
import json
from typing import List
from os.path import normpath
from ModelSubject import ModelSubject
from ModelObserver import ModelObserver
from config import CHAOS_CONFIG_FILE, CHATBOT_CONFIG_FILE
from config.defaults import chaosDefaults

class ChaosModel(ModelSubject):
  _observers: List[ModelObserver] = []

  def __init__(self):

    self.keepGoing = True
    self.botConnected = False
    self.engineConnected = False
    self.paused = True
    self.reset_softmax = False
    self.game_name = 'NONE'
    self.game_config_errors = 0
    self.modifierData = {}
    self.activeMods = [""] * self.get_value('active_modifiers')
    self.activeModTimes = [0.0] * self.get_value('active_modifiers')

    self.chaosConfigFile = normpath(CHAOS_CONFIG_FILE)
    logging.info("Initializing ChaosModel with file " + self.chaosConfigFile)
    self.chaosConfig = self.openConfig(self.chaosConfigFile)
    self.botConfig = self.openConfig(CHATBOT_CONFIG_FILE)

    # The chatbot generates its own configuration file automatically. We hook into it here

  def attach(self, observer: ModelObserver) -> None:
    self._observers.append(observer)

  def detach(self, observer: ModelObserver) -> None:
    self._observers.remove(observer)
    
  #Trigger an update in each subscriber.
  def notify(self, message) -> None:
    for observer in self._observers:
      observer.modelUpdate(message)

  def openConfig(self, configFile):
    try:
      with open(configFile) as json_data_file:
        chaos_dict = json.load(json_data_file)
    except Exception:
      logging.error("Error opening " + configFile)
      chaos_dict = {}
    return chaos_dict

  def saveConfig(self) -> None:
    with open(self.chaosConfigFile, 'w') as outfile:
      json.dump(self.chaosConfig, outfile)


  def get_value(self, attribute: str):
    if not self.chaosConfig.get(attribute):    
      self.chaosConfig[attribute] = chaosDefaults[attribute]
    return self.chaosConfig[attribute]

  # No default for the bot values -- should there be?
  def get_bot_value(self, attribute: str):
    return self.botConfig[attribute]

  # Set value directly without notifying observers
  def _set_value(self, attribute: str, new_val) -> None:
    self.chaosConfig[attribute] = new_val

  # Set value and notify observers
  def set_value(self, attribute: str, new_val) -> None:
    if self.chaosConfig[attribute] != new_val:
      self.chaosConfig[attribute] = new_val
      self.notify(attribute)

  def setActiveModifiers(self, value: int) -> None:
    if value < 1:
      logging.error("There must be at least one active modifier at a time")
      value = 1
    if value != self.chaosConfig['active_modifiers']:
      self.set_value('active_modifiers', value)
      self.activeMods = [""] * value
      self.activeModTimes = [0.0] * value

  def newModifier(self, name: str) -> None:
    finishedModIndex = self.activeModTimes.index(min(self.activeModTimes))
    self.activeMods[finishedModIndex] = name
    self.activeModTimes[finishedModIndex] = 1.0

  def setModifierTime(self, value: float) -> None:
    if value < 30.0:
      logging.error("Modifiers must be applied for at least 30 seconds each")
      value = 30.0
    self.set_value('modifier_time', value)
  
  def initializeGameData(self, gamedata):
    self.game_name = gamedata["game"]
    if "errors" in gamedata:
      self.game_config_errors = int(gamedata["errors"])
    else:
      logging.error("Missing error count in gamedata message")
      self.game_config_errors = -1
    if "nmods" in gamedata:
      self.set_value('active_modifiers', int(gamedata["nmods"]))
    else:
      logging.error("Missing modifier count in gamedata message")
    if "modtime" in gamedata:
      self.set_value('modifier_time', float(gamedata["modtime"]))
    else:
      logging.error("Missing modifier time in gamedata message")
    logging.debug("Initializing modifier data")
    self.modifierData = {}
    for mod in gamedata["mods"]:
      modName = mod["name"]
      self.modifierData[modName] = {}
      self.modifierData[modName]["desc"] = mod["desc"]
      self.modifierData[modName]["groups"] = mod["groups"]
    self.notify('load_game_file')

  def getModNames(self):
    return self.modifierData.keys()  

  def getModInfo(self, modname):
    return self.modifierData[modname]

  def getModDescription(self, *args):
    # Regularize spaces (the engine does the same)
    argument = (' '.join(c for c in args if c.isalnum()))
    message = str(self.get_value('mod_not_found')).format(modname=argument)
    argument = argument.lower()

    for key in self.modifierData.keys():
      if key.lower() == argument:
        mod = self.modifierData[key]
        if mod["desc"] == "":
          message = "mod " + key + ": No Description :("
        else:
          message = "!mod " + key + ": " + mod["desc"]
        break
    return message

  def getActiveMods(self) -> str:
    message = self.get_value('active_mods_msg')
    return message + ", ".join(filter(None, self.activeMods))

  def getVotableMods(self) -> str:
    message = self.get_value('voting_mods_msg')
    for num, mod in enumerate(self.currentMods, start=1):
      message += " {}: {}.".format(num, mod)
    return message

  def explainVoting(self) -> str:
    if self.get_value('voting_enabled'):
      msg = self.get_value('how_to_vote_msg').format(options=self.chaosConfig['vote_options'])
      if (self.get_value('proportional_voting')):
        return msg + self.chaosConfig['proportional_voting_msg']
      else:
        return msg + self.chaosConfig['top_voting_msg']
    else:
      return self.chaosConfig['no_voting_msg']

  def explainRedemption(self) -> str:
    return self.get_value('how_to_redeem').format(cooldown=self.get_value('apply_mod_cooldown'))
    
  def explainCredits(self) -> str:
    points_msg = ""
    if (self.get_value('points_redemption')):
      points_msg = self.get_value('points_msg')

    bits_msg = ""      
    if (self.get_value('bits_redemption')):
      if self.get_value('multiple_credits'):
        restrict = self.get_value('multiple_credits_msg').format(amount=self.get_value('bits_per_credit'))
      else:
        restrict = self.get_value('no_multiple_credits_msg').format(amount=self.get_value('bits_per_credit'))
      bits_msg = self.get_value('bits_msg').format(restriction=restrict)

    raffle_msg = ""
    if (self.get_value('raffle_enabled')):
      raffle_msg = self.get_value('raffle_msg')
    methods = ', '.join(filter(None, [points_msg, bits_msg, raffle_msg]))
    if not methods:
      return self.get_value('no_redemption')
    msg = self.get_value('how_to_earn').format(activemethods=methods)
      
    
#  @flx.reaction('voteTime')
#  def on_voteTime(self, *events):
#    for ev in events:
#      self.updateVoteTime(ev.new_value)
      
#  @flx.emitter
#  def updateVoteTime(self, value):
#    return dict(value=value)
