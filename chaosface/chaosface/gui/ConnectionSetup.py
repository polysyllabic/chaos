# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Connection settings tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from nicegui import ui

import chaosface.config.globals as config
import chaosface.config.token_utils as util

from .ui_helpers import safe_int


def build_connection_tab() -> None:
  client_id = str(config.relay.get_attribute('client_id') or '')
  bot_oauth_url = util.generate_irc_oauth(client_id)
  eventsub_oauth_url = util.generate_auth_url(
    client_id, util.Scopes.CHANNEL_READ_REDEMPTIONS, util.Scopes.BITS_READ
  )

  with ui.card().classes('w-full'):
    ui.label('Connection Setup').classes('text-h6')
    status_message = ui.label('').classes('text-sm')

    with ui.row().classes('w-full gap-8'):
      with ui.column().classes('w-96'):
        ui.label('Twitch Connection').classes('text-subtitle1')
        channel_name = ui.input('Streamer channel', value=config.relay.channel_name)
        bot_name = ui.input('Bot name', value=config.relay.bot_name)
        bot_oauth = ui.input('Bot OAuth token', value=config.relay.bot_oauth).props('type=password')
        pubsub_oauth = ui.input('EventSub OAuth token', value=config.relay.pubsub_oauth).props('type=password')
        ui.link('Generate bot OAuth token', bot_oauth_url, new_tab=True)
        ui.link('Generate EventSub OAuth token', eventsub_oauth_url, new_tab=True)

      with ui.column().classes('w-96'):
        ui.label('Chaos Engine Connection').classes('text-subtitle1')
        pi_host = ui.input('Raspberry Pi address', value=config.relay.pi_host)
        listen_port = ui.number('Listen port', value=int(config.relay.listen_port), min=1, max=65535, step=1)
        talk_port = ui.number('Talk port', value=int(config.relay.talk_port), min=1, max=65535, step=1)

    def save_connection():
      need_save = False
      config.relay.bot_reboot = False

      if str(bot_name.value) != config.relay.bot_name:
        config.relay.bot_reboot = True
        config.relay.set_bot_name(str(bot_name.value))
        need_save = True
      if str(bot_oauth.value) != config.relay.bot_oauth:
        config.relay.bot_reboot = True
        config.relay.set_bot_oauth(str(bot_oauth.value))
        need_save = True
      if str(pubsub_oauth.value) != config.relay.pubsub_oauth:
        config.relay.bot_reboot = True
        config.relay.set_pubsub_oauth(str(pubsub_oauth.value))
        need_save = True
      if str(channel_name.value) != config.relay.channel_name:
        config.relay.bot_reboot = True
        config.relay.set_channel_name(str(channel_name.value))
        need_save = True
      if str(pi_host.value) != config.relay.pi_host:
        config.relay.set_pi_host(str(pi_host.value))
        need_save = True

      listen = safe_int(listen_port.value, config.relay.listen_port, 1, 65535)
      talk = safe_int(talk_port.value, config.relay.talk_port, 1, 65535)
      if listen != config.relay.listen_port:
        config.relay.set_listen_port(listen)
        need_save = True
      if talk != config.relay.talk_port:
        config.relay.set_talk_port(talk)
        need_save = True

      if need_save:
        config.relay.set_need_save(True)
        if config.relay.bot_reboot:
          status_message.text = 'Restarting bot with new credentials'
        else:
          status_message.text = 'Updated'
      else:
        status_message.text = 'No changes'

    ui.button('Save', on_click=save_connection)
