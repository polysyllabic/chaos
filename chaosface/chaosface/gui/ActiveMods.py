
from flexx import flx
import chaosface.config.globals as config

class ActiveMods(flx.PyWidget):
  def init(self):
    self.chaosActiveView = ChaosActiveView()
    
  @config.relay.reaction('updateModTimes')
  def _update_mod_times(self, *events):
    for ev in events:
      self.chaosActiveView.update_time(ev.value)
      
  @config.relay.reaction('updateActiveMods')
  def _updateActiveMods(self, *events):
    for ev in events:
      self.chaosActiveView.update_mods(ev.value)

class ChaosActiveView(flx.PyWidget):
  def init(self):
    super().init()
    
    self.label = []
    self.progress = []
    
    mod_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;"
    title_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;"
    progress_style = " background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; width:1050px;"
    
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.voteLabel = flx.Label(flex=0,style=title_text_style, text="Active Mods" )
        self.blankLabel = flx.Label(flex=0,style=title_text_style, text=" ")
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(config.relay.num_active_mods):
            self.progress.append(flx.ProgressBar(flex=2, value=config.relay.mod_times[i], text='', style=progress_style))
        with flx.VFix(flex=1):
          for i in range(config.relay.num_active_mods):
            self.label.append(flx.Label(flex=1,style=mod_text_style, text=config.relay.active_mods[i]))

  @flx.action
  def update_mods(self, active_mods):
    for i in range(config.relay.num_active_mods):
      self.label[i].set_text(active_mods[i])
    
  @flx.action
  def update_time(self, mod_times):
    for i in range(config.relay.num_active_mods):
      self.progress[i].set_value(mod_times[i])
