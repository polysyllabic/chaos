from flexx import flx
from config import model

class Votes(flx.PyWidget):
  def init(self):
    self.chaosVoteView = VoteView(self)
    
#  @relay.reaction('updateMods')
  def updateMods(self, *events):
    for ev in events:
      self.chaosVoteView.updateMods(ev.value)
      
#  @relay.reaction('updateVotes')
  def updateVotes(self, *events):
    for ev in events:
      self.chaosVoteView.updateNumbers(ev.value)
  
class VoteView(flx.PyWidget):
  def init(self, model):
    super().init()
    self.model = model
    
    self.label = []
    self.progress = []

    styleModText = model.get_value('mod_text_style')
    styleTitleText = model.get_value('title_text_style')
    styleProgress = model.get_value('progress_style')

    totalVotes = sum(self.model.relay.votes)
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.voteLabel = flx.Label(flex=0,style=styleTitleText, text="Total Votes: " + str(int(totalVotes)) )
        self.blankLabel = flx.Label(flex=0,style=styleTitleText, text=" ")
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(model.get_value('vote_options')):
            self.progress.append(flx.ProgressBar(flex=2, value=1.0/len(self.model.relay.votes), text='{percent}', style=styleProgress))
              
        with flx.VFix(flex=1):
          for i in range(model.get_value('vote_options')):
            self.label.append(flx.Label(flex=1,style=styleModText, text=str(i+1)))

  #@flx.action
  def updateMods(self, mods):
    for i in range(model.get_value('active_modifiers')):
      self.label[i].set_text(f"{i} {mods[i]}")
  
  #@flx.action
  def updateVotes(self, votes):
    totalVotes = sum(votes)    
    self.voteLabel.set_text(f"Total Votes: {totalVotes}")
    if totalVotes > 0:
      for i in range(model.get_value('vote_options')):
        self.progress[i].set_value(votes[i]/totalVotes)
    else:
      for i in range(model.get_value('vote_options')):
        self.progress[i].set_value(1.0/len(self.progress))

