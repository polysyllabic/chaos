# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Chatbot command alias configuration tab."""

from __future__ import annotations

from nicegui import ui

import chaosface.config.globals as config
from chaosface.chatbot.command_aliases import (
  CHATBOT_COMMANDS,
  normalize_alias,
  sanitize_alias_map,
  sanitize_alias_only_map,
)


_CHATBOT_COMMANDS_STYLE = """
<style>
  .chatbot-commands .q-field__label {
    font-size: 14px !important;
    line-height: 1.2 !important;
    white-space: normal !important;
    overflow: visible !important;
    text-overflow: clip !important;
    max-width: none !important;
  }
</style>
"""


def build_chatbot_commands_tab() -> None:
  ui.add_head_html(_CHATBOT_COMMANDS_STYLE)

  aliases = sanitize_alias_map(config.relay.chatbot_command_aliases)
  alias_only = sanitize_alias_only_map(config.relay.chatbot_alias_only)
  initial_dark_mode = bool(getattr(config.relay, 'ui_dark_mode', False))
  initial_streamer_text_scale = float(getattr(config.relay, 'ui_streamer_text_scale', 2.0) or 2.0)
  initial_streamer_text_scale = max(0.5, min(4.0, initial_streamer_text_scale))

  with ui.card().classes('w-full chatbot-commands'):
    with ui.row().classes('w-full items-start justify-between'):
      ui.label('INTERFACE SETTINGS').classes('text-h6')
      with ui.column().classes('items-end gap-1'):
        button_bar = ui.row().classes('gap-2')
        status_label = ui.label('').classes('text-sm text-right whitespace-normal')
        status_label.style('max-width: 48ch; min-height: 1.25rem;')

    ui.label('GUI Settings').classes('text-subtitle1')
    dark_mode = ui.dark_mode(value=initial_dark_mode)

    def _apply_dark_mode(enabled: bool):
      dark_mode.set_value(bool(enabled))

    dark_mode_toggle = ui.checkbox(
      'Dark Mode',
      value=initial_dark_mode,
      on_change=lambda event: _apply_dark_mode(bool(getattr(event, 'value', False))),
    )
    _apply_dark_mode(initial_dark_mode)
    streamer_text_scale = ui.number(
      'Streamer tab mod text size (x)',
      value=initial_streamer_text_scale,
      step=0.1,
      min=0.5,
      max=4.0,
    ).props('stack-label').classes('w-64')
    ui.label('Applies only to modifier name/time text on Streamer Interface (not OBS sources).').classes('text-caption')

    ui.separator()
    ui.label('Chatbot Commands').classes('text-subtitle1')
    ui.label(
      'Set one alias per command. Aliases must be single words (no spaces).'
    ).classes('text-caption')
    rows = {}

    with ui.column().classes('w-full gap-2'):
      for spec in CHATBOT_COMMANDS:
        with ui.row().classes('w-full items-end gap-3'):
          ui.label(spec.label).classes('w-56 text-body2')
          ui.input('Default command', value=spec.default_chat_syntax).props('readonly stack-label').classes('w-56')
          alias_input = ui.input(
            'Alias',
            value=str(aliases.get(spec.key, '') or ''),
          ).props('stack-label').classes('w-56')
          alias_toggle = ui.checkbox(
            'Use alias only',
            value=bool(alias_only.get(spec.key, False)),
          )
          rows[spec.key] = {
            'alias': alias_input,
            'alias_only': alias_toggle,
            'default': spec.default_chat_syntax,
          }

    def _collect_settings():
      new_aliases = {}
      new_alias_only = {}
      invalid_commands = []
      duplicate_aliases = set()
      seen_aliases = {}

      for spec in CHATBOT_COMMANDS:
        widgets = rows[spec.key]
        raw_alias = str(widgets['alias'].value or '').strip()
        alias = normalize_alias(raw_alias)
        if raw_alias and not alias:
          invalid_commands.append(widgets['default'])
        widgets['alias'].value = alias
        new_aliases[spec.key] = alias
        new_alias_only[spec.key] = bool(widgets['alias_only'].value)
        if not alias:
          continue
        prior = seen_aliases.get(alias)
        if prior is not None and prior != spec.key:
          duplicate_aliases.add(alias)
        else:
          seen_aliases[alias] = spec.key

      if invalid_commands:
        status_label.text = (
          'Invalid alias format for: ' + ', '.join(invalid_commands) + '. Use single-word aliases only.'
        )
        return None, None
      if duplicate_aliases:
        status_label.text = (
          'Duplicate aliases are not allowed: ' + ', '.join(sorted(duplicate_aliases))
        )
        return None, None

      return new_aliases, new_alias_only

    def save_settings():
      new_aliases, new_alias_only = _collect_settings()
      if new_aliases is None or new_alias_only is None:
        return

      current_aliases = sanitize_alias_map(config.relay.chatbot_command_aliases)
      current_alias_only = sanitize_alias_only_map(config.relay.chatbot_alias_only)
      current_dark_mode = bool(getattr(config.relay, 'ui_dark_mode', False))
      current_streamer_text_scale = float(getattr(config.relay, 'ui_streamer_text_scale', 2.0) or 2.0)
      current_streamer_text_scale = max(0.5, min(4.0, current_streamer_text_scale))
      new_dark_mode = bool(dark_mode_toggle.value)
      try:
        new_streamer_text_scale = float(streamer_text_scale.value)
      except (TypeError, ValueError):
        new_streamer_text_scale = current_streamer_text_scale
      new_streamer_text_scale = max(0.5, min(4.0, new_streamer_text_scale))
      streamer_text_scale.value = new_streamer_text_scale
      need_save = False

      if current_aliases != new_aliases:
        config.relay.set_chatbot_command_aliases(new_aliases)
        need_save = True
      if current_alias_only != new_alias_only:
        config.relay.set_chatbot_alias_only(new_alias_only)
        need_save = True
      if current_dark_mode != new_dark_mode:
        config.relay.set_ui_dark_mode(new_dark_mode)
        need_save = True
      if abs(current_streamer_text_scale - new_streamer_text_scale) > 1e-9:
        config.relay.set_ui_streamer_text_scale(new_streamer_text_scale)
        need_save = True

      if need_save:
        config.relay.set_need_save(True)
        status_label.text = 'Interface settings updated'
      else:
        status_label.text = 'No interface settings changed'

    def restore_settings():
      restored_aliases = sanitize_alias_map(config.relay.chatbot_command_aliases)
      restored_alias_only = sanitize_alias_only_map(config.relay.chatbot_alias_only)
      restored_dark_mode = bool(getattr(config.relay, 'ui_dark_mode', False))
      restored_streamer_text_scale = float(getattr(config.relay, 'ui_streamer_text_scale', 2.0) or 2.0)
      restored_streamer_text_scale = max(0.5, min(4.0, restored_streamer_text_scale))
      for spec in CHATBOT_COMMANDS:
        widgets = rows[spec.key]
        widgets['alias'].value = str(restored_aliases.get(spec.key, '') or '')
        widgets['alias_only'].value = bool(restored_alias_only.get(spec.key, False))
      dark_mode_toggle.value = restored_dark_mode
      _apply_dark_mode(restored_dark_mode)
      streamer_text_scale.value = restored_streamer_text_scale
      status_label.text = 'Restored saved interface settings'

    with button_bar:
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
