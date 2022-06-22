# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  The main page for the streamer while running the game. It provides both the streamer's view to
  look at while playing as well as tabs with the settings for the game, the chatbot, and the chaos
  engine.
"""
from flexx import flx, ui
import chaosface.config.globals as config

from chaosface.gui.StreamerInterface import StreamerInterface
from chaosface.gui.GameSettings import GameSettings
from chaosface.gui.ConnectionSetup import ConnectionSetup

class ChaosInterface(flx.PyWidget):

  def init(self):
    self.model = config
    self.set_title('Twitch Controls Chaos')
    with ui.TabLayout(style="background: #aaa; color: #000; text-align: center; foreground-color:#808080"):
      self.interface = StreamerInterface(title='Streamer Interface')
      self.game_settings = GameSettings(title='Game Settings')
#     self.ui_settings = GUISettings(title='UI Settings')    
      self.botSetup = ConnectionSetup(title='Connection Setup')

  def dispose(self):
    super().dispose()
    # ... do clean up or notify other parts of the app
    self.interface.dispose()
    self.game_settings.dispose()
    self.botSetup.dispose()

 