# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Source-configuration tab UI for OBS overlay styling."""

from __future__ import annotations

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import safe_int


def _add_color_picker(input_field, initial_value: str):
  """Attach a color picker if this NiceGUI build provides one."""
  if not hasattr(ui, 'color_input'):
    ui.label('Color picker unavailable in this build; use CSS color text input.').classes('text-xs')
    return None
  try:
    picker = ui.color_input('Picker', value=initial_value).classes('w-40')
  except Exception:
    ui.label('Color picker unavailable in this build; use CSS color text input.').classes('text-xs')
    return None

  def _picker_to_input(event):
    input_field.value = str(getattr(event, 'value', picker.value) or '')

  def _input_to_picker(_event):
    value = str(input_field.value or '').strip()
    if not value:
      return
    try:
      picker.value = value
    except Exception:
      # Ignore invalid CSS values for the picker widget; the text input remains authoritative.
      pass

  picker.on('update:model-value', _picker_to_input)
  input_field.on('change', _input_to_picker)
  return picker


def build_source_configuration_tab() -> None:
  with ui.card().classes('w-full'):
    ui.label('Source Configuration').classes('text-h6')
    ui.label(
      'Configure OBS source spacing and colors. Color fields accept CSS names, hex values, or rgb()/rgba().'
    ).classes('text-caption')

    status_label = ui.label('').classes('text-sm')

    ui.separator()
    ui.label('Current Votes').classes('text-subtitle1')
    with ui.column().classes('w-full gap-2'):
      current_votes_gap = ui.number(
        'Name/bar spacing (px)',
        value=safe_int(config.relay.overlay_current_votes_gap, 10, 0, 120),
        min=0,
        max=120,
        step=1,
      ).classes('w-56')
      with ui.row().classes('items-end gap-3'):
        current_votes_text_color = ui.input(
          'Text color',
          value=str(config.relay.overlay_current_votes_text_color or '#ffffff'),
        ).classes('w-96')
        _add_color_picker(current_votes_text_color, str(current_votes_text_color.value or '#ffffff'))
      with ui.row().classes('items-end gap-3'):
        current_votes_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_current_votes_bar_color or 'rgba(245, 245, 245, 0.8)'),
        ).classes('w-96')
        _add_color_picker(current_votes_bar_color, str(current_votes_bar_color.value or 'rgba(245, 245, 245, 0.8)'))

    ui.separator()
    ui.label('Active Mods').classes('text-subtitle1')
    with ui.column().classes('w-full gap-2'):
      active_mods_gap = ui.number(
        'Name/bar spacing (px)',
        value=safe_int(config.relay.overlay_active_mods_gap, 10, 0, 120),
        min=0,
        max=120,
        step=1,
      ).classes('w-56')
      with ui.row().classes('items-end gap-3'):
        active_mods_text_color = ui.input(
          'Text color',
          value=str(config.relay.overlay_active_mods_text_color or '#ffffff'),
        ).classes('w-96')
        _add_color_picker(active_mods_text_color, str(active_mods_text_color.value or '#ffffff'))
      with ui.row().classes('items-end gap-3'):
        active_mods_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_active_mods_bar_color or 'rgba(245, 245, 245, 0.75)'),
        ).classes('w-96')
        _add_color_picker(active_mods_bar_color, str(active_mods_bar_color.value or 'rgba(245, 245, 245, 0.75)'))

    ui.separator()
    ui.label('Vote Timer').classes('text-subtitle1')
    with ui.column().classes('w-full gap-2'):
      with ui.row().classes('items-end gap-3'):
        vote_timer_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_vote_timer_bar_color or 'rgba(240, 240, 240, 0.85)'),
        ).classes('w-96')
        _add_color_picker(vote_timer_bar_color, str(vote_timer_bar_color.value or 'rgba(240, 240, 240, 0.85)'))

    def _set_if_changed(current, value, setter):
      if current != value:
        setter(value)
        return True
      return False

    def save_settings():
      need_save = False

      need_save |= _set_if_changed(
        int(config.relay.overlay_current_votes_gap),
        safe_int(current_votes_gap.value, int(config.relay.overlay_current_votes_gap), 0, 120),
        config.relay.set_overlay_current_votes_gap,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_current_votes_text_color),
        str(current_votes_text_color.value or '').strip(),
        config.relay.set_overlay_current_votes_text_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_current_votes_bar_color),
        str(current_votes_bar_color.value or '').strip(),
        config.relay.set_overlay_current_votes_bar_color,
      )
      need_save |= _set_if_changed(
        int(config.relay.overlay_active_mods_gap),
        safe_int(active_mods_gap.value, int(config.relay.overlay_active_mods_gap), 0, 120),
        config.relay.set_overlay_active_mods_gap,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_active_mods_text_color),
        str(active_mods_text_color.value or '').strip(),
        config.relay.set_overlay_active_mods_text_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_active_mods_bar_color),
        str(active_mods_bar_color.value or '').strip(),
        config.relay.set_overlay_active_mods_bar_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_vote_timer_bar_color),
        str(vote_timer_bar_color.value or '').strip(),
        config.relay.set_overlay_vote_timer_bar_color,
      )

      if need_save:
        config.relay.set_need_save(True)
        status_label.text = 'Source settings updated'
      else:
        status_label.text = 'No source settings changed'

    def restore_settings():
      current_votes_gap.value = int(config.relay.overlay_current_votes_gap)
      current_votes_text_color.value = str(config.relay.overlay_current_votes_text_color or '#ffffff')
      current_votes_bar_color.value = str(config.relay.overlay_current_votes_bar_color or 'rgba(245, 245, 245, 0.8)')
      active_mods_gap.value = int(config.relay.overlay_active_mods_gap)
      active_mods_text_color.value = str(config.relay.overlay_active_mods_text_color or '#ffffff')
      active_mods_bar_color.value = str(config.relay.overlay_active_mods_bar_color or 'rgba(245, 245, 245, 0.75)')
      vote_timer_bar_color.value = str(config.relay.overlay_vote_timer_bar_color or 'rgba(240, 240, 240, 0.85)')
      status_label.text = 'Restored saved source settings'

    with ui.row().classes('gap-2'):
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
