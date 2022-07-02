# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
  View for settings to connect to Twitch and to the Chaos engine
"""
import logging
from flexx import flx
import chaosface.config.globals as config
import chaosface.config.token_utils as util
from twitchbot import cfg

REDIRECT_URL = 'https://twitchapps.com/tmi/'

class ConnectionSetup(flx.PyWidget):
  def init(self):
    super().init()
    
    label_style = "text-align:right"
    field_style = "background-color:#BBBBBB;text-align:center"
    bot_oauth = util.generate_irc_oauth(cfg['client_id'])
    logging.debug(f'Bot oauth url={bot_oauth}')
    pubsub_oauth = util.generate_auth_url(cfg['client_id'], util.Scopes.PUBSUB_CHANNEL_POINTS, util.Scopes.PUBSUB_BITS)
    logging.debug(f'Pubsub oauth url={pubsub_oauth}')
    instructions=('To get the bot\'s OAuth token,  '
      '<a href="{bot}" target="_blank">log in as your bot and then click here.</a> ' 
      'Paste the generated token into the Bot OAuth field. '
      'To use points redemptions, you must also get a separate OAuth token. Log in with your '
      'streamer\'s account <a href="{pubsub}"target="_blank">and click here.</a>.')

    with flx.VBox():
      with flx.GroupWidget(flex=1, title="Twitch Connection"):
        with flx.VBox():
          with flx.HBox():
            with flx.VBox():
              flx.Label(style=label_style, text="Streamer Channel:")
              flx.Label(style=label_style, text="Bot Name:")
              flx.Label(style=label_style, text="Bot Oauth Token:")
              flx.Label(style=label_style, text="PubSub Oauth Token:")
            with flx.VBox(flex=1):
              self.channel_name = flx.LineEdit(style=field_style, text=config.relay.channel_name)
              self.bot_name = flx.LineEdit(style=field_style, text=config.relay.bot_name)
              self.bot_oauth = flx.LineEdit(style=field_style, text=config.relay.bot_oauth, password_mode=True)
              self.pubsub_oauth = flx.LineEdit(style=field_style, text=config.relay.pubsub_oauth, password_mode=True)
          flx.Label(flex=1, wrap=True, html=instructions.format(bot=bot_oauth, pubsub=pubsub_oauth))  
      with flx.GroupWidget(flex=1, title="Chaos Engine Connection"):
        with flx.HBox():
          with flx.VBox(flex=1):
            flx.Widget(flex=1)
          with flx.VBox():
            flx.Label(style=label_style, text="Raspberry Pi Address:")
            flx.Label(style=label_style, text="Listen Port:") 
            flx.Label(style=label_style, text="Talk Port:")
          with flx.VBox(flex=1):
            self.pi_host = flx.LineEdit(style=field_style, text=config.relay.pi_host)
            self.listen_port = flx.LineEdit(style=field_style, text=str(config.relay.listen_port))
            self.talk_port = flx.LineEdit(style=field_style, text=str(config.relay.talk_port))
          with flx.VBox(flex=1):
            flx.Widget(flex=1)
      with flx.HBox():
        flx.Widget(flex=1)
        self.save_button = flx.Button(flex=0, text="Save")
        self.status_message = flx.Label(text="")
        flx.Widget(flex=1)
      
      with flx.VBox(minsize=450):
        self.status_message = flx.MultiLineEdit(flex=2, style="text-align:left; background-color:#CCCCCC;")

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
      self.status_message.set_text('')
    else:
      if minval is None and maxval is None:      
        self.status_message.set_text(f"Enter an integer.")
      elif minval is None:
        self.status_message.set_text(f"Enter an integer less than or equal to {maxval}.")
      elif maxval is None:
        self.status_message.set_text(f"Enter an integer greater than or equal to {minval}.")
      else:
        self.status_message.set_text(f"Enter an integer between {minval} and {maxval}.")
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
    if self.pubsub_oauth.text != config.relay.pubsub_oauth:
      need_save = True   
      config.relay.change_pubsub_oauth(self.pubsub_oauth.text)    
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

#  @flx.reaction('generate_token_button.pointer_click')
#  def on_generate_token(self, *events):
#    oauth = util.generate_irc_oauth(cfg['client_id'])
#    logging.debug(f'oauth url = {oauth}')
    

