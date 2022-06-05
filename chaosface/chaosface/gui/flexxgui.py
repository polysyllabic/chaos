
import asyncio
from flexx import flx, ui

#from chaosface.config import CHAOS_CONFIG_FILE
from chaosface.config.globals import relay

from chaosface.gui.ActiveModsView import ActiveModsView
from chaosface.gui.VoteView import VoteView
from chaosface.gui.VoteTimerView import VoteTimerView
from chaosface.gui.ConnectionView import ConnectionView
from chaosface.gui.PausedView import PausedView
from chaosface.gui.SettingsView import SettingsView
from chaosface.gui.BotConfigurationView import BotConfigurationView

# Source for active mods overlay
class ActiveMods(flx.PyWidget):

  def init(self):
    self.chaosActiveView = ActiveModsView()
    
  @relay.reaction('updateModTimes')
  def _updateModTimes(self, *events):
    for ev in events:
      self.chaosActiveView.updateTime(ev.value)
      
  @relay.reaction('updateActiveMods')
  def _updateActiveMods(self, *events):
    for ev in events:
      self.chaosActiveView.updateMods(ev.value)

# Source for current votes overlay
class Votes(flx.PyWidget):
  def init(self):
    self.chaosVoteView = VoteView()
    
  @relay.reaction('updateMods')
  def _updateMods(self, *events):
    for ev in events:
      self.chaosVoteView.updateMods(ev.value)
      
  @relay.reaction('updateVotes')
  def _updateVotes(self, *events):
    for ev in events:
      self.chaosVoteView.updateNumbers(ev.value)

# Source for vote time remaining
class VoteTimer(flx.PyWidget):
  def init(self):
    self.voteTimerView = VoteTimerView()
      
  @relay.reaction('updateVoteTime')
  def _updateVoteTime(self, *events):
    for ev in events:
      self.voteTimerView.updateTime(ev.value)

class Settings(flx.PyWidget):
  def init(self):
    self.settingsView = SettingsView()

class StreamerInterfaceLayout(ui.HVLayout):
  def init(self):
    self.set_flex(1)
    self.set_orientation('v')
    self.apply_style('background:#000000;')

class StreamerInterface(flx.PyWidget):

  def init(self):
    with StreamerInterfaceLayout() as self.s:
      self.voteTimerView = VoteTimerView()
      self.activeModsView = ActiveModsView()
      self.connectionView = ConnectionView()
      self.pausedView = PausedView()
      
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

class BotConfiguration(flx.PyWidget):
  def init(self, relay):
    self.relay = relay
    self.configurationView = BotConfigurationView(self)
      
  @relay.reaction('updateTmiResponse')
  def _updateTmiResponse(self, *events):
    for ev in events:
      self.configurationView.updateTmiResponse(ev.value)

# Primary interface to chaos
class MainInterface(flx.PyWidget):

  def init(self):
    with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080") as self.t:
      self.interface = StreamerInterface(title='Interface')
      self.settings = Settings(title='Settings')
      self.botSetup = BotConfiguration(title='Connection')

  def dispose(self):
    super().dispose()
    # ... do clean up or notify other parts of the app
    self.interface.dispose()
    self.settings.dispose()
    self.botSetup.dispose()


def startFlexx():
  flx.App(ActiveMods).serve()
  flx.App(VoteTimer).serve()
  flx.App(Votes).serve()
  flx.App(MainInterface).serve()
  
  flx.create_server(host='0.0.0.0', port=str(relay.get_attribute("uiPort")), loop=asyncio.new_event_loop())
  flx.start()

def stopFlexx():
  flx.stop()