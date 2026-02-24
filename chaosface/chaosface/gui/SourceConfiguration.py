# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Source-configuration tab UI for OBS overlay styling."""

from __future__ import annotations

import json
import re

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import safe_int


_SOURCE_CONFIG_STYLE = """
<style>
  .source-config .q-field__label {
    font-size: 14px !important;
    line-height: 1.2 !important;
    white-space: normal !important;
    overflow: visible !important;
    text-overflow: clip !important;
    max-width: none !important;
  }
</style>
"""


def _sanitize_css_color(value: str, fallback: str = 'transparent') -> str:
  color = str(value or '').strip()
  if not color:
    return fallback
  # Avoid injecting extra style declarations while still allowing CSS color syntax.
  return color.replace(';', '').replace('{', '').replace('}', '')


def _swatch_style(color_value: str) -> str:
  safe_color = _sanitize_css_color(color_value, 'transparent')
  return (
    'width:56px; min-width:56px; height:56px; '
    f'background:{safe_color}; border:1px solid #000; border-radius:4px;'
  )


def _add_color_swatch(input_field):
  swatch = ui.element('div').classes('self-end')

  def refresh(color_value=None):
    color = str(input_field.value if color_value is None else color_value)
    swatch.style(_swatch_style(color))

  def _on_input(event):
    refresh(getattr(event, 'value', input_field.value))

  input_field.on('update:model-value', _on_input)
  input_field.on('input', _on_input)
  input_field.on('change', _on_input)
  refresh()
  return refresh


def _coerce_hex_for_native_picker(value: str, fallback: str = '#FFFFFF') -> str:
  """
  Convert user color text to a form accepted by native <input type=color> (#RRGGBB).
  Alpha channels are dropped for the picker preview/control.
  """
  text = str(value or '').strip()
  if re.fullmatch(r'#[0-9a-fA-F]{3}', text):
    return '#' + ''.join(ch * 2 for ch in text[1:])
  if re.fullmatch(r'#[0-9a-fA-F]{6}', text):
    return text.upper()
  if re.fullmatch(r'#[0-9a-fA-F]{8}', text):
    return ('#' + text[1:7]).upper()
  return fallback


def _add_color_picker_icon(input_field, initial_value: str, on_apply=None):
  """Attach a palette icon that opens a picker with explicit OK/Cancel apply."""
  if not hasattr(ui, 'dialog') or not hasattr(ui, 'input'):
    return None

  try:
    with ui.dialog() as dialog, ui.card().classes('w-auto'):
      ui.label('Pick color').classes('text-subtitle2')
      picker = ui.input(
        'Color',
        value=_coerce_hex_for_native_picker(str(initial_value or '').strip()),
      ).props('type=color stack-label').classes('w-48')

      preview = ui.element('div').style(_swatch_style(str(picker.value or '#FFFFFF')))

      def _refresh_preview(event):
        preview.style(_swatch_style(str(getattr(event, 'value', picker.value) or '#FFFFFF')))

      picker.on('update:model-value', _refresh_preview)
      picker.on('change', _refresh_preview)

      def _apply_color():
        value = str(getattr(picker, 'value', '') or '').strip()
        if value:
          input_field.value = value
          if callable(on_apply):
            on_apply(value)
        dialog.close()

      with ui.row().classes('w-full justify-end gap-2'):
        ui.button('Cancel', on_click=dialog.close).props('flat')
        ui.button('OK', on_click=_apply_color).props('unelevated')
  except Exception:
    return None

  def _open_picker():
    picker.value = _coerce_hex_for_native_picker(str(input_field.value or '').strip(), '#FFFFFF')
    preview.style(_swatch_style(str(picker.value or '#FFFFFF')))
    dialog.open()

  return ui.button(icon='palette', on_click=_open_picker).props('flat round dense').tooltip('Pick color')


