from flexx import flx, ui

from views import ConnectionView, VoteTimerView
from views import ActiveView

from chaosface.configs.config import relay

class StreamerInterfaceLayout(ui.HVLayout):
	def init(self):
		self.set_flex(1)
		self.set_orientation('v')
		self.apply_style('background:#000000;')

class StreamerInterface(flx.PyWidget):

	def init(self, relay):
		self.relay = relay
		with StreamerInterfaceLayout() as self.s:
			self.voteTimerView = VoteTimerView(self)
			self.chaosActiveView = ActiveView(self)
			self.chaosConnectionView = ConnectionView(self)
			self.chaosPausedView = PausedView(self)
			
	@relay.reaction('updateVoteTime')
	def _updateVoteTime(self, *events):
		for ev in events:
			self.voteTimerView.updateTime(ev.value)
			
	@relay.reaction('updateModTimes')
	def _updateModTimes(self, *events):
		for ev in events:
			self.chaosActiveView.updateTime(ev.value)
			
	@relay.reaction('updateActiveMods')
	def _updateActiveMods(self, *events):
		for ev in events:
			self.chaosActiveView.updateMods(ev.value)
			
	@relay.reaction('updatePaused')
	def _updatePaused(self, *events):
		for ev in events:
			self.chaosPausedView.updatePaused(ev.value)
			
	@relay.reaction('updatePausedBrightBackground')
	def _updatePausedBrightBackground(self, *events):
		for ev in events:
			self.chaosPausedView.updatePausedBrightBackground(ev.value)
			
	@relay.reaction('updateConnected')
	def _updateConnected(self, *events):
		for ev in events:
			self.chaosConnectionView.updateConnected(ev.value)
			
	@relay.reaction('updateConnectedBrightBackground')
	def _updateConnectedBrightBackground(self, *events):
		for ev in events:
			self.chaosConnectionView.updateConnectedBrightBackground(ev.value)
			
class PausedView(flx.PyWidget):
	def init(self, model):
		super().init()
		self.model = model

		self.stylePausedDark = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:50px; min-width:250px; line-height: 2.0; background-color:black"
		self.stylePausedBright = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: middle; font-size:50px; min-width:250px; line-height: 2.0; background-color:white"
		if self.model.relay.paused:
			text="Paused"
		else:
			text="Running"
		self.pausedText = flx.Label(flex=3, style=self.stylePausedDark, text=text )

	@flx.action
	def updatePaused(self, paused):
		if paused:
			self.pausedText.set_text("Paused")
		else:
			self.pausedText.apply_style(self.stylePausedDark)
			self.pausedText.set_text("Running")


	@flx.action
	def updatePausedBrightBackground(self, pausedBright):
		if pausedBright:
			self.pausedText.apply_style(self.stylePausedBright)
		else:
			self.pausedText.apply_style(self.stylePausedDark)
