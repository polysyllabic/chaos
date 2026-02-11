# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Streamer tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import clamp01


def build_streamer_tab(*, shutdown_runtime: Callable[[], None]) -> None:
  with ui.card().classes('w-full'):
    ui.label('Streamer Interface').classes('text-h6')
    with ui.row().classes('items-center gap-6'):
      connection_label = ui.label('')
      paused_label = ui.label('')

    vote_timer = ui.linear_progress(value=0.0).classes('w-full')
    vote_label = ui.label('Vote Timer: 0%').classes('text-sm')
    mods_column = ui.column().classes('w-full gap-1')

    def refresh_streamer():
      connected = bool(config.relay.connected)
      connected_bright = bool(config.relay.connected_bright)
      paused = bool(config.relay.paused)
      paused_bright = bool(config.relay.paused_bright)
      vote_progress = clamp01(config.relay.vote_time)

      connection_label.text = 'Connected to Twitch' if connected else 'Disconnected from Twitch'
      paused_label.text = 'Chaos Paused' if paused else 'Chaos Running'
      connection_color = '#2e7d32' if connected else ('#d32f2f' if connected_bright else '#8b0000')
      paused_color = '#f57c00' if paused and paused_bright else ('#fbc02d' if paused else '#2e7d32')
      connection_label.style(f'font-weight:700; color:{connection_color}')
      paused_label.style(f'font-weight:700; color:{paused_color}')

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

    def stop_button_clicked():
      shutdown_runtime()
      ui.notify('Stopping loop...', color='warning')

    ui.button('Quit', on_click=stop_button_clicked).props('color=negative')
    ui.timer(0.25, refresh_streamer)
