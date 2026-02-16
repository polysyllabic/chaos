# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Connection settings tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from nicegui import ui

import chaosface.config.globals as config
import chaosface.config.security_utils as security_utils
import chaosface.config.token_utils as util

from .ui_helpers import safe_int


def build_connection_tab() -> None:
  bot_oauth_url = '/api/oauth/start?target=bot'
  eventsub_oauth_url = '/api/oauth/start?target=eventsub'
  callback_uri = f'http://localhost{util.REDIRECT_PATH}'
  client = getattr(ui.context, 'client', None)
  request = getattr(client, 'request', None)
  if request is not None:
    callback_uri = f"{str(request.base_url).rstrip('/')}{util.REDIRECT_PATH}"

  with ui.card().classes('w-full'):
    ui.label('Connection Setup').classes('text-h6')
    status_message = ui.label('').classes('text-sm')

    with ui.row().classes('w-full gap-8'):
      with ui.column().classes('w-96'):
        ui.label('Twitch Connection').classes('text-subtitle1')
        channel_name = ui.input('Streamer channel', value=config.relay.channel_name)
        bot_name = ui.input('Bot name', value=config.relay.bot_name)
        bot_oauth = ui.input('Bot OAuth token', value=config.relay.bot_oauth).props('type=password')
        eventsub_oauth = ui.input('EventSub OAuth token', value=config.relay.eventsub_oauth).props('type=password')
        ui.label(
          f'Configure your Twitch app redirect URI as {callback_uri}'
        ).classes('text-caption')
        ui.link('Start bot OAuth login', bot_oauth_url, new_tab=True)
        ui.link('Start EventSub OAuth login', eventsub_oauth_url, new_tab=True)

      with ui.column().classes('w-96'):
        ui.label('Chaos Engine Connection').classes('text-subtitle1')
        pi_host = ui.input('Raspberry Pi address', value=config.relay.pi_host)
        listen_port = ui.number('Listen port', value=int(config.relay.listen_port), min=1, max=65535, step=1)
        talk_port = ui.number('Talk port', value=int(config.relay.talk_port), min=1, max=65535, step=1)

      with ui.column().classes('w-96'):
        ui.label('UI Security').classes('text-subtitle1')
        auth_mode_default = 'password' if config.relay.ui_auth_mode == 'password' else 'none'
        auth_mode = ui.select(
          {
            'password': 'Require password',
            'none': 'No password',
          },
          value=auth_mode_default,
          label='UI access mode',
        )
        ui_password = ui.input('UI password (set/change)', password=True, password_toggle_button=True)
        tls_mode = ui.select(
          {
            'off': 'HTTP only',
            'self-signed': 'HTTPS with self-signed certificate',
            'custom': 'HTTPS with custom certificate',
          },
          value=config.relay.ui_tls_mode,
          label='TLS mode',
        )
        tls_hostname = ui.input(
          'Self-signed certificate hostname',
          value=config.relay.ui_tls_selfsigned_hostname,
        )
        tls_cert_file = ui.input('Custom TLS cert path', value=config.relay.ui_tls_cert_file)
        tls_key_file = ui.input('Custom TLS key path', value=config.relay.ui_tls_key_file)
        ui.label('Security/TLS changes require restarting chaosface service.').classes('text-caption')

    def load_generated_tokens():
      loaded = []

      bot_token = util.consume_generated_oauth('bot')
      if bot_token:
        bot_oauth.value = bot_token
        loaded.append('bot')

      eventsub_token = util.consume_generated_oauth('eventsub')
      if eventsub_token:
        eventsub_oauth.value = eventsub_token
        loaded.append('eventsub')

      if loaded:
        status_message.text = f"Loaded {' and '.join(loaded)} token(s). Click Save to apply."
      else:
        status_message.text = 'No generated tokens available yet'

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
      if str(eventsub_oauth.value) != config.relay.eventsub_oauth:
        config.relay.bot_reboot = True
        config.relay.set_eventsub_oauth(str(eventsub_oauth.value))
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

      selected_auth_mode = str(auth_mode.value or 'none').strip().lower()
      entered_ui_password = str(ui_password.value or '')
      if selected_auth_mode == 'password':
        encrypted_password = config.relay.ui_password_encrypted
        if entered_ui_password:
          encrypted_password = security_utils.encrypt_password(entered_ui_password)
          if not encrypted_password:
            status_message.text = 'Failed to store UI password'
            return
        if not encrypted_password:
          status_message.text = 'Password mode requires a password'
          return
        if config.relay.ui_auth_mode != 'password':
          config.relay.set_ui_auth_mode('password')
          need_save = True
        if encrypted_password != config.relay.ui_password_encrypted:
          config.relay.set_ui_password_encrypted(encrypted_password)
          need_save = True
      else:
        if config.relay.ui_auth_mode != 'none':
          config.relay.set_ui_auth_mode('none')
          need_save = True
        if config.relay.ui_password_encrypted:
          config.relay.set_ui_password_encrypted('')
          need_save = True

      tls_mode_value = str(tls_mode.value or 'off').strip().lower()
      if tls_mode_value not in ('off', 'self-signed', 'custom'):
        tls_mode_value = 'off'
      tls_cert_value = str(tls_cert_file.value or '').strip()
      tls_key_value = str(tls_key_file.value or '').strip()
      tls_hostname_value = str(tls_hostname.value or '').strip() or 'localhost'
      if tls_mode_value == 'custom' and (not tls_cert_value or not tls_key_value):
        status_message.text = 'Custom TLS mode requires both cert and key paths'
        return
      if tls_mode_value != config.relay.ui_tls_mode:
        config.relay.set_ui_tls_mode(tls_mode_value)
        need_save = True
      if tls_cert_value != config.relay.ui_tls_cert_file:
        config.relay.set_ui_tls_cert_file(tls_cert_value)
        need_save = True
      if tls_key_value != config.relay.ui_tls_key_file:
        config.relay.set_ui_tls_key_file(tls_key_value)
        need_save = True
      if tls_hostname_value != config.relay.ui_tls_selfsigned_hostname:
        config.relay.set_ui_tls_selfsigned_hostname(tls_hostname_value)
        need_save = True

      if need_save:
        config.relay.set_need_save(True)
        if config.relay.bot_reboot:
          status_message.text = 'Restarting bot with new credentials'
        else:
          status_message.text = 'Updated'
      else:
        status_message.text = 'No changes'

    ui.button('Load generated tokens', on_click=load_generated_tokens)
    ui.button('Save', on_click=save_connection)
