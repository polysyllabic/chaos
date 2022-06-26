# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View for settings to connect to Twitch and to the Chaos engine
"""
from flexx import flx
import chaosface.config.globals as config

class ConnectionSetup(flx.PyWidget):
  def init(self):
    super().init()
    
    label_style = "text-align:right"
    field_style = "background-color:#BBBBBB;text-align:center"
    
    with flx.VSplit(flex=1):
      flx.Label(style="font-weight: bold; text-align:center", text="Twitch Connection" )
      flx.Label(style="text-align:center", wrap=True, html='<a href="https://twitchapps.com/tmi/" target="_blank">Click here to get your bot\'s OAuth Token</a>.  You must be logged in as your bot.' )
      with flx.HBox():
        with flx.VBox(flex=1):
          flx.Widget(flex=1)
        with flx.VBox():
          flx.Label(style=label_style, text="Twitch Bot Username:" )
          flx.Label(style=label_style, text="Twitch Bot Oauth:" )
          flx.Label(style=label_style, text="Your Channel Name:" )
        with flx.VBox(flex=1):
          self.bot_name = flx.LineEdit(style=field_style, text=config.relay.bot_name)
          self.bot_oauth = flx.LineEdit(style=field_style, text=config.relay.bot_oauth, password_mode=True)
          self.channel_name = flx.LineEdit(style=field_style, text=config.relay.channel_name)
        with flx.VBox(flex=1):
          flx.Widget(flex=1)
      flx.Label(style="font-weight: bold; text-align:center", text="Chaos Engine Connection")
      with flx.HBox():
        with flx.VBox(flex=1):
          flx.Widget(flex=1)
        with flx.VBox():
          flx.Label(style=label_style, text="Raspberry Pi Address:" )
          flx.Label(style=label_style, text="Listen Port:" )
          flx.Label(style=label_style, text="Talk Port:" )
        with flx.VBox(flex=1):
          self.pi_host = flx.LineEdit(style=field_style, text=config.relay.pi_host)
          self.listen_port = flx.LineEdit(style=field_style, text=str(config.relay.listen_port))
          self.talk_port = flx.LineEdit(style=field_style, text=str(config.relay.talk_port))
        with flx.VBox(flex=1):
          flx.Widget(flex=1)
      with flx.HBox():
        flx.Widget(flex=1)
        self.save_button = flx.Button(flex=0,text="Save")
        flx.Widget(flex=1)
      with flx.HBox():
        flx.Widget(flex=1)
        self.status_message = flx.Label(style="text-align:center", text="" )
        flx.Widget(flex=1)
      
      with flx.VBox(minsize=450):
        self.status_box = flx.MultiLineEdit(flex=2, style="text-align:left; background-color:#CCCCCC;")

  @flx.reaction('listen_port.text')
  def _listen_port_changed(self, *events):
    listen = self.validate_int(self.listen_port.text, 1, 65535, 'listen_port')
    self.listen_port.set_text(str(listen))

  @flx.reaction('talk_port.text')
  def _talk_port_changed(self, *events):
    talk = self.validate_int(self.talk_port.text, 1, 65535, 'talk_port')
    self.talk_port.set_text(str(talk))

  
  def validate_int(self, field: str, minval=None, maxval=None, fallback=None):
    good = True    
    value = config.relay.get_attribute(fallback) if fallback is not None else 0
    if not field.isdigit():
      good = False      
    else:
      value = int(field)
      if minval is not None and value < minval:
        value = minval
        good = False
      elif maxval is not None and value > maxval:
        value = maxval
        good = False
    if good:
      self.status_box.set_text('')
    else:
      if minval is None and maxval is None:      
        self.status_box.set_text(f"Enter an integer.")
      elif minval is None:
        self.status_box.set_text(f"Enter an integer less than or equal to {maxval}.")
      elif maxval is None:
        self.status_box.set_text(f"Enter an integer greater than or equal to {minval}.")
      else:
        self.status_box.set_text(f"Enter an integer between {minval} and {maxval}.")
    return value
    

  @flx.reaction('save_button.pointer_click')
  def _button_clicked(self, *events):
    need_save = False
    if self.bot_name.text != config.relay.bot_name:
      need_save = True
      config.relay.change_bot_name(self.bot_name.text)
    if self.bot_oauth.text != config.relay.bot_oauth:   
      need_save = True   
      config.relay.change_bot_oauth(self.bot_oauth.text)    
    if self.channel_name.text != config.relay.channel_name:
      need_save = True
      config.relay.change_channel_name(self.channel_name.text)
    if self.pi_host.text != config.relay.pi_host:
      need_save = True
      config.relay.change_pi_host(self.pi_host.text)
    if int(self.listen_port.text) != config.relay.listen_port:
      need_save = True
      config.relay.change_listen_port(int(self.listen_port.text))
    if int(self.talk_port.text) != config.relay.talk_port:
      need_save = True
      config.relay.change_talk_port(int(self.talk_port.text))
    if need_save == True:
      config.relay.set_need_save(True)
      self.status_message.set_text('Updated!')
    else:
      self.status_message.set_text('No Change')


