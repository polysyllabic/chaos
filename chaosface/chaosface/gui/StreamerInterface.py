# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Streamer tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import clamp01


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

  engine_status_colors = {
    'not_connected': '#8b0000',
    'timeout': '#d32f2f',
    'waiting_for_game': '#f57c00',
    'bad_config_file': '#c62828',
    'paused': '#fbc02d',
    'running': '#2e7d32',
    'unknown': '#6d4c41',
  }

  with ui.card().classes('w-full'):
    ui.label('Streamer Interface').classes('text-h6')
    with ui.row().classes('items-center gap-6'):
      connection_label = ui.label('')
      engine_status_label = ui.label('')

    vote_timer = ui.linear_progress(value=0.0).classes('w-full')
    vote_label = ui.label('Vote Timer: 0%').classes('text-sm')
    mods_column = ui.column().classes('w-full gap-1')
    with ui.element('div').classes('h-8'):
      pass
    ui.label('Bot Status').classes('text-subtitle1')
    status_box = ui.textarea(
      label='',
      value='No bot status yet.',
    ).props('readonly').classes('w-full h-48 streamer-bot-status-box')

    def scroll_status_to_latest():
      ui.run_javascript(
        """
        (() => {
          const el = document.querySelector('.streamer-bot-status-box textarea');
          if (el) {
            el.scrollTop = el.scrollHeight;
          }
        })();
        """
      )

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

    def refresh_streamer():
      if connection_label.is_deleted or engine_status_label.is_deleted:
        return
      connected = bool(config.relay.connected)
      connected_bright = bool(config.relay.connected_bright)
      engine_status = str(config.relay.engine_status or 'unknown').strip().lower()
      paused_bright = bool(config.relay.paused_bright)
      vote_progress = clamp01(config.relay.vote_time)

      connection_label.text = 'Connected to Twitch' if connected else 'Disconnected from Twitch'
      engine_status_label.text = engine_status_labels.get(engine_status, engine_status_labels['unknown'])
      connection_color = '#2e7d32' if connected else ('#d32f2f' if connected_bright else '#8b0000')
      status_color = engine_status_colors.get(engine_status, engine_status_colors['unknown'])
      if engine_status == 'paused' and paused_bright:
        status_color = '#f57c00'
      connection_label.style(f'font-weight:700; color:{connection_color}')
      engine_status_label.style(f'font-weight:700; color:{status_color}')

      vote_timer.value = vote_progress
      vote_label.text = f'Vote Timer: {int(vote_progress * 100)}%'

      mods_column.clear()
      active_mods = list(config.relay.active_mods or [])
      mod_times = list(config.relay.mod_times or [])
      count = max(int(config.relay.num_active_mods), len(active_mods), len(mod_times))
      for idx in range(count):
        mod_name = active_mods[idx] if idx < len(active_mods) else ''
        time_remaining = clamp01(mod_times[idx] if idx < len(mod_times) else 0.0)
        with mods_column:
          with ui.row().classes('w-full items-center gap-4'):
            ui.label(mod_name).classes('min-w-72')
            ui.linear_progress(value=time_remaining).classes('flex-1')
      refresh_bot_status()

    def stop_button_clicked():
      shutdown_runtime()
      ui.notify('Stopping loop...', color='warning')

    with ui.row().classes('gap-2'):
      ui.button('Clear Bot Status', on_click=clear_bot_status)
      ui.button('Quit', on_click=stop_button_clicked).props('color=negative')
    refresh_bot_status()
    return refresh_streamer
