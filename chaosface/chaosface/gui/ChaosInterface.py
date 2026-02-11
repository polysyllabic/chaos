# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Main NiceGUI page composition for Twitch Controls Chaos."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

from .ConnectionSetup import build_connection_tab
from .GameSettings import build_game_settings_tab
from .StreamerInterface import build_streamer_tab


def build_chaos_interface(*, ensure_runtime_started: Callable[[], None], shutdown_runtime: Callable[[], None]) -> None:
  ensure_runtime_started()
  ui.label('Twitch Controls Chaos').classes('text-h4')
  tabs = ui.tabs().classes('w-full')
  with tabs:
    streamer_tab = ui.tab('Streamer Interface')
    game_settings_tab = ui.tab('Game Settings')
    connection_tab = ui.tab('Connection Setup')

  with ui.tab_panels(tabs, value=streamer_tab).classes('w-full'):
    with ui.tab_panel(streamer_tab):
      build_streamer_tab(shutdown_runtime=shutdown_runtime)
    with ui.tab_panel(game_settings_tab):
      build_game_settings_tab()
    with ui.tab_panel(connection_tab):
      build_connection_tab()
