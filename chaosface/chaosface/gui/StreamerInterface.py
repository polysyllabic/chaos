# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Streamer tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import clamp01, safe_float


def build_streamer_tab(*, shutdown_runtime: Callable[[], None]) -> Callable[[], None]:
  engine_status_labels = {
    'not_connected': 'Engine: Not Connected',
    'timeout': 'Engine: Connection Timeout',
    'waiting_for_game': 'Engine: Waiting for Game Selection',
    'bad_config_file': 'Engine: Bad Config File',
    'paused': 'Engine: Paused',
    'running': 'Engine: Running',
    'unknown': 'Engine: Unknown State',
  }

  engine_status_box_styles = {
    'not_connected': ('#4a1111', '#8b0000', '#ffe9e9'),
    'timeout': ('#6a1b1b', '#d32f2f', '#fff1f1'),
    'waiting_for_game': ('#704107', '#f57c00', '#fff2dc'),
    'bad_config_file': ('#6a1b1b', '#c62828', '#fff1f1'),
    'paused': ('#fbc02d', '#9a6e00', '#111111'),
    'running': ('#1f5f28', '#2e7d32', '#e8f5e9'),
    'unknown': ('#4a352f', '#6d4c41', '#f7ece7'),
  }

  with ui.card().classes('w-full'):
    ui.label('Streamer Interface').classes('text-h6')
    with ui.row().classes('w-full items-center justify-between'):
      connection_label = ui.label('')
      show_chatbot_diagnostics = ui.checkbox('Show chatbot diagnostics.', value=False)
    engine_status_notice = ui.label('').classes('w-full')
    engine_status_notice.style(
      'display:block; padding:14px 16px; border-radius:10px; border:2px solid #6d4c41; '
      'font-size:2.6rem; line-height:1.1; font-weight:800; text-align:center; '
      'background:#4a352f; color:#f7ece7;'
    )

    vote_timer = ui.linear_progress(value=0.0).classes('w-full')
    vote_label = ui.label('Vote Timer: 0s remaining').classes('text-sm')
    mods_column = ui.column().classes('w-full gap-1')
    mod_rows: list[dict] = []
    with ui.element('div').classes('h-8'):
      pass
    with ui.column().classes('w-full gap-2') as diagnostics_column:
      ui.label('Bot Status').classes('text-subtitle1')
      status_box = ui.textarea(
        label='',
        value='No bot status yet.',
      ).props('readonly').classes('w-full h-48 streamer-bot-status-box')
      ui.button('Clear Bot Status', on_click=lambda: clear_bot_status())
    diagnostics_column.visible = False

    def scroll_status_to_latest():
      try:
        ui.run_javascript(
          """
          (() => {
            const el = document.querySelector('.streamer-bot-status-box textarea');
            if (el) {
              el.scrollTop = el.scrollHeight;
            }
          })();
          """,
          timeout=0.0,
        )
      except Exception:
        # Ignore transient client disconnect races while keeping the refresh loop alive.
        pass

    def refresh_bot_status():
      if status_box.is_deleted:
        return
      messages = config.relay.get_bot_status()
      text = '\n'.join(str(message) for message in messages) if messages else 'No bot status yet.'
      if str(status_box.value or '') != text:
        status_box.value = text
        scroll_status_to_latest()

    def clear_bot_status():
      config.relay.clear_bot_status()
      refresh_bot_status()

    def set_diagnostics_visibility(visible: bool):
      diagnostics_column.visible = bool(visible)

    def refresh_streamer():
      if connection_label.is_deleted or engine_status_notice.is_deleted:
        return
      set_diagnostics_visibility(bool(show_chatbot_diagnostics.value))
      connected = bool(config.relay.connected)
      connected_bright = bool(config.relay.connected_bright)
      engine_status = str(config.relay.engine_status or 'unknown').strip().lower()
      paused_bright = bool(config.relay.paused_bright)
      vote_progress = clamp01(config.relay.vote_time)
      configured_vote_seconds = safe_float(
        config.relay.get_attribute('vote_time'),
        config.relay.time_per_vote(),
        1.0,
      )
      vote_seconds_remaining = int(round(max(0.0, (1.0 - vote_progress) * configured_vote_seconds)))
      if not bool(config.relay.vote_open):
        vote_seconds_remaining = 0

      connection_label.text = 'Connected to Twitch' if connected else 'Disconnected from Twitch'
      connection_color = '#2e7d32' if connected else ('#d32f2f' if connected_bright else '#8b0000')
      connection_label.style(f'font-weight:700; color:{connection_color}')
      engine_status_notice.text = engine_status_labels.get(engine_status, engine_status_labels['unknown'])

      status_bg, status_border, status_text = engine_status_box_styles.get(
        engine_status,
        engine_status_box_styles['unknown'],
      )
      if engine_status == 'paused' and paused_bright:
        status_bg = '#f57c00'
        status_border = '#a55a00'
        status_text = '#111111'
      engine_status_notice.style(
        'display:block; padding:14px 16px; border-radius:10px; border:2px solid '
        f'{status_border}; font-size:2.6rem; line-height:1.1; font-weight:800; '
        f'text-align:center; background:{status_bg}; color:{status_text};'
      )

      vote_timer.value = vote_progress
      vote_label.text = f'Vote Timer: {vote_seconds_remaining}s remaining'

      active_mods = list(config.relay.active_mods or [])
      mod_times = list(config.relay.mod_times or [])
      count = max(int(config.relay.num_active_mods), len(active_mods), len(mod_times))

      while len(mod_rows) < count:
        with mods_column:
          row = ui.row().classes('w-full items-center gap-4')
          with row:
            label = ui.label('').classes('min-w-72')
            progress = ui.linear_progress(value=0.0).classes('flex-1')
            remaining = ui.label('0s').classes('min-w-16 text-right')
        mod_rows.append({'row': row, 'label': label, 'progress': progress, 'remaining': remaining})

      while len(mod_rows) > count:
        item = mod_rows.pop()
        item['row'].delete()

      for idx in range(count):
        mod_name = active_mods[idx] if idx < len(active_mods) else ''
        time_remaining = clamp01(mod_times[idx] if idx < len(mod_times) else 0.0)
        seconds_remaining = int(round(max(0.0, time_remaining * max(1.0, float(config.relay.time_per_modifier)))))
        item = mod_rows[idx]
        if item['label'].text != mod_name:
          item['label'].text = mod_name
        if item['progress'].value != time_remaining:
          item['progress'].value = time_remaining
        remaining_text = f'{seconds_remaining}s'
        if item['remaining'].text != remaining_text:
          item['remaining'].text = remaining_text
      refresh_bot_status()

    def stop_button_clicked():
      shutdown_runtime()
      ui.notify('Stopping loop...', color='warning')

    with ui.row().classes('gap-2'):
      ui.button('Quit', on_click=stop_button_clicked).props('color=negative')
    refresh_bot_status()
    return refresh_streamer
