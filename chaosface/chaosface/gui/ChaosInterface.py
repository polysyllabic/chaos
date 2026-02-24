# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Main NiceGUI page composition for Twitch Controls Chaos."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

from .ConnectionSetup import build_connection_tab
from .GameSettings import build_game_settings_tab
from .SourceConfiguration import build_source_configuration_tab
from .StreamerInterface import build_streamer_tab
from chaosface.version import get_version


def build_chaos_interface(*, ensure_runtime_started: Callable[[], None], shutdown_runtime: Callable[[], None]) -> None:
  ensure_runtime_started()
  ui.label(f'Twitch Controls Chaos ({get_version()})').classes('text-h4')
  tabs = ui.tabs().classes('w-full')
  with tabs:
    streamer_tab = ui.tab('Streamer Interface')
    game_settings_tab = ui.tab('Game Settings')
    source_configuration_tab = ui.tab('SOURCE CONFIGURATION')
    connection_tab = ui.tab('Connection Setup')

  with ui.tab_panels(tabs, value=streamer_tab).classes('w-full'):
    with ui.tab_panel(streamer_tab):
      refresh_streamer = build_streamer_tab(shutdown_runtime=shutdown_runtime)
    with ui.tab_panel(game_settings_tab):
      refresh_game_settings = build_game_settings_tab()
    with ui.tab_panel(source_configuration_tab):
      build_source_configuration_tab()
    with ui.tab_panel(connection_tab):
      build_connection_tab()

  streamer_timer = ui.timer(0.5, refresh_streamer)
  game_settings_timer = ui.timer(1.0, refresh_game_settings)
  client = getattr(ui.context, 'client', None)
  if client is not None:
    on_disconnect = getattr(client, 'on_disconnect', None)
    if callable(on_disconnect):
      on_disconnect(lambda: setattr(streamer_timer, 'active', False))
      on_disconnect(lambda: setattr(game_settings_timer, 'active', False))
