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
import chaosface.config.globals as config

from .ChaosPausedView import ChaosPausedView
from .ChaosConnectionView import ChaosConnectionView
from .ActiveMods import ChaosActiveView
from .VoteTimer import VoteTimerView

class StreamerInterfaceLayout(ui.HVLayout):
  def init(self):
    self.set_flex(1)
    self.set_orientation('v')
    self.apply_style('background:#000000;')

class StreamerInterface(flx.PyWidget):

  def init(self):
    
    with flx.VBox():
      self.vote_timer_view = VoteTimerView()
      self.chaos_active_view = ChaosActiveView()
      self.chaos_connection_view = ChaosConnectionView()
      self.chaos_paused_view = ChaosPausedView()
      self.quit_button = flx.Button(flex=0, text="Quit")
  
  @flx.reaction('quit_button.pointer_click')
  def _quit_button_clicked(self, *events):
    ev = events[-1]
    # TODO: pop-up confirmation
    config.relay.keep_going = False

  @config.relay.reaction('update_vote_time')
  def _update_vote_time(self, *events):
    for ev in events:
      self.vote_timer_view.update_time(ev.value)
      
  @config.relay.reaction('update_mod_times')
  def _update_mod_times(self, *events):
    for ev in events:
      self.chaos_active_view.update_time(ev.value)
      
  @config.relay.reaction('update_active_mods')
  def _update_active_mods(self, *events):
    for ev in events:
      self.chaos_active_view.update_mods(ev.value)
      
  @config.relay.reaction('update_paused')
  def _update_paused(self, *events):
    for ev in events:
      self.chaos_paused_view.update_paused(ev.value)
      
  @config.relay.reaction('update_paused_bright')
  def _update_paused_bright(self, *events):
    for ev in events:
      self.chaos_paused_view.update_paused_bright(ev.value)
      
  @config.relay.reaction('update_connected')
  def _updateConnected(self, *events):
    for ev in events:
      self.chaos_connection_view.update_connected(ev.value)
      
  @config.relay.reaction('update_connected_bright')
  def _updateConnectedBrightBackground(self, *events):
    for ev in events:
      self.chaos_connection_view.update_connected_bright(ev.value)
