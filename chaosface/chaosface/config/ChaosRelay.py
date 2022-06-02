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

from flexx import flx
import json

from .defaults import chaosDefaults

class ChaosRelay(flx.Component):

  def init(self, file_name):
    super().init()
    self.chaosConfig = {}
    self.openConfig(file_name)

    # Model-View bridge:
    self.gameName = flx.StringProp("NONE", settable=True)
    self.gameErrors = flx.IntProp(0, settable=True)

    self.voteTime = flx.FloatProp(0.5, settable=True)
    self.modTimes = flx.ListProp([0.0,0.0,0.0], settable=True)
    self.votes = flx.ListProp([0,0,0], settable=True)
    self.mods = flx.ListProp(["","",""], settable=True)
    self.activeMods = flx.ListProp(["","",""], settable=True)
    self.allMods = flx.ListProp([], settable=True)
    self.paused = flx.BoolProp(True, settable=True)
    self.pausedBrightBackground = flx.BoolProp(True, settable=True)
    self.connected = flx.BoolProp(True, settable=True)
    self.connectedBrightBackground = flx.BoolProp(True, settable=True)
    self.resetSoftmax = flx.BoolProp(False, settable=True)
    self.tmiResponse = flx.StringProp("", settable=True)
  
    # Chaos Settings:
    self.totalActiveMods = flx.IntProp(self.get_attribute("active_modifiers"), settable=True)
    self.timePerModifier = flx.FloatProp(self.get_attribute("modifier_time"), settable=True)
    self.softmaxFactor = flx.IntProp(self.get_attribute("softmax_factor"), settable=True)

    # Chat Bot Configuration  
    self.ircHost = flx.StringProp(self.get_attribute("host"), settable=True)
    self.ircPort = flx.IntProp(self.get_attribute("port"), settable=True)  
    self.bot_name = flx.StringProp(self.get_attribute("bot_name"), settable=True)
    self.bot_oauth = flx.StringProp(self.get_attribute("bot_oauth"), settable=True)
    self.channel_name = flx.StringProp(self.get_attribute("channel_name"), settable=True)
    self.chat_rate = flx.FloatProp(self.get_attribute("chat_rate"), settable=True)

    # Engine Settings
    self.piHost = flx.StringProp(self.get_attribute("pi_host"), settable=True)
    self.listenPort = flx.IntProp(self.get_attribute("listen_port"), settable=True)
    self.talkPort = flx.IntProp(self.get_attribute("talk_port"), settable=True)
    # User Interface Settings      
    self.announceMods = flx.BoolProp(self.get_attribute("announce_mods"), settable=True)
    self.uiRate = flx.FloatProp(self.get_attribute("ui_rate"), settable=True)
    self.uiPort = flx.IntProp(self.get_attribute("uiPort"), settable=True)
  
    self.shouldSave = flx.BoolProp(False, settable=True)

  def get_attribute(self, attribute):
    if attribute in self.chaosConfig:
      return self.chaosConfig.get(attribute)
    # Nothing found, return default setting
    if attribute in chaosDefaults:
      return chaosDefaults[attribute]
    log.error(f"No default for attribute '{attribute}'")
    return None


  def openConfig(self, configFile):
    try:
      self.chaosConfigFile = configFile
      log.info("ChaosRelay initializing with file: " + self.chaosConfigFile)
      with open(self.chaosConfigFile) as json_data_file:
        ChaosRelay.chaosConfig = json.load(json_data_file)
    except Exception as e:
      log.error("openConfig(): Error in opening file: " + self.chaosConfigFile)

  def saveConfig(self):
    with open(self.chaosConfigFile, 'w') as outfile:
      json.dump(self.chaosConfig, outfile)

  @flx.reaction('!gameName')
  def on_gameName(self, *events):
    for ev in events:
      self.updateGameName(ev.new_value)

  @flx.reaction('!gameErrors')
  def on_gameErrors(self, *events):
    for ev in events:
      self.updateGameErrors(ev.new_value)

  @flx.reaction('!totalActiveMods')
  def on_totalActiveMods(self, *events):
    for ev in events:
      self.updateTotalActiveMods(ev.new_value)

  @flx.reaction('!voteTime')
  def on_voteTime(self, *events):
    for ev in events:
      self.updateVoteTime(ev.new_value)
      
  @flx.reaction('!modTimes')
  def on_modTimes(self, *events):
    for ev in events:
      self.updateModTimes(ev.new_value)
      
  @flx.reaction('!votes')
  def on_votes(self, *events):
    for ev in events:
      self.updateVotes(ev.new_value)
      
  @flx.reaction('!mods')
  def on_mods(self, *events):
    for ev in events:
      self.updateMods(ev.new_value)
      
  @flx.reaction('!activeMods')
  def on_activeMods(self, *events):
    for ev in events:
      self.updateActiveMods(ev.new_value)
      
  @flx.reaction('!timePerModifier')
  def on_timePerModifier(self, *events):
    for ev in events:
      self.chaosConfig["modifier_time"]  = ev.new_value
      
  @flx.reaction('!softmaxFactor')
  def on_softmaxFactor(self, *events):
    for ev in events:
      self.chaosConfig["softmax_factor"]  = ev.new_value
      
  @flx.reaction('!paused')
  def on_paused(self, *events):
    for ev in events:
      self.updatePaused(ev.new_value)
      
  @flx.reaction('!pausedBrightBackground')
  def on_pausedBrightBackground(self, *events):
    for ev in events:
      self.updatePausedBrightBackground(ev.new_value)
      
  @flx.reaction('!connected')
  def on_connected(self, *events):
    for ev in events:
      self.updateConnected(ev.new_value)
      
  @flx.reaction('!connectedBrightBackground')
  def on_connectedBrightBackground(self, *events):
    for ev in events:
      self.updateConnectedBrightBackground(ev.new_value)
      
  @flx.reaction('!tmiResponse')
  def on_tmiResponse(self, *events):
    for ev in events:
      self.updateTmiResponse(ev.new_value)
      
  @flx.reaction('!ircHost')
  def on_ircHost(self, *events):
    for ev in events:
      self.chaosConfig["host"] = ev.new_value
      
  @flx.reaction('!ircPort')
  def on_ircPort(self, *events):
    for ev in events:
      self.chaosConfig["port"] = ev.new_value
      
  @flx.reaction('!bot_name')
  def on_bot_name(self, *events):
    for ev in events:
      self.chaosConfig["bot_name"] = ev.new_value
      
  @flx.reaction('!bot_oauth')
  def on_bot_oauth(self, *events):
    for ev in events:
      self.chaosConfig["bot_oauth"] = ev.new_value
      
  @flx.reaction('!channel_name')
  def on_channel_name(self, *events):
    for ev in events:
      self.chaosConfig["channel_name"] = ev.new_value

  @flx.reaction('!announceMods')
  def on_announceMods(self, *events):
    for ev in events:
      self.chaosConfig["announce_mods"] = ev.new_value
                        
  @flx.reaction('!uiRate')
  def on_uiRate(self, *events):
    for ev in events:
      self.chaosConfig["ui_rate"]  = ev.new_value
      
  @flx.reaction('!uiPort')
  def on_uiPort(self, *events):
    for ev in events:
      self.chaosConfig["uiPort"]  = ev.new_value

  @flx.reaction('!piHost')
  def on_piHost(self, *events):
    for ev in events:
      self.chaosConfig["pi_host"]  = ev.new_value

  @flx.reaction('!listenPort')
  def on_listenPort(self, *events):
    for ev in events:
      self.chaosConfig["listen_port"]  = ev.new_value

  @flx.reaction('!talkPort')
  def on_talkPort(self, *events):
    for ev in events:
      self.chaosConfig["talkPort"]  = ev.new_value

  @flx.reaction('!shouldSave')
  def on_shouldSave(self, *events):
    for ev in events:
      if ev.new_value:
        log.info("Saving config to: " + self.chaosConfigFile)
        self.saveConfig()
        self.set_shouldSave(False)
      
  # Global object to relay paint events to all participants.
    
  @flx.emitter
  def updateGameName(self, value):
    return dict(value=value)

  @flx.emitter
  def updateGameErrors(self, value):
    return dict(value=value)

  @flx.emitter
  def updateTotalActiveMods(self, value):
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
