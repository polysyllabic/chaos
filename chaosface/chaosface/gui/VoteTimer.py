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
import chaosface.config as config
from .VoteTimerView import VoteTimerView

class VoteTimer(flx.PyWidget):
  def init(self):
    self.voteTimerView = VoteTimerView(self)
      
  @config.relay.reaction('updateVoteTime')
  def _updateVoteTime(self, *events):
    for ev in events:
      self.voteTimerView.updateTime(ev.value)