async def _normalize_color_to_hex(value: str, fallback: str) -> str:
  """Normalize any valid CSS color (name/rgb/rgba/hex) to #RRGGBB or #RRGGBBAA via browser parsing."""
  raw = str(value or '').strip()
  if not raw:
    return fallback
  script = f"""(() => {{
    const value = {json.dumps(raw)};
    const probe = document.createElement('span');
    probe.style.color = '';
    probe.style.color = value;
    if (!probe.style.color) {{
      return '';
    }}
    document.body.appendChild(probe);
    const resolved = getComputedStyle(probe).color || '';
    probe.remove();
    const match = resolved.match(/^rgba?\\(([^)]+)\\)$/i);
    if (!match) {{
      return '';
    }}
    const parts = match[1].split(',').map(p => p.trim());
    if (parts.length < 3) {{
      return '';
    }}
    const clamp = (n) => Math.max(0, Math.min(255, Math.round(Number(n) || 0)));
    const toHex = (n) => clamp(n).toString(16).toUpperCase().padStart(2, '0');
    const r = clamp(parts[0]);
    const g = clamp(parts[1]);
    const b = clamp(parts[2]);
    let a = 255;
    if (parts.length >= 4) {{
      const alpha = Math.max(0, Math.min(1, Number(parts[3]) || 0));
      a = Math.round(alpha * 255);
    }}
    if (a < 255) {{
      return `#${{toHex(r)}}${{toHex(g)}}${{toHex(b)}}${{toHex(a)}}`;
    }}
    return `#${{toHex(r)}}${{toHex(g)}}${{toHex(b)}}`;
  }})()"""
  try:
    normalized = await ui.run_javascript(script, timeout=2.0)
  except Exception:
    normalized = ''
  clean = str(normalized or '').strip()
  return clean if clean else fallback


