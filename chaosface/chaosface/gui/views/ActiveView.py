
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

class ActiveView(flx.PyWidget):
  def init(self):
    super().init()    
    self.label = []
    self.progress = []
    
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.voteLabel = flx.Label(flex=0,style=model.get_value('title_text_style'), text="Active Mods" )
        self.blankLabel = flx.Label(text=' ', flex=1) # take remaining space
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(model.get_value('active_modifiers')):
            self.progress.append(flx.ProgressBar(flex=2, value=model.modTimes[i], text='', style=model.get_value('progress_style')) )
        with flx.VFix(flex=1):
          for i in range(model.get_value('active_modifiers')):
            self.label.append(flx.Label(flex=1,style=model.get_value('mod_text_style'), text=model.activeMods[i]) )


  @flx.action
  def updateMods(self, activeMods):
    for i in range(model.get_value('active_modifiers')):
      self.label[i].set_text(activeMods[i])
    
  @flx.action
  def updateTime(self, modTimes):
    for i in range(model.get_value('active_modifiers')):
      self.progress[i].set_value(modTimes[i])

