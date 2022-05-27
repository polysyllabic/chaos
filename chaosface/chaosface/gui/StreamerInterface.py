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
from flexx import flx, ui
from config import relay
from gui import VoteTimerView, ActiveModsView, ConnectionView, PausedView

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
			self.activeModsView = ActiveModsView(self)
			self.connectionView = ConnectionView(self)
			self.pausedView = PausedView(self)
			
	@relay.reaction('updateVoteTime')
	def _updateVoteTime(self, *events):
		for ev in events:
			self.voteTimerView.updateTime(ev.value)
			
	@relay.reaction('updateModTimes')
	def _updateModTimes(self, *events):
		for ev in events:
			self.activeModsView.updateTime(ev.value)
			
	@relay.reaction('updateActiveMods')
	def _updateActiveMods(self, *events):
		for ev in events:
			self.activeModsView.updateMods(ev.value)
			
	@relay.reaction('updatePaused')
	def _updatePaused(self, *events):
		for ev in events:
			self.pausedView.updatePaused(ev.value)
			
	@relay.reaction('updatePausedBrightBackground')
	def _updatePausedBrightBackground(self, *events):
		for ev in events:
			self.pausedView.updatePausedBrightBackground(ev.value)
			
	@relay.reaction('updateConnected')
	def _updateConnected(self, *events):
		for ev in events:
			self.connectionView.updateConnected(ev.value)
			
	@relay.reaction('updateConnectedBrightBackground')
	def _updateConnectedBrightBackground(self, *events):
		for ev in events:
			self.connectionView.updateConnectedBrightBackground(ev.value)
