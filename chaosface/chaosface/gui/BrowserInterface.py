from flexx import flx, ui

import StreamerInterface
import SettingsView
import BotConfiguration

class BrowserInterface(flx.PyWidget):

	def init(self):
		with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080") as self.t:
			self.interface = StreamerInterface(title='Streamer View')
			self.settings = SettingsView(title='Settings')
			self.botSetup = BotConfiguration(title='Chat Bot')

	# Clean up 
	def dispose(self):
		super().dispose()
		self.interface.dispose()
		self.settings.dispose()
		self.botSetup.dispose()
