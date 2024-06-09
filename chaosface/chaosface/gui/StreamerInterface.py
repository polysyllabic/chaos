# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The view for the main page streamers will look at while playing the game.
"""
from flexx import flx

import chaosface.config.globals as config

from .ActiveMods import ActiveMods
from .ChaosConnectionView import ChaosConnectionView
from .ChaosPausedView import ChaosPausedView
from .VoteTimer import VoteTimer


class StreamerInterface(flx.PyWidget):

  def init(self):
    
    with flx.VBox():
      self.vote_timer_view = VoteTimer()
      self.chaos_active_view = ActiveMods()
      self.chaos_connection_view = ChaosConnectionView()
      self.chaos_paused_view = ChaosPausedView()
      self.quit_button = flx.Button(flex=0, text="Quit")
  
  @flx.reaction('quit_button.pointer_click')
  def _quit_button_clicked(self, *events):
    ev = events[-1]
    # TODO: pop-up confirmation
    config.relay.keep_going = False

  #@config.relay.reaction('update_vote_time')
  #def _update_vote_time(self, *events):
  #  for ev in events:
  #    self.vote_timer_view.update_time(ev.value)
      
  #@config.relay.reaction('update_mod_times')
  #def _update_mod_times(self, *events):
  #  for ev in events:
  #    self.chaos_active_view.update_time(ev.value)
      
  #@config.relay.reaction('update_active_mods')
  #def _update_active_mods(self, *events):
  #  for ev in events:
  #    self.chaos_active_view.update_mods(ev.value)
      
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