def build_source_configuration_tab() -> None:
  ui.add_head_html(_SOURCE_CONFIG_STYLE)
  with ui.card().classes('w-full source-config'):
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
      ).classes('w-96').props('stack-label')
      with ui.row().classes('items-end gap-3'):
        current_votes_text_color = ui.input(
          'Text color',
          value=str(config.relay.overlay_current_votes_text_color or '#ffffff'),
        ).classes('w-96').props('stack-label')
        refresh_current_votes_text_swatch = _add_color_swatch(current_votes_text_color)
        _add_color_picker_icon(
          current_votes_text_color,
          str(current_votes_text_color.value or '#ffffff'),
          on_apply=refresh_current_votes_text_swatch,
        )
      with ui.row().classes('items-end gap-3'):
        current_votes_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_current_votes_bar_color or 'rgba(245, 245, 245, 0.8)'),
        ).classes('w-96').props('stack-label')
        refresh_current_votes_bar_swatch = _add_color_swatch(current_votes_bar_color)
        _add_color_picker_icon(
          current_votes_bar_color,
          str(current_votes_bar_color.value or 'rgba(245, 245, 245, 0.8)'),
          on_apply=refresh_current_votes_bar_swatch,
        )

    ui.separator()
    ui.label('Active Mods').classes('text-subtitle1')
    with ui.column().classes('w-full gap-2'):
      active_mods_gap = ui.number(
        'Name/bar spacing (px)',
        value=safe_int(config.relay.overlay_active_mods_gap, 10, 0, 120),
        min=0,
        max=120,
        step=1,
      ).classes('w-96').props('stack-label')
      with ui.row().classes('items-end gap-3'):
        active_mods_text_color = ui.input(
          'Text color',
          value=str(config.relay.overlay_active_mods_text_color or '#ffffff'),
        ).classes('w-96').props('stack-label')
        refresh_active_mods_text_swatch = _add_color_swatch(active_mods_text_color)
        _add_color_picker_icon(
          active_mods_text_color,
          str(active_mods_text_color.value or '#ffffff'),
          on_apply=refresh_active_mods_text_swatch,
        )
      with ui.row().classes('items-end gap-3'):
        active_mods_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_active_mods_bar_color or 'rgba(245, 245, 245, 0.75)'),
        ).classes('w-96').props('stack-label')
        refresh_active_mods_bar_swatch = _add_color_swatch(active_mods_bar_color)
        _add_color_picker_icon(
          active_mods_bar_color,
          str(active_mods_bar_color.value or 'rgba(245, 245, 245, 0.75)'),
          on_apply=refresh_active_mods_bar_swatch,
        )

    ui.separator()
    ui.label('Vote Timer').classes('text-subtitle1')
    with ui.column().classes('w-full gap-2'):
      with ui.row().classes('items-end gap-3'):
        vote_timer_bar_color = ui.input(
          'Bar color',
          value=str(config.relay.overlay_vote_timer_bar_color or 'rgba(240, 240, 240, 0.85)'),
        ).classes('w-96').props('stack-label')
        refresh_vote_timer_bar_swatch = _add_color_swatch(vote_timer_bar_color)
        _add_color_picker_icon(
          vote_timer_bar_color,
          str(vote_timer_bar_color.value or 'rgba(240, 240, 240, 0.85)'),
          on_apply=refresh_vote_timer_bar_swatch,
        )

    def _set_if_changed(current, value, setter):
      if current != value:
        setter(value)
        return True
      return False

    async def save_settings():
      need_save = False

      current_votes_text_hex = await _normalize_color_to_hex(
        str(current_votes_text_color.value or ''),
        '#FFFFFF',
      )
      current_votes_bar_hex = await _normalize_color_to_hex(
        str(current_votes_bar_color.value or ''),
        '#F5F5F5CC',
      )
      active_mods_text_hex = await _normalize_color_to_hex(
        str(active_mods_text_color.value or ''),
        '#FFFFFF',
      )
      active_mods_bar_hex = await _normalize_color_to_hex(
        str(active_mods_bar_color.value or ''),
        '#F5F5F5BF',
      )
      vote_timer_bar_hex = await _normalize_color_to_hex(
        str(vote_timer_bar_color.value or ''),
        '#F0F0F0D9',
      )

      current_votes_text_color.value = current_votes_text_hex
      current_votes_bar_color.value = current_votes_bar_hex
      active_mods_text_color.value = active_mods_text_hex
      active_mods_bar_color.value = active_mods_bar_hex
      vote_timer_bar_color.value = vote_timer_bar_hex
      refresh_current_votes_text_swatch()
      refresh_current_votes_bar_swatch()
      refresh_active_mods_text_swatch()
      refresh_active_mods_bar_swatch()
      refresh_vote_timer_bar_swatch()

      need_save |= _set_if_changed(
        int(config.relay.overlay_current_votes_gap),
        safe_int(current_votes_gap.value, int(config.relay.overlay_current_votes_gap), 0, 120),
        config.relay.set_overlay_current_votes_gap,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_current_votes_text_color),
        current_votes_text_hex,
        config.relay.set_overlay_current_votes_text_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_current_votes_bar_color),
        current_votes_bar_hex,
        config.relay.set_overlay_current_votes_bar_color,
      )
      need_save |= _set_if_changed(
        int(config.relay.overlay_active_mods_gap),
        safe_int(active_mods_gap.value, int(config.relay.overlay_active_mods_gap), 0, 120),
        config.relay.set_overlay_active_mods_gap,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_active_mods_text_color),
        active_mods_text_hex,
        config.relay.set_overlay_active_mods_text_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_active_mods_bar_color),
        active_mods_bar_hex,
        config.relay.set_overlay_active_mods_bar_color,
      )
      need_save |= _set_if_changed(
        str(config.relay.overlay_vote_timer_bar_color),
        vote_timer_bar_hex,
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
      refresh_current_votes_text_swatch()
      refresh_current_votes_bar_swatch()
      refresh_active_mods_text_swatch()
      refresh_active_mods_bar_swatch()
      refresh_vote_timer_bar_swatch()
      status_label.text = 'Restored saved source settings'

    with ui.row().classes('gap-2'):
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
