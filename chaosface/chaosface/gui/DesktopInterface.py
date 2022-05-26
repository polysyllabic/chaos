import PySimpleGUI as sg

from configs import relay

import StreamerInterface
import Settings
import BotConfiguration

class DesktopInterface():

  def __init__(self):
    self.streamer_interface = StreamerInterface(self.relay, title='Interface')
    self.settings = Settings(self.relay, title='Settings')
    self.configuration = BotConfiguration(self.relay, title='Bot Setup')

    self.layout = [[sg.Tab('Streaming View', self.streamer_interface, key='-streamer-')],
      [sg.Tab('Configuration', self.configuration, key='-configuration-')]
    ]
    self.window = sg.Window('Twitch Controls Chaos', self.layout, default_element_size=(12,1))

  def _loop(self):
    while True:
      event, values = self.window.read()
      if event == sg.WIN_CLOSED:
        break

      # Update 