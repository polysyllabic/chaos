
from flexx import flx
import chaosface.config.globals as config

class VoteTimerView(flx.PyWidget):
	def init(self):
		super().init()
		
		styleTime = " background-color:#808080; foreground-color:#808080; color:#000000; border-color:#000000; border-radius:5px"
		self.progressTime = flx.ProgressBar(flex=1, value=config.relay.voteTime, text='', style=styleTime)
		
	@flx.action
	def updateTime(self, voteTime):
		self.progressTime.set_value(voteTime)

class VoteTimer(flx.PyWidget):
  def init(self):
    self.voteTimerView = VoteTimerView()
      
  @config.relay.reaction('updateVoteTime')
  def _updateVoteTime(self, *events):
    for ev in events:
      self.voteTimerView.updateTime(ev.value)




