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
import math
import copy
import numpy as np
from flexx import flx

#from ..config.ChaosData import ChaosData
#from ..config.DataObserver import DataObserver
#model = ChaosData("chaosRelay.json")

# We use these values when we can't read a value from the json file, typically because it hasn't
# been created yet.
chaosDefaults = {
  'active_modifiers': 3,
  'modifier_time': 180.0,
  'softmax_factor': 33,
  'vote_options': 3,
  'voting_type': 'Proportional',
  'irc_host': "irc.twitch.tv",
  'irc_port': 6667,
  'bot_name': "your_bot",
  'bot_oauth': "oauth:",
  'channel_name': "your_channel",
  'chat_rate': 0.67,
  'pi_host': "192.168.1.232",
  'listen_port': 5556,
  'talk_port': 5555,
  'announce_mods': False,
  'ui_rate': 20.0,
  'ui_port': 80,
  'announce_candidates': False,
  'announce_winner': False,
  'bits_redemptions': False,
  'bits_per_credit': 100,
  'points_redemptions': False,
  'points_per_credit': 20000,
  'raffles': False,
  'redemption_cooldown': 600,
}

# Some property subclasses to keep numerical input within sane limits
class BoundedIntProp(flx.IntProp):

  def __init__(self, minval, maxval, *args, **kwargs):
    super().__init__(*args, **kwargs)
    self.min_val = minval
    self.max_val = maxval

  def _validate(self, value, name, data):
    if value < self.min_val:
      value = self.min_val
    elif value > self.max_val:
      value = self.max_val
    return value

class BoundedFloatProp(flx.FloatProp):

  def __init__(self, minval, maxval, *args, **kwargs):
    super().__init__(*args, **kwargs)
    self.min_val = minval
    self.max_val = maxval

  def _validate(self, value, name, data):
    if value < self.min_val:
      value = self.min_val
    elif value > self.max_val:
      value = self.max_val
    return value


