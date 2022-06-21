"""
  Twitch Controls Chaos (TCC)
  Copyright 2021-2022 The Twitch Controls Chaos developers. See the AUTHORS
  file at the top-level directory of this distribution for details of the
  contributers.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""

from flexx import flx
import chaosface.config.globals as config

class CurrentVotes(flx.PyWidget):
  def init(self):
    self.vote_view = ChaosVoteView()
    
  @config.relay.reaction('update_candidate_mods')
  def _update_candidates(self, *events):
    for ev in events:
      self.vote_view.update_candidates(ev.value)
      
  @config.relay.reaction('update_votes')
  def _update_votes(self, *events):
    for ev in events:
      self.vote_view.update_voting(ev.value)
  

class ChaosVoteView(flx.PyWidget):
  def init(self):
    super().init()
    
    self.label = []
    self.progress = []
    
    mod_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:left;font-weight: bold; vertical-align: middle; line-height: 1.5; min-width:250px;"
    title_text_style = "color:white;text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black;text-align:center;font-weight: bold; vertical-align: bottom; line-height: 1.5; min-width:250px;"
    progress_style = " background-color:#808080; foreground-color:#808080; color:#FFFFFF; border-color:#000000; border-radius:5px; text-shadow: -1px 0 black, 0 1px black, 1px 0 black, 0 -1px black; font-weight: bold;"

    votes = sum(config.relay.votes)
    with flx.VBox(flex=0):
      with flx.HFix(flex=1):
        self.vote_total = flx.Label(flex=0,style=title_text_style, text=f"Total Votes: {votes}")
        self.blank_label = flx.Label(flex=0,style=title_text_style, text=" ")
        
      with flx.HFix(flex=1):
        with flx.VFix(flex=1):
          for i in range(len(config.relay.votes)):
            if votes > 0:
              self.progress.append(flx.ProgressBar(flex=2, value=config.relay.votes[i]/votes, text='{percent}', style=progress_style))
            else:
              self.progress.append( flx.ProgressBar(flex=2, value=1.0/len(config.relay.votes), text='{percent}', style=progress_style))
              
        with flx.VFix(flex=1):
          for i in range(len(config.relay.votes)):
            self.label.append(flx.Label(flex=1,style=mod_text_style, text=str(i+1) + " " + config.relay.candidate_mods[i]))

  @flx.action
  def update_candidates(self, mods):
    for i in range(len(mods)):
      self.label[i].set_text(f"{i} {mods[i]}")
  
  @flx.action
  def update_voting(self, votes):
    total = sum(votes)
    self.vote_total.set_text(f"Total Votes: {total}")
    for i in range(len(votes)):
      if total > 0:
        self.progress[i].set_value(votes[i]/total)
      else:
        self.progress[i].set_value(1.0/len(votes))
