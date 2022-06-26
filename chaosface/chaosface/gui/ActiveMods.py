# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Generates the source to show which modifiers are currently active and how much time remains for each.
"""

from flexx import flx
import chaosface.config.globals as config

class ActiveMods(flx.PyWidget):
  def init(self):
    super().init()
    
    self.label = []
    self.progress = []
    
    mod_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;"
    title_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;"
    progress_style = "background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; width:1050px;"
    
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.vote_label = flx.Label(flex=0,style=title_text_style, text="Active Mods" )
        flx.Label(flex=0,style=title_text_style, text=" ")
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(config.relay.num_active_mods):
            self.progress.append(flx.ProgressBar(flex=2, value=config.relay.mod_times[i], text='', style=progress_style))
        with flx.VFix(flex=1):
          for i in range(config.relay.num_active_mods):
            self.label.append(flx.Label(flex=1,style=mod_text_style, text=config.relay.active_mods[i]))

  @config.relay.reaction('update_mod_times')
  def _update_mod_times(self, *events):
    for ev in events:
      self.update_time(ev.value)
      
  @config.relay.reaction('update_active_mods')
  def _updateActiveMods(self, *events):
    for ev in events:
      self.update_mods(ev.value)

  @flx.action
  def update_mods(self, active_mods):
    for i in range(config.relay.num_active_mods):
      self.label[i].set_text(active_mods[i])
    
  @flx.action
  def update_time(self, mod_times):
    for i in range(config.relay.num_active_mods):
      self.progress[i].set_value(mod_times[i])
