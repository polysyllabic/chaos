
import PySimpleGUI as sg
from chaosface.config.globals import relay

# Just throw an exception if we have key errors
sg.set_options(suppress_raise_key_errors=False, suppress_error_popups=True, suppress_key_guessing=True)

active_mods = []
for i in range(relay.totalActiveMods):
  active_mods += [
    [sg.ProgressBar(key=f'-mod_{i} streamer-', max_value=100, size_px=(1050,12)),
     sg.Text(text=relay.activeMods[i], key=f'-mod_name{i} streamer-')],
  ]

streamer_interface = [
  [sg.Text(text=f'Game: {relay.gameName} '),
   sg.Text(text=f'(encountered {relay.gameErrors} errors loading config file)', key='-game_errors-', visible=False)],
  [sg.ProgressBar(key='-vote_timer streamer-', max_value=100, size_px=(1050,12))], 
  [sg.Frame('Active Mods',[active_mods], key='-active_mods_view streamer-')],
  [sg.HorizontalSeparator()],
  [sg.Text(text='Not Connected', key='-chatbot_status-', text_color='red')],
  [sg.VPush()],
  [sg.Push(), sg.Text(text='Chaos Engine Disconnected', key='-engine_status-', size=40,
                      justification='center', font=('Helvetica 32 bold'), pad=(0,20)), sg.Push()],
]


game_settings = [
  [sg.Text('Active Modifiers:', size=28, justification='right'),
   sg.Input(key='-active_modifiers-', size=6,
      tooltip='Number of modifiers active at one time')],
  [sg.Text('Time Per Modifier:', size=28, justification='right'),
   sg.Input(key='-modifier_time-', size=6, 
      tooltip='Time in seconds each modifier lasts before being replaced')],
  [sg.Frame('Modifier Selection'), [

  ]]
  [sg.Text('Votable Modifiers:', size=28, justification='right'),
   sg.Input(key='-votable_modifiers-', size=6,
      tooltip='Number of modifiers chat can vote on at one time')],
  [sg.Text('')],
  [sg.Text("Repeat Modifier Weighting:", size=28, justification='right'),
   sg.Slider(key='-softmax_factor-', range=(1,100), size=20,
      tooltip='Weighting against modifiers being reused (lower = repeat is less likely)')],
  [sg.Frame('Available Modifiers:', [])],
]

configuration = [
  [sg.Frame('Twitch Configuration:', [ 
    [sg.Text('Channel Name:', size=12),
     sg.Input(key='-channel_name-', size=30,
        tooltip='The name of your Twitch channel')],
    [sg.Text('Bot Name:', size=12),
     sg.Input(key='-bot_name-', size=30, password_char='*',
        tooltip='The user name of your Twitch bot (an account you control, not StreamElemens, Nightbot, etc.')],
    [sg.Text('Bot Oauth:', size=12),
     sg.Input(key='-oauth-', size=30,
        tooltip='The Oauth token for your bot (see documentation for instructions on how to get one')],
  ])],
  [sg.Frame('Chaos Engine Connection:', [
    [sg.Text("Rapberry Pi address:"), 
     sg.Input(key="-pi_host-", size=16,
        tooltip='IP address or domain name of the Raspberry Pi. If the interface is running on the Pi, use "localhost"')],
    [sg.Text("Listen to Chaos Engine on port:"),
     sg.Input(key="-listen_port-", size=8)],
    [sg.Text("Talk to Chaos Engine on port:"),
     sg.Input(key="-talk_port-", size=8)],
  ])],
  [sg.Frame('GUI Configuration', [
    [sg.Text('Update Rate (Hz):'), sg.Input(key='-ui_rate-',
        tooltip='Frequency with which the UI and browser sources are updated')],
    [sg.Checkbox(text='Generate browser sources',
        tooltip="Disable this to run chaos without OBS overlays")],
  ])],
  [sg.Push(), sg.Button('Apply', text='-apply_config-'), sg.Button('Defaults', key='-default_config-'), sg.Push()],
] 

# Primary interface to chaos
class MainInterface():

  def __init__(self):
    self.layout = [
      [sg.Tab('Streamer View', streamer_interface, key='-streamer-')],
      [sg.Tab('Game Settings', game_settings, key='-settings-')],
      [sg.Tab('Connection', configuration, key='-connection-')],
    ]
    self.window = sg.Window('Twitch Controls Chaos', self.layout, default_element_size=12)

  def run(self):
    while True:
      event, values = self.window.read()
      if event == sg.WIN_CLOSED:
        break

    self.window.close()

