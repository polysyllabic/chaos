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
import chaosface.config.globals as config

class ChaosPausedView(flx.PyWidget):
	def init(self):
		super().init()

		self.dark_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:50px; min-width:250px; line-height: 2.0; background-color:black"
		self.bright_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:50px; min-width:250px; line-height: 2.0; background-color:white"

		if config.relay.paused:
			text="Paused"
		else:
			text="Running"

		self.paused_message = flx.Label(flex=3, style=self.dark_style, text=text)

	@flx.action
	def update_paused(self, paused):
		if paused:
			self.paused_message.set_text("Paused")
		else:
			self.paused_message.apply_style(self.dark_style)
			self.paused_message.set_text("Running")

	@flx.action
	def update_paused_bright(self, bright):
		if bright:
			self.paused_message.apply_style(self.bright_style)
		else:
			self.paused_message.apply_style(self.dark_style)
