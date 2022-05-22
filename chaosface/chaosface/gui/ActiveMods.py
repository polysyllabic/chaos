from flexx import flx

from chaosface.config import relay
from views import ActiveView

class ActiveMods(flx.PyWidget):
	def init(self, relay):
		self.relay = relay
		self.chaosActiveView = ActiveView(self)
		
	@relay.reaction('updateModTimes')
	def _updateModTimes(self, *events):
		for ev in events:
			self.chaosActiveView.updateTime(ev.value)
			
	@relay.reaction('updateActiveMods')
	def _updateActiveMods(self, *events):
		for ev in events:
			self.chaosActiveView.updateMods(ev.value)
