from flexx import flx
from config import model
from views import VoteTimerView

class VoteTimer(flx.PyWidget):
  def init(self):
    self.voteTimerView = VoteTimerView(self)
      
#  @relay.reaction('updateVoteTime')
  def updateVoteTime(self, *events):
    for ev in events:
      self.voteTimerView.updateTime(ev.value)
