# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Connection settings tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from urllib.parse import urlsplit

from nicegui import ui

import chaosface.config.globals as config
import chaosface.config.security_utils as security_utils
import chaosface.config.token_utils as util

from .ui_helpers import safe_int


def _sanitize_hostname(value: str) -> str:
  raw = str(value or '').strip()
  if not raw:
    return ''
  if '://' in raw:
    parsed = urlsplit(raw)
    return parsed.hostname or ''
  # Allow host:port or host/path values.
  parsed = urlsplit(f'//{raw}')
  return parsed.hostname or ''


def _resolve_self_signed_hostname(request) -> str:
  configured = _sanitize_hostname(config.relay.ui_tls_selfsigned_hostname)
  if configured and configured.lower() != 'localhost':
    return configured

  if request is not None:
    request_host = str(getattr(request.url, 'hostname', '') or '').strip()
    if request_host and request_host.lower() != 'localhost':
      return request_host

  pi_host = _sanitize_hostname(config.relay.pi_host)
  if pi_host and pi_host.lower() != 'localhost':
    return pi_host
  return 'raspberrypi.local'


def build_connection_tab() -> None:
  def normalize_tls_mode(value: str) -> str:
    mode = str(value or 'off').strip().lower()
    if mode not in ('off', 'self-signed', 'custom'):
      return 'off'
    return mode

  def default_ui_port_for_tls_mode(mode: str) -> int:
    return 443 if normalize_tls_mode(mode) in ('self-signed', 'custom') else 80

  current_tls_mode = normalize_tls_mode(config.relay.ui_tls_mode)

  client = getattr(ui.context, 'client', None)
  request = getattr(client, 'request', None)
  callback_uri = util.DEFAULT_REDIRECT_URL
  bot_oauth_url = '/api/oauth/start?target=bot'
  eventsub_oauth_url = '/api/oauth/start?target=eventsub'
  tls_hostname_default = _resolve_self_signed_hostname(request)
  tls_hostname_initial = str(config.relay.ui_tls_selfsigned_hostname or '').strip()
  if not tls_hostname_initial or tls_hostname_initial.lower() == 'localhost':
    tls_hostname_initial = tls_hostname_default

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
        ui.label(
          'Use Chaosface through http://localhost:8080 (for Pi-hosted runs, use an SSH tunnel).'
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
          value=current_tls_mode,
          label='TLS mode',
        )
        ui_port = ui.number(
          'UI port',
          value=safe_int(config.relay.ui_port, default_ui_port_for_tls_mode(current_tls_mode), 1, 65535),
          min=1,
          max=65535,
          step=1,
        )
        tls_mode_state = {'value': current_tls_mode}

        def sync_ui_port_for_tls_mode(selected_mode: str) -> None:
          previous_mode = normalize_tls_mode(tls_mode_state['value'])
          next_mode = normalize_tls_mode(selected_mode)
          previous_default_port = default_ui_port_for_tls_mode(previous_mode)
          next_default_port = default_ui_port_for_tls_mode(next_mode)
          current_port = safe_int(ui_port.value, previous_default_port, 1, 65535)
          if current_port == previous_default_port:
            ui_port.value = next_default_port
          tls_mode_state['value'] = next_mode

        tls_mode.on(
          'update:model-value',
          lambda event: sync_ui_port_for_tls_mode(str(getattr(event, 'value', tls_mode.value or 'off'))),
        )
        tls_hostname = ui.input(
          'Self-signed certificate hostname',
          value=tls_hostname_initial,
        )
        tls_cert_file = ui.input('Custom TLS cert path', value=config.relay.ui_tls_cert_file)
        tls_key_file = ui.input('Custom TLS key path', value=config.relay.ui_tls_key_file)
        ui.label('Security/TLS changes require restarting chaosface service.').classes('text-caption')

    with ui.column().classes('w-full'):
      ui.label('Bot Diagnostics').classes('text-subtitle1')
      diagnostics_box = ui.column().classes(
        'w-full h-48 overflow-auto rounded border border-gray-300 p-2'
      )

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

    def refresh_bot_diagnostics():
      messages = config.relay.get_bot_diagnostics()
      diagnostics_box.clear()
      with diagnostics_box:
        if not messages:
          ui.label('No bot diagnostics yet.').classes('text-xs text-gray-600')
          return
        for message in messages:
          ui.label(str(message)).classes('text-xs font-mono break-all')

    def clear_bot_diagnostics():
      config.relay.clear_bot_diagnostics()
      refresh_bot_diagnostics()

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
      tls_mode_value = normalize_tls_mode(tls_mode_value)
      previous_tls_mode = normalize_tls_mode(config.relay.ui_tls_mode)
      tls_cert_value = str(tls_cert_file.value or '').strip()
      tls_key_value = str(tls_key_file.value or '').strip()
      ui_port_value = safe_int(
        ui_port.value,
        safe_int(config.relay.ui_port, default_ui_port_for_tls_mode(previous_tls_mode), 1, 65535),
        1,
        65535,
      )
      tls_hostname_value = _sanitize_hostname(str(tls_hostname.value or '').strip())
      if not tls_hostname_value:
        tls_hostname_value = tls_hostname_default
      if tls_mode_value == 'custom' and (not tls_cert_value or not tls_key_value):
        status_message.text = 'Custom TLS mode requires both cert and key paths'
        return

      previous_default_port = default_ui_port_for_tls_mode(previous_tls_mode)
      new_default_port = default_ui_port_for_tls_mode(tls_mode_value)
      if tls_mode_value != previous_tls_mode and ui_port_value == previous_default_port:
        ui_port_value = new_default_port
        ui_port.value = ui_port_value

      if ui_port_value != config.relay.ui_port:
        config.relay.set_ui_port(ui_port_value)
        need_save = True

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

    def generate_self_signed_cert():
      hostname = _sanitize_hostname(str(tls_hostname.value or '').strip())
      if not hostname:
        hostname = tls_hostname_default
      try:
        cert_path, key_path = security_utils.default_self_signed_cert_paths()
        cert_file, key_file = security_utils.ensure_self_signed_cert(
          cert_path, key_path, hostname, overwrite=True
        )
      except Exception:
        status_message.text = 'Failed to generate self-signed certificate'
        return

      tls_hostname.value = hostname
      current_mode = normalize_tls_mode(str(tls_mode.value or 'off'))
      tls_mode.value = 'self-signed'
      current_port = safe_int(ui_port.value, default_ui_port_for_tls_mode(current_mode), 1, 65535)
      if current_port == default_ui_port_for_tls_mode(current_mode):
        ui_port.value = default_ui_port_for_tls_mode('self-signed')
      status_message.text = (
        f'Generated self-signed certificate for {hostname} '
        f'({cert_file}, {key_file}). Click Save and restart chaosface.'
      )

    with ui.row().classes('gap-2'):
      ui.button('Load generated tokens', on_click=load_generated_tokens)
      ui.button('Generate self-signed cert', on_click=generate_self_signed_cert)
      ui.button('Save', on_click=save_connection)
      ui.button('Clear bot diagnostics', on_click=clear_bot_diagnostics)

    refresh_bot_diagnostics()
    ui.timer(1.0, refresh_bot_diagnostics)
