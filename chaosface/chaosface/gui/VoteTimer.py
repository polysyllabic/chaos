from flexx import flx
from chaosface.config import relay
from views import VoteTimerView

class VoteTimer(flx.PyWidget):
	def init(self, relay):
		self.relay = relay
		self.voteTimerView = VoteTimerView(self)
			
	@relay.reaction('updateVoteTime')
	def _updateVoteTime(self, *events):
		for ev in events:
			self.voteTimerView.updateTime(ev.value)