class ChaosRelay(flx.Component):
  keepGoing = True
  modifierData = {"1":{"desc":"","groups":""}, "2":{"desc":"","groups":""}, "3":{"desc":"","groups":""},
                  "4":{"desc":"","groups":""}, "5":{"desc":"","groups":""}, "6":{"desc":"","groups":""}}

  # Model-View bridge:
  gameName = flx.StringProp(settable=True)
  gameErrors = flx.IntProp(settable=True)

  voteTime = flx.FloatProp(settable=True)
  modTimes = flx.ListProp(settable=True)
  votes = flx.ListProp(settable=True)
  candidateMods = flx.ListProp(settable=True)
  activeMods = flx.ListProp(settable=True)
  allMods = flx.ListProp(settable=True)
  paused = flx.BoolProp(settable=True)
  pausedBrightBackground = flx.BoolProp(settable=True)
  connected = flx.BoolProp(settable=True)
  connectedBrightBackground = flx.BoolProp(settable=True)
  resetSoftmax = flx.BoolProp(settable=True)
  tmiResponse = flx.StringProp(settable=True)
  
  # Chaos Settings:
  numActiveMods = flx.IntProp(settable=True)
  timePerModifier = BoundedFloatProp(minval=0, maxval=3600.0, settable=True)
  softmaxFactor = flx.IntProp(settable=True)
  voteOptions = BoundedIntProp(minval=2, maxval=10, settable=True)
  votingType = flx.EnumProp(['Proportional', 'Majority', 'DISABLE'], 'Proportional', settable=True)
  bitsRedemption = flx.BoolProp(settable=True)
  bitsPerCredit = flx.IntProp(settable=True)
  pointsRedemption = flx.BoolProp(settable=True)
  pointsPerCredit = flx.IntProp(settable=True)
  raffles = flx.BoolProp(settable=True)
  redemptionCooldown = flx.IntProp(settable=True)

  # Chat Bot Configuration  
  ircHost = flx.StringProp(settable=True)
  ircPort = flx.IntProp(settable=True)  
  bot_name = flx.StringProp(settable=True)
  bot_oauth = flx.StringProp(settable=True)
  channel_name = flx.StringProp(settable=True)
  chat_rate = flx.FloatProp(settable=True)

  # Engine Settings
  piHost = flx.StringProp(settable=True)
  listenPort = BoundedIntProp(minval=1, maxval=65535, settable=True)
  talkPort = BoundedIntProp(minval=1, maxval=65535, settable=True)

  # User Interface Settings      
  announceCandidates = flx.BoolProp(settable=True)
  announceWinner = flx.BoolProp(settable=True)
  uiRate = BoundedFloatProp(minval=0.05, maxval=60.0, settable=True)
  uiPort = BoundedIntProp(minval=1, maxval=65535, settable=True)
  
  shouldSave = flx.BoolProp(settable=True)

  def init(self, file_name):
    self.chaosConfig = {}
    self.openConfig(file_name)
    self.resetAll()

  def get_attribute(self, key):
    if key in self.chaosConfig:
      return self.chaosConfig[key]
    # Nothing found, return default setting if there is one
    if key in chaosDefaults:
      return chaosDefaults[key]
    logging.error(f"No default for '{key}'")
    return None

  def openConfig(self, configFile):
    try:
      self.chaosConfigFile = configFile
      print("Initializing from " + self.chaosConfigFile)
      with open(self.chaosConfigFile) as json_data_file:
        ChaosRelay.chaosConfig = json.load(json_data_file)
    except Exception as e:
      print("Error opening " + self.chaosConfigFile)
      ChaosRelay.chaosConfig = {}

  @flx.action
  def resetAll(self):
    self.set_numActiveMods(self.get_attribute('active_modifiers'))
    self.set_timePerModifier(self.get_attribute('modifier_time'))
    self.set_voteOptions(self.get_attribute('vote_options'))
    self.set_votingType(self.get_attribute('voting_type'))
    self.set_softmaxFactor(self.get_attribute('softmax_factor'))
    self.set_ircHost(self.get_attribute('irc_host'))
    self.set_ircPort(self.get_attribute('irc_port'))
    self.set_bot_name(self.get_attribute('bot_name'))
    self.set_bot_oauth(self.get_attribute('bot_oauth'))
    self.set_channel_name(self.get_attribute('channel_name'))
    self.set_chat_rate(self.get_attribute('chat_rate'))
    self.set_piHost(self.get_attribute('pi_host'))
    self.set_listenPort(self.get_attribute('listen_port'))
    self.set_talkPort(self.get_attribute('talk_port'))
    self.set_announceCandidates(self.get_attribute('announce_candidates'))
    self.set_announceWinner(self.get_attribute('announce_winner'))
    self.set_bitsRedemption(self.get_attribute('bits_redemptions'))
    self.set_bitsPerCredit(self.get_attribute('bits_per_credit'))
    self.set_pointsRedemption(self.get_attribute('points_redemptions'))
    self.set_pointsPerCredit(self.get_attribute('points_per_credit'))
    self.set_raffles(self.get_attribute('raffles'))
    self.set_redemptionCooldown(self.get_attribute('redemption_cooldown'))
    self.set_uiRate(self.get_attribute('ui_rate'))
    self.set_uiPort(self.get_attribute('ui_port'))

    self.set_gameName("NONE")
    self.set_gameErrors(0)
    self.set_voteTime(0.5)
    self.set_paused(True)
    self.set_pausedBrightBackground(True)
    self.set_connected(False)
    self.set_connectedBrightBackground(True)
    self.set_resetSoftmax(False)
    self.set_tmiResponse("")
    self.set_modTimes([0.0]*self.get_attribute('active_modifiers'))
    self.set_activeMods([""]*self.get_attribute('active_modifiers'))
    self.set_votes([0]*self.get_attribute('vote_options'))
    self.set_candidateMods([""]*self.get_attribute('vote_options'))
    self.set_allMods([])
    
    self.votedUsers = []

  def timePerVote(self) -> float:
    return self.get_attribute('modifier_time')/float(self.get_attribute('active_modifiers'))

  @flx.action
  def setTimePerVote(self, time):
    self.set_voteTime(time/self.timePerVote())

  @flx.action
  def initializeGame(self, gamedata: dict):
    logging.debug("Initializing game data")
    game_name = gamedata["game"]
    logging.debug(f"Setting game name to '{game_name}'")
    self.set_gameName(gamedata["game"])

    errors = int(gamedata["errors"])
    logging.debug(f"Setting game errors to '{errors}'")
    self.set_gameErrors(errors)

    nmods = int(gamedata["nmods"])
    logging.debug(f"Setting total active mods to '{nmods}'")
    self.set_numActiveMods(nmods)
    
    modtime = float(gamedata["modtime"])
    logging.debug(f"Setting modifier time to {modtime}")
    self.set_timePerModifier(modtime)

    logging.debug(f"Initializing modifier data:")
    self.modifierData = {}
    for mod in gamedata["mods"][0]:
      print(mod)
      modName = mod["name"]
      self.modifierData[modName] = {}
      self.modifierData[modName]["desc"] = mod["desc"]
      self.modifierData[modName]["groups"] = mod["groups"]
    self.allMods = self.modifierData.keys()
    self.resetSoftMax()

  def sleepTime(self):
    return 1.0 / self.get_attribute('ui_rate')

  @flx.reaction('numActiveMods', 'timePerModifier')
  def resetCurrentMods(self, *events):
    self.set_modTimes([0.0]*self.numActiveMods)
    self.set_activeMods([""]*self.numActiveMods)

  @flx.reaction('voteOptions')
  def resetVoteOptions(self, *events):
    self.set_votes([0]*self.voteOptions)
    self.set_candidateMods([""]*self.voteOptions)
    self.votedUsers = []


  @flx.action
  def tallyVote(self, index: int, user: str):
    if index >= 0 and index < self.get_attribute('vote_options') and not user in self.votedUsers:
      self.votedUsers.append(user)
      self._mutate('votes', self.votes[index]+1, 'set', index)

  @flx.action
  def resetVotes(self):
    self.votedUsers = []

  @flx.action
  def decrementVoteTimes(self, dTime):
    delta = dTime/self.timePerModifier
    for i in range(self.numActiveMods):
      self._mutate('modTimes', self.modTimes[i] - delta, i)
      

  def resetSoftMax(self):
    logging.info("Resetting SoftMax!")
    for mod in self.modifierData:
      self.modifierData[mod]["count"] = 0

  def getSoftmaxDivisor(self, data):
    # determine the sum for the softmax divisor:
    softMaxDivisor = 0
    for key in data:
      softMaxDivisor += data[key]["contribution"]
    return softMaxDivisor

  def updateSoftmaxProbabilities(self, data):
    for mod in data:
      data[mod]["contribution"] = math.exp(self.modifierData[mod]["count"] *
          math.log(float(self.softmaxFactor)/100.0))
    softMaxDivisor = self.getSoftmaxDivisor(data)
    for mod in data:
      data[mod]["p"] = data[mod]["contribution"]/softMaxDivisor
      
  def updateSoftMax(self, newMod):
    if newMod in self.modifierData:
      self.modifierData[newMod]["count"] += 1        
      # update all probabilities:
      self.updateSoftmaxProbabilities(self.modifierData)
    else:
      logging.error(f"Updating SoftMax: modifier '{newMod}' not in mod list")


  def getNewVotingPool(self):
    # Ignore currently active mods:
    inactiveMods = set(np.setdiff1d(self.allMods, list(self.activeMods)))
        
    candidates = []
    for k in range(self.voteOptions):
      # build a list of contributor for this selection:
      votableTracker = {}
      for mod in inactiveMods:
        votableTracker[mod] = copy.deepcopy(self.modifierData[mod])
              
      # Calculate the softmax probablities (must be done each time):
      self.updateSoftmaxProbabilities(votableTracker)
      # make a decision:
      theChoice = np.random.uniform(0,1)
      selectionTracker = 0
      for mod in votableTracker:
        selectionTracker += votableTracker[mod]["p"]
        if selectionTracker > theChoice:
          candidates.append(mod)
          inactiveMods.remove(mod)  #remove this to prevent a repeat
          break
    
    if logging.getLogger().isEnabledFor(logging.DEBUG):
      logging.debug("New Voting Round:")
      for mod in candidates:
        if "p" in self.modifierData[mod]:
          logging.debug(f" - {self.modifierData[mod]['p']*100.0:0.2f}% {mod}")
        else:
          logging.debug(f" - 0.00% {mod}")

    self.set_candidateMods(candidates)
    # Reset votes since there is a new voting pool
    self.set_votes([0]*self.get_attribute('vote_options'))

  @flx.action
  def replaceMod(self, newMod):
    mods = list(self.activeMods)
    # Find the oldest mod
    times = list(self.modTimes)
    finishedModIndex = times.index(min(times))
    mods[finishedModIndex] = newMod
    times[finishedModIndex] = 1.0
    self._mutate_modTimes(times)
    self._mutate_activeMods(mods)

  @flx.reaction('gameName')
  def on_gameName(self, *events):
    for ev in events:
      self.updateGameName(ev.new_value)

  @flx.reaction('gameErrors')
  def on_gameErrors(self, *events):
    for ev in events:
      self.updateGameErrors(ev.new_value)

  # TODO: If the number of active mods changes, we should reset the list of active and candidate mods
  @flx.reaction('numActiveMods')
  def on_numActiveMods(self, *events):
    for ev in events:
      self.updateNumActiveMods(ev.new_value)

  @flx.reaction('voteTime')
  def on_voteTime(self, *events):
    for ev in events:
      self.updateVoteTime(ev.new_value)
      
  @flx.reaction('modTimes')
  def on_modTimes(self, *events):
    for ev in events:
      self.updateModTimes(ev.new_value)
      
  @flx.reaction('votes')
  def on_votes(self, *events):
    for ev in events:
      self.updateVotes(ev.new_value)
      
  @flx.reaction('mods')
  def on_mods(self, *events):
    for ev in events:
      self.updateMods(ev.new_value)
      
  @flx.reaction('activeMods')
  def on_activeMods(self, *events):
    for ev in events:
      self.updateActiveMods(ev.new_value)
      
  @flx.reaction('timePerModifier')
  def on_timePerModifier(self, *events):
    for ev in events:
      self.chaosConfig["modifier_time"]  = ev.new_value
      
  @flx.reaction('softmaxFactor')
  def on_softmaxFactor(self, *events):
    for ev in events:
      self.chaosConfig["softmax_factor"]  = ev.new_value
      
  @flx.reaction('paused')
  def on_paused(self, *events):
    for ev in events:
      self.updatePaused(ev.new_value)
      
  @flx.reaction('pausedBrightBackground')
  def on_pausedBrightBackground(self, *events):
    for ev in events:
      self.updatePausedBrightBackground(ev.new_value)
      
  @flx.reaction('connected')
  def on_connected(self, *events):
    for ev in events:
      self.updateConnected(ev.new_value)
      
  @flx.reaction('connectedBrightBackground')
  def on_connectedBrightBackground(self, *events):
    for ev in events:
      self.updateConnectedBrightBackground(ev.new_value)
      
  @flx.reaction('tmiResponse')
  def on_tmiResponse(self, *events):
    for ev in events:
      self.updateTmiResponse(ev.new_value)
      
  @flx.reaction('ircHost')
  def on_ircHost(self, *events):
    for ev in events:
      self.chaosConfig["host"] = ev.new_value
      
  @flx.reaction('ircPort')
  def on_ircPort(self, *events):
    for ev in events:
      self.chaosConfig["port"] = ev.new_value
      
  @flx.reaction('bot_name')
  def on_bot_name(self, *events):
    for ev in events:
      self.chaosConfig["bot_name"] = ev.new_value
      
  @flx.reaction('bot_oauth')
  def on_bot_oauth(self, *events):
    for ev in events:
      self.chaosConfig["bot_oauth"] = ev.new_value
      
  @flx.reaction('channel_name')
  def on_channel_name(self, *events):
    for ev in events:
      self.chaosConfig["channel_name"] = ev.new_value

  @flx.reaction('announceWinner')
  def on_announceWinner(self, *events):
    for ev in events:
      self.chaosConfig["announce_winner"] = ev.new_value
                        
  @flx.reaction('uiRate')
  def on_uiRate(self, *events):
    for ev in events:
      self.chaosConfig["ui_rate"]  = ev.new_value
      
  @flx.reaction('uiPort')
  def on_uiPort(self, *events):
    for ev in events:
      self.chaosConfig["uiPort"]  = ev.new_value

  @flx.reaction('piHost')
  def on_piHost(self, *events):
    for ev in events:
      self.chaosConfig["pi_host"]  = ev.new_value

  @flx.reaction('listenPort')
  def on_listenPort(self, *events):
    for ev in events:
      self.chaosConfig["listen_port"]  = ev.new_value

  @flx.reaction('talkPort')
  def on_talkPort(self, *events):
    for ev in events:
      self.chaosConfig["talkPort"]  = ev.new_value

  @flx.reaction('shouldSave')
  def on_shouldSave(self, *events):
    for ev in events:
      if ev.new_value:
        logging.info("Saving config to: " + self.chaosConfigFile)
        self.saveConfig()
        self.set_shouldSave(False)
      
  # Emitters to relay events to all participants.
    
  @flx.emitter
  def updateGameName(self, value):
    return dict(value=value)

  @flx.emitter
  def updateGameErrors(self, value):
    return dict(value=value)

  @flx.emitter
  def updateNumActiveMods(self, value):
    return dict(value=value)

  @flx.emitter
  def updateVoteTime(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateModTimes(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateVotes(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateMods(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateActiveMods(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updatePaused(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updatePausedBrightBackground(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateConnected(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateConnectedBrightBackground(self, value):
    return dict(value=value)
    
  @flx.emitter
  def updateTmiResponse(self, value):
    return dict(value=value)
        
  @flx.emitter
  def updatePiHost(self, value):
    return dict(value=value)

  @flx.emitter
  def updateListenPort(self, value):
    return dict(value=value)

  @flx.emitter
  def updateTalkPort(self, value):
    return dict(value=value)

