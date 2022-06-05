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
from chaosface.config.globals import relay

class ActiveModsView(flx.PyWidget):
	def init(self):
		super().init()
		self.label = []
		self.progress = []
		
		styleModText = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;"
		styleTitleText = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;"
		styleModProgress = " background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; width:1050px;"
		
		with flx.VBox(flex=0):
			with flx.HFix(flex=1):
				self.voteLabel = flx.Label(flex=0,style=styleTitleText, text="Active Mods")
				self.blankLabel = flx.Label(flex=0,style=styleTitleText, text=" ")
				
			with flx.HFix(flex=1):
				with flx.VFix(flex=1):
					for i in range(3):
						self.progress.append(flx.ProgressBar(flex=2, value=relay.modTimes[i], text='', style=styleModProgress))
				with flx.VFix(flex=1):
					for i in range(3):
						self.label.append(flx.Label(flex=1,style=styleModText, text=relay.activeMods[i]))

	@flx.action
	def updateMods(self, activeMods):
		self.label[0].set_text(activeMods[0])
		self.label[1].set_text(activeMods[1])
		self.label[2].set_text(activeMods[2])
		
	@flx.action
	def updateTime(self, modTimes):
		self.progress[0].set_value(modTimes[0])
		self.progress[1].set_value(modTimes[1])
		self.progress[2].set_value(modTimes[2])
