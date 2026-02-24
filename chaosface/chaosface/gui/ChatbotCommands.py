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

  with ui.card().classes('w-full chatbot-commands'):
    ui.label('CHATBOT COMMANDS').classes('text-h6')
    ui.label(
      'Set one alias per command. Aliases must be single words (no spaces).'
    ).classes('text-caption')

    status_label = ui.label('').classes('text-sm')
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
      need_save = False

      if current_aliases != new_aliases:
        config.relay.set_chatbot_command_aliases(new_aliases)
        need_save = True
      if current_alias_only != new_alias_only:
        config.relay.set_chatbot_alias_only(new_alias_only)
        need_save = True

      if need_save:
        config.relay.set_need_save(True)
        status_label.text = 'Chatbot command settings updated'
      else:
        status_label.text = 'No chatbot command settings changed'

    def restore_settings():
      restored_aliases = sanitize_alias_map(config.relay.chatbot_command_aliases)
      restored_alias_only = sanitize_alias_only_map(config.relay.chatbot_alias_only)
      for spec in CHATBOT_COMMANDS:
        widgets = rows[spec.key]
        widgets['alias'].value = str(restored_aliases.get(spec.key, '') or '')
        widgets['alias_only'].value = bool(restored_alias_only.get(spec.key, False))
      status_label.text = 'Restored saved chatbot command settings'

    with ui.row().classes('gap-2'):
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
