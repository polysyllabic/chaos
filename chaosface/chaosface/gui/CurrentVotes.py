
from flexx import flx
import chaosface.config.globals as config

class CurrentVotes(flx.PyWidget):
  def init(self):
    self.chaosVoteView = ChaosVoteView()
    
  @config.relay.reaction('updateMods')
  def _updateMods(self, *events):
    for ev in events:
      self.chaosVoteView.updateMods(ev.value)
      
  @config.relay.reaction('updateVotes')
  def _updateVotes(self, *events):
    for ev in events:
      self.chaosVoteView.updateNumbers(ev.value)
  

class ChaosVoteView(flx.PyWidget):
  def init(self):
    super().init()
    
    self.label = []
    self.progress = []
    
    styleModText = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;"
    styleTitleText = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;"
    styleVoteProgress = " background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black; font-weight: bold;"

    totalVotes = sum(config.relay.votes)
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.voteLabel = flx.Label(flex=0,style=styleTitleText, text="Total Votes: " + str(int(totalVotes)) )
        self.blankLabel = flx.Label(flex=0,style=styleTitleText, text=" ")
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(len(config.relay.votes)):
            if totalVotes > 0:
              self.progress.append(flx.ProgressBar(flex=2, value=config.relay.votes[i]/totalVotes, text='{percent}', style=styleVoteProgress))
            else:
              self.progress.append( flx.ProgressBar(flex=2, value=1.0/len(config.relay.votes), text='{percent}', style=styleVoteProgress))
              
        with flx.VFix(flex=1):
          for i in range(len(config.relay.votes)):
            self.label.append(flx.Label(flex=1,style=styleModText, text=str(i+1) + " " + config.relay.mods[i]))

  @flx.action
  def updateMods(self, mods):
    for i in range(len(mods)):
      self.label[i].set_text(f"{i} {mods[i]}")
  
  @flx.action
  def updateNumbers(self, votes):
    totalVotes = sum(votes)
    self.voteLabel.set_text(f"Total Votes: {totalVotes}")
    for i in range(len(votes)):
      if totalVotes > 0:
        self.progress[i].set_value(votes[i]/totalVotes)
      else:
        self.progress[i].set_value(1.0/len(votes))
