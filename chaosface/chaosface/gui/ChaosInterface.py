# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Main NiceGUI page composition for Twitch Controls Chaos."""

from __future__ import annotations

from typing import Callable, Optional

from nicegui import ui

from .ChatbotCommands import build_chatbot_commands_tab
from .ConnectionSetup import build_connection_tab
from .GameSettings import build_game_settings_tab
from .SourceConfiguration import build_source_configuration_tab
from .StreamerInterface import build_streamer_tab
from chaosface.version import get_version


def build_chaos_interface(*, ensure_runtime_started: Callable[[], None], shutdown_runtime: Callable[[], None]) -> None:
  ensure_runtime_started()
  ui.label(f'Twitch Controls Chaos ({get_version()})').classes('text-h4')
  tabs = ui.tabs().props('align=left').classes('w-full')
  with tabs:
    streamer_tab = ui.tab('Streamer Interface')
    game_settings_tab = ui.tab('Game Settings')
    chatbot_commands_tab = ui.tab('INTERFACE SETTINGS')
    source_configuration_tab = ui.tab('SOURCE CONFIGURATION')
    connection_tab = ui.tab('Connection Setup')

  refresh_game_settings: Optional[Callable[[], None]] = None
  game_settings_timer: Optional[object] = None

  with ui.tab_panels(tabs, value=streamer_tab).classes('w-full'):
    with ui.tab_panel(streamer_tab):
      refresh_streamer = build_streamer_tab(shutdown_runtime=shutdown_runtime)
    with ui.tab_panel(game_settings_tab):
      game_settings_mount = ui.column().classes('w-full')
      with game_settings_mount:
        ui.label('Open this tab to load game settings...').classes('text-caption')
    with ui.tab_panel(chatbot_commands_tab):
      chatbot_commands_mount = ui.column().classes('w-full')
      with chatbot_commands_mount:
        ui.label('Open this tab to load interface settings...').classes('text-caption')
    with ui.tab_panel(source_configuration_tab):
      source_configuration_mount = ui.column().classes('w-full')
      with source_configuration_mount:
        ui.label('Open this tab to load source configuration...').classes('text-caption')
    with ui.tab_panel(connection_tab):
      connection_mount = ui.column().classes('w-full')
      with connection_mount:
        ui.label('Open this tab to load connection setup...').classes('text-caption')

  tab_loaded = {
    'game_settings': False,
    'chatbot_commands': False,
    'source_configuration': False,
    'connection': False,
  }

  def load_game_settings_tab() -> None:
    nonlocal refresh_game_settings, game_settings_timer
    if tab_loaded['game_settings']:
      return
    game_settings_mount.clear()
    with game_settings_mount:
      refresh_game_settings = build_game_settings_tab()
    game_settings_timer = ui.timer(1.0, refresh_game_settings)
    tab_loaded['game_settings'] = True

  def load_chatbot_commands_tab() -> None:
    if tab_loaded['chatbot_commands']:
      return
    chatbot_commands_mount.clear()
    with chatbot_commands_mount:
      build_chatbot_commands_tab()
    tab_loaded['chatbot_commands'] = True

  def load_source_configuration_tab() -> None:
    if tab_loaded['source_configuration']:
      return
    source_configuration_mount.clear()
    with source_configuration_mount:
      build_source_configuration_tab()
    tab_loaded['source_configuration'] = True

  def load_connection_tab() -> None:
    if tab_loaded['connection']:
      return
    connection_mount.clear()
    with connection_mount:
      build_connection_tab()
    tab_loaded['connection'] = True

  game_settings_tab.on('click', lambda _event: load_game_settings_tab())
  chatbot_commands_tab.on('click', lambda _event: load_chatbot_commands_tab())
  source_configuration_tab.on('click', lambda _event: load_source_configuration_tab())
  connection_tab.on('click', lambda _event: load_connection_tab())

  streamer_timer = ui.timer(0.5, refresh_streamer)

  def set_timers_active(active: bool) -> None:
    streamer_timer.active = active
    if game_settings_timer is not None:
      game_settings_timer.active = active

  client = getattr(ui.context, 'client', None)
  if client is not None:
    on_connect = getattr(client, 'on_connect', None)
    if callable(on_connect):
      on_connect(lambda: set_timers_active(True))
    on_delete = getattr(client, 'on_delete', None)
    if callable(on_delete):
      on_delete(lambda: set_timers_active(False))
