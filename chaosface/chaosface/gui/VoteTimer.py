# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  Browser source showing the time remaining for the current voting round
"""
from flexx import flx
import chaosface.config.globals as config

class VoteTimerView(flx.PyWidget):
	def init(self):
		super().init()
		
		time_style = " background-color:#808080; foreground-color:#808080; color:#000000; border-color:#000000; border-radius:5px"
		self.progress_time = flx.ProgressBar(flex=1, value=config.relay.vote_time, text='', style=time_style)
		
	@flx.action
	def update_time(self, vote_time):
		self.progress_time.set_value(vote_time)

class VoteTimer(flx.PyWidget):
  def init(self):
    self.vote_timer_view = VoteTimerView()
      
  @config.relay.reaction('update_vote_time')
  def _update_vote_time(self, *events):
    for ev in events:
      self.vote_timer_view.update_time(ev.value)




