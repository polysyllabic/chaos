# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View for whether or not the chaos engine is currently paused.
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
