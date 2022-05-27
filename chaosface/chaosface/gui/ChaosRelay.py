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
from flexx import flx
from config import model

# This Class implements a Model-View bridge

class ChaosRelay(flx.Component):

  voteTime = flx.FloatProp(0.5, settable=True)
  modTimes = flx.ListProp([0.0,0.0,0.0], settable=True)
  votes = flx.ListProp([0,0,0], settable=True)
  mods = flx.ListProp(["","",""], settable=True)
  activeMods = flx.ListProp(["","",""], settable=True)
  paused = flx.BoolProp(True, settable=True)
  pausedBrightBackground = flx.BoolProp(True, settable=True)
  connected = flx.BoolProp(True, settable=True)
  connectedBrightBackground = flx.BoolProp(True, settable=True)
  resetSoftmax = flx.BoolProp(False, settable=True)

  tmiResponse = flx.StringProp("", settable=True)
  
  # Settings:
  timePerModifier = flx.FloatProp(model.get_value("modifier_time"), settable=True)
  softmaxFactor = flx.IntProp(model.get_value("softmax_factor"), settable=True)
  
  bot_name = flx.StringProp(model.get_bot_value("nick"), settable=True)
  bot_oauth = flx.StringProp(model.get_bot_value("oauth"), settable=True)
  channel_name = flx.StringProp(model.get_bot_value("channels"), settable=True)

  announce_mods = flx.BoolProp(model.get_value("announce_mods"), settable=True)
        
  ui_rate = flx.FloatProp(model.get_value("ui_rate"), settable=True)
  uiPort = flx.IntProp(model.get_value("ui_port"), settable=True)
  
  shouldSave = flx.BoolProp(False, settable=True)
  
  def init(self):
    super().init()

    self.set_timePerModifier(model.get_value("modifier_time"))
    self.set_softmaxFactor(model.get_value("softmax_factor"))
    
    self.set_bot_name(model.get_bot_value("nick"))
    self.set_bot_oauth(model.get_bot_value("oauth"))
    self.set_channel_name(model.get_value("channels"))

    self.set_ui_rate(model.get_value("ui_rate"))
    self.set_uiPort(model.get_value("ui_port"))

  
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
      
#  @flx.reaction('allMods')
#  def on_allMods(self, *events):
#    for ev in events:
#      chaosConfig["allMods"] = ev.new_value
##      logging.info("Relay set allMods")
      
  @flx.reaction('timePerModifier')
  def on_timePerModifier(self, *events):
    for ev in events:
      self.chaosConfig["modifier_time"]  = ev.new_value
#      self.updateTimePerModifier(ev.new_value)
      
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

  @flx.reaction('announce_mods')
  def on_announce_mods(self, *events):
    for ev in events:
      self.chaosConfig["announce_mods"] = ev.new_value
                        
  @flx.reaction('ui_rate')
  def on_ui_rate(self, *events):
    for ev in events:
#      print("new ui_Rate: " + str(ev.new_value))
      self.chaosConfig["ui_rate"]  = ev.new_value
      
  @flx.reaction('uiPort')
  def on_uiPort(self, *events):
    for ev in events:
#      print("new ui_Rate: " + str(ev.new_value))
      self.chaosConfig["uiPort"]  = ev.new_value
      
  @flx.reaction('shouldSave')
  def on_shouldSave(self, *events):
    for ev in events:
      if ev.new_value:
        print("Saving config to: " + self.chaosConfigFile)
        self.saveConfig()
        self.set_shouldSave(False)
      
  """ Global object to relay paint events to all participants.
  """
    
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
