from flexx import flx, ui
from configs import relay

import StreamerInterface
import Settings
import BotConfiguration

class BrowserInterface(flx.PyWidget):

	def init(self):
		with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080") as self.t:
		#with StreamerInterfaceLayout() as self.s:
			self.interface = StreamerInterface(relay, title='Interface')
			self.settings = Settings(relay, title='Settings')
			self.botSetup = BotConfiguration(relay, title='Bot Setup')

	def dispose(self):
		super().dispose()
		# ... do clean up or notify other parts of the app
		self.interface.dispose()
		self.settings.dispose()
		self.botSetup.dispose()
