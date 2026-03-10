# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Game settings tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from typing import Callable

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import relay_config_float, safe_float, safe_int, sync_enabled_mods


def build_game_settings_tab() -> Callable[[], None]:
  with ui.card().classes('w-full'):
    def labeled_control(
      label_text: str,
      create_control,
      *,
      row_classes: str = 'w-full items-center gap-3',
      control_classes: str = 'w-64 max-w-full',
      label_style: str = 'width: 22rem; min-width: 22rem;',
    ):
      with ui.row().classes(row_classes):
        ui.label(label_text).classes('text-body1 whitespace-normal').style(label_style)
        control = create_control()
        if control_classes:
          control.classes(control_classes)
      return control

    with ui.row().classes('w-full items-start justify-between'):
      ui.label('Game Settings').classes('text-h6')
      with ui.column().classes('items-end gap-1'):
        button_bar = ui.row().classes('gap-2')
        status_label = ui.label('').classes('text-sm text-right whitespace-normal')
        status_label.style('max-width: 48ch; min-height: 1.25rem;')

    ui.label('Game Selection').classes('text-subtitle1')

    game_status = ui.label('').classes('text-xs whitespace-normal')
    game_status.style('max-width: 48ch; min-height: 1rem;')

    with ui.row().classes('w-full items-end gap-3'):
      selector_state = {'pending_user_selection': False}

      def _on_game_selector_change(_event):
        selector_state['pending_user_selection'] = True

      game_selector = labeled_control(
        'Available games',
        lambda: ui.select([], on_change=_on_game_selector_change),
        row_classes='items-center gap-3',
        control_classes='w-96',
      )

      def confirm_game_selection():
        selected = str(game_selector.value or '').strip()
        if not selected:
          status_label.text = 'Select a game before confirming'
          return
        selector_state['pending_user_selection'] = False
        config.relay.request_game_selection(selected)
        status_label.text = f'Requested game load: {selected}'

      ui.button('Confirm Game', on_click=confirm_game_selection)

    def refresh_game_selector():
      if game_selector.is_deleted:
        return
      options = list(config.relay.available_games)
      if game_selector.options != options:
        game_selector.options = options
        game_selector.update()

      selected = str(config.relay.selected_game or '').strip()
      if selected.upper() == 'NONE':
        selected = ''
      if selected not in options:
        selected = options[0] if options else None
      current = game_selector.value
      if current is not None and current not in options:
        current = None
        selector_state['pending_user_selection'] = False
      should_sync_from_relay = not selector_state['pending_user_selection']
      if should_sync_from_relay and game_selector.value != selected:
        game_selector.value = selected
      elif not should_sync_from_relay and current is None:
        game_selector.value = selected
        selector_state['pending_user_selection'] = False

      game_status.text = f'Current game: {config.relay.game_name or "NONE"}'

      current_game = str(config.relay.game_name or 'NONE')
      if mod_list_state['bound_game'] != current_game:
        mod_list_state['bound_game'] = current_game
        mod_list_state['pending_user_edit'] = False

      effective_mod_list = str(config.relay.mod_list_link or '')
      if not mod_list_state['pending_user_edit'] and mod_list_link.value != effective_mod_list:
        mod_list_link.value = effective_mod_list

    ui.separator()
    with ui.row().classes('w-full items-end gap-3'):
      mod_list_state = {
        'pending_user_edit': False,
        'bound_game': str(config.relay.game_name or 'NONE'),
      }

      def _on_mod_list_change(_event):
        mod_list_state['pending_user_edit'] = True

      mod_list_link = labeled_control(
        'Modifier List Link (used by !mods)',
        lambda: ui.input(value=str(config.relay.mod_list_link or ''), on_change=_on_mod_list_change),
        row_classes='items-center gap-3',
        control_classes='max-w-full',
      )
      mod_list_link.style('width: 48ch;')

      def reset_mod_list_to_default():
        mod_list_link.value = str(config.relay.default_mod_list_link or '')
        mod_list_state['pending_user_edit'] = True
        status_label.text = 'Mod-list link reset to engine default (click Save to persist)'

      ui.button('Default', on_click=reset_mod_list_to_default)

    ui.separator()
    chaoscmd_link = labeled_control(
      'Chatbot Commands Link (used by !chaoscmd)',
      lambda: ui.input(value=str(config.relay.chaoscmd_link or '')),
      row_classes='items-center gap-3',
      control_classes='max-w-full',
    )
    chaoscmd_link.style('width: 48ch;')

    refresh_game_selector()

    ui.separator()

    settings_row_classes = 'w-full items-center gap-3 no-wrap'
    settings_label_style = 'width: 13rem; min-width: 13rem;'

    with ui.row().classes('w-full gap-8'):
      with ui.column().classes('w-96'):
        num_active_mods = labeled_control(
          'Active modifiers',
          lambda: ui.number(value=int(config.relay.num_active_mods), min=1, max=10, step=1),
          row_classes=settings_row_classes,
          control_classes='w-20',
          label_style=settings_label_style,
        )
        time_per_modifier = labeled_control(
          'Time per modifier (sec)',
          lambda: ui.number(value=float(config.relay.time_per_modifier), min=1, max=172800, step=1),
          row_classes=settings_row_classes,
          control_classes='w-28',
          label_style=settings_label_style,
        )
        softmax_factor = labeled_control(
          'Repeat penalty',
          lambda: ui.number(value=int(config.relay.softmax_factor), min=1, max=100, step=1),
          row_classes=settings_row_classes,
          control_classes='w-20',
          label_style=settings_label_style,
        )
        vote_options = labeled_control(
          'Vote options',
          lambda: ui.number(value=int(config.relay.vote_options), min=2, max=5, step=1),
          row_classes=settings_row_classes,
          control_classes='w-20',
          label_style=settings_label_style,
        )
        voting_type = labeled_control(
          'Voting method',
          lambda: ui.select(['Proportional', 'Majority', 'Authoritarian'], value=config.relay.voting_type),
          row_classes=settings_row_classes,
          control_classes='w-40',
          label_style=settings_label_style,
        )
        voting_cycle = labeled_control(
          'Voting cycle',
          lambda: ui.select(
            ['Continuous', 'Interval', 'Random', 'Triggered', 'DISABLED'],
            value=config.relay.voting_cycle,
          ),
          row_classes=settings_row_classes,
          control_classes='w-40',
          label_style=settings_label_style,
        )
        vote_time = labeled_control(
          'Vote time (sec)',
          lambda: ui.number(
            value=relay_config_float('vote_time', 60.0, 1.0, 3600.0),
            min=1,
            max=3600,
            step=1,
          ),
          row_classes=settings_row_classes,
          control_classes='w-24',
          label_style=settings_label_style,
        )
        vote_delay = labeled_control(
          'Vote delay (sec)',
          lambda: ui.number(
            value=relay_config_float('vote_delay', 0.0, 0.0, 3600.0),
            min=0,
            max=3600,
            step=1,
          ),
          row_classes=settings_row_classes,
          control_classes='w-24',
          label_style=settings_label_style,
        )

      with ui.column().classes('w-96'):
        announce_candidates = ui.checkbox('Announce candidates in chat', value=bool(config.relay.announce_candidates))
        announce_winner = ui.checkbox('Announce winner in chat', value=bool(config.relay.announce_winner))
        announce_active = ui.checkbox('Announce active mods in chat', value=bool(config.relay.announce_active))
        startup_random_modifier = ui.checkbox(
          'Apply one random modifier on startup',
          value=bool(config.relay.startup_random_modifier),
        )
        bits_redemptions = ui.checkbox('Allow bits redemptions', value=bool(config.relay.bits_redemptions))
        points_redemptions = ui.checkbox('Allow points redemptions', value=bool(config.relay.points_redemptions))
        raffles = ui.checkbox('Conduct raffles', value=bool(config.relay.raffles))
        multiple_credits = ui.checkbox('Allow multiple credits per cheer', value=bool(config.relay.multiple_credits))
        redemption_cooldown = labeled_control(
          'Apply cooldown (sec)',
          lambda: ui.number(value=float(config.relay.redemption_cooldown), min=0, max=86400, step=1),
          row_classes=settings_row_classes,
          control_classes='w-28',
          label_style=settings_label_style,
        )
        bits_per_credit = labeled_control(
          'Bits per mod credit',
          lambda: ui.number(value=int(config.relay.bits_per_credit), min=1, max=100000, step=1),
          row_classes=settings_row_classes,
          control_classes='w-28',
          label_style=settings_label_style,
        )

    with ui.row().classes('w-full items-center gap-8 no-wrap'):
      raffle_time = labeled_control(
        'Default raffle time (sec)',
        lambda: ui.number(value=float(config.relay.raffle_time), min=30, max=3600, step=1),
        row_classes='items-center gap-2 no-wrap',
        control_classes='w-24',
        label_style=settings_label_style,
      )
      points_reward_title = labeled_control(
        'Points reward title',
        lambda: ui.input(value=str(config.relay.points_reward_title)),
        row_classes='items-center gap-2 no-wrap',
        control_classes='w-44',
        label_style=settings_label_style,
      )

    ui.separator()
    filter_state = {
      'enabled': False,
      'selected_groups': set(),
      'rendering_groups': False,
    }

    def _group_names(mod_data) -> set[str]:
      raw_groups = mod_data.get('groups', [])
      if isinstance(raw_groups, str):
        raw_groups = [raw_groups]
      groups = set()
      if isinstance(raw_groups, (list, tuple, set)):
        for group in raw_groups:
          group_name = str(group).strip()
          if group_name:
            groups.add(group_name)
      return groups

    def _normalized(name: str) -> str:
      return str(name).strip().lower()

    def _group_options() -> list[tuple[str, str]]:
      by_normalized: dict[str, str] = {}
      for mod_data in config.relay.modifier_data.values():
        for group_name in _group_names(mod_data):
          norm = _normalized(group_name)
          if norm and norm not in by_normalized:
            by_normalized[norm] = group_name
      return sorted(by_normalized.items(), key=lambda item: item[1].lower())

    def _matches_group_filter(mod_data) -> bool:
      if not filter_state['enabled']:
        return True
      selected = filter_state['selected_groups']
      if not selected:
        return False
      mod_groups = {_normalized(group_name) for group_name in _group_names(mod_data)}
      return bool(mod_groups.intersection(selected))

    with ui.row().classes('w-full items-start gap-8'):
      with ui.column().classes('w-96 max-w-full'):
        ui.label('Available Modifiers').classes('text-subtitle1')
        with ui.row().classes('gap-2'):
          ui.button('Enable All', on_click=lambda: set_all_mods(True))
          ui.button('Disable All', on_click=lambda: set_all_mods(False))
        mod_toggles = ui.column().classes('w-full gap-1 overflow-y-auto border rounded p-2')
        mod_toggles.style('height: 30rem;')

      with ui.column().classes('w-80 max-w-full gap-2'):
        ui.label('Modifier Groups').classes('text-subtitle1')
        group_filter_checkbox = ui.checkbox('Filter by group', value=False)
        group_toggles = ui.column().classes('w-full gap-1 overflow-y-auto border rounded p-2')
        group_toggles.style('height: 30rem;')

    def render_group_toggles():
      group_toggles.clear()
      options = _group_options()
      option_norms = {norm for norm, _label in options}
      filter_state['selected_groups'] = {
        name for name in filter_state['selected_groups'] if name in option_norms
      }

      filter_state['rendering_groups'] = True
      try:
        with group_toggles:
          if not options:
            ui.label('No modifier groups available').classes('text-xs')
            return
          for norm, label in options:
            checked = norm in filter_state['selected_groups']
            ui.checkbox(
              label,
              value=checked,
              on_change=lambda event, group_norm=norm: on_group_toggle(group_norm, bool(getattr(event, 'value', False))),
            )
      finally:
        filter_state['rendering_groups'] = False

    def render_mod_toggles():
      mod_toggles.clear()
      render_group_toggles()
      mods = sorted(
        config.relay.modifier_data.items(),
        key=lambda item: str(item[1].get('name', item[0])).lower(),
      )
      with mod_toggles:
        for key, mod_data in mods:
          if not _matches_group_filter(mod_data):
            continue
          def _on_toggle(event, mod_key=key):
            config.relay.enable_mod(mod_key, bool(event.value))
            sync_enabled_mods()
            config.relay.save_mod_info()

          ui.checkbox(mod_data.get('name', key), value=bool(mod_data.get('active', True)), on_change=_on_toggle)

    def on_group_toggle(group_norm: str, enabled: bool):
      if filter_state['rendering_groups']:
        return
      if enabled:
        filter_state['selected_groups'].add(group_norm)
      else:
        filter_state['selected_groups'].discard(group_norm)
      render_mod_toggles()

    def on_filter_toggle(event):
      filter_state['enabled'] = bool(getattr(event, 'value', group_filter_checkbox.value))
      render_mod_toggles()

    group_filter_checkbox.on('update:model-value', on_filter_toggle)

    def set_all_mods(value: bool):
      for key in list(config.relay.modifier_data.keys()):
        config.relay.enable_mod(key, value)
      sync_enabled_mods()
      config.relay.save_mod_info()
      render_mod_toggles()

    render_mod_toggles()

    def save_settings():
      need_save = False

      def set_if_changed(current, value, setter):
        nonlocal need_save
        if current != value:
          setter(value)
          need_save = True

      set_if_changed(config.relay.num_active_mods, safe_int(num_active_mods.value, config.relay.num_active_mods, 1, 10), config.relay.set_num_active_mods)
      set_if_changed(
        config.relay.time_per_modifier,
        safe_float(time_per_modifier.value, config.relay.time_per_modifier, 1.0, 172800.0),
        config.relay.set_time_per_modifier,
      )
      set_if_changed(config.relay.softmax_factor, safe_int(softmax_factor.value, config.relay.softmax_factor, 1, 100), config.relay.set_softmax_factor)
      set_if_changed(config.relay.vote_options, safe_int(vote_options.value, config.relay.vote_options, 2, 5), config.relay.set_vote_options)
      set_if_changed(config.relay.voting_type, str(voting_type.value), config.relay.set_voting_type)
      set_if_changed(config.relay.voting_cycle, str(voting_cycle.value), config.relay.set_voting_cycle)
      set_if_changed(
        config.relay.startup_random_modifier,
        bool(startup_random_modifier.value),
        config.relay.set_startup_random_modifier,
      )
      current_vote_time = relay_config_float('vote_time', 60.0, 1.0, 3600.0)
      new_vote_time = safe_float(vote_time.value, current_vote_time, 1.0, 3600.0)
      if current_vote_time != new_vote_time:
        config.relay.set_vote_duration(new_vote_time)
        need_save = True
      current_vote_delay = relay_config_float('vote_delay', 0.0, 0.0, 3600.0)
      new_vote_delay = safe_float(vote_delay.value, current_vote_delay, 0.0, 3600.0)
      if current_vote_delay != new_vote_delay:
        config.relay.set_vote_delay(new_vote_delay)
        need_save = True
      set_if_changed(config.relay.announce_candidates, bool(announce_candidates.value), config.relay.set_announce_candidates)
      set_if_changed(config.relay.announce_winner, bool(announce_winner.value), config.relay.set_announce_winner)
      set_if_changed(config.relay.announce_active, bool(announce_active.value), config.relay.set_announce_active)
      set_if_changed(config.relay.bits_redemptions, bool(bits_redemptions.value), config.relay.set_bits_redemptions)
      set_if_changed(config.relay.points_redemptions, bool(points_redemptions.value), config.relay.set_points_redemptions)
      set_if_changed(config.relay.raffles, bool(raffles.value), config.relay.set_raffles)
      set_if_changed(config.relay.multiple_credits, bool(multiple_credits.value), config.relay.set_multiple_credits)
      set_if_changed(
        config.relay.redemption_cooldown,
        safe_int(redemption_cooldown.value, int(config.relay.redemption_cooldown), 0, 86400),
        config.relay.set_redemption_cooldown,
      )
      set_if_changed(config.relay.bits_per_credit, safe_int(bits_per_credit.value, config.relay.bits_per_credit, 1, 100000), config.relay.set_bits_per_credit)
      set_if_changed(config.relay.points_reward_title, str(points_reward_title.value), config.relay.set_points_reward_title)
      set_if_changed(config.relay.raffle_time, safe_float(raffle_time.value, config.relay.raffle_time, 30.0, 3600.0), config.relay.set_raffle_time)
      new_mod_list_link = str(mod_list_link.value or '').strip()
      if str(config.relay.mod_list_link or '') != new_mod_list_link:
        config.relay.set_mod_list_link(new_mod_list_link, persist_override=True)
        need_save = True
      new_chaoscmd_link = str(chaoscmd_link.value or '').strip()
      if str(config.relay.chaoscmd_link or '') != new_chaoscmd_link:
        config.relay.set_chaoscmd_link(new_chaoscmd_link)
        need_save = True
      mod_list_state['pending_user_edit'] = False
      sync_enabled_mods()
      config.relay.save_mod_info()

      if need_save:
        config.relay.set_need_save(True)
        status_label.text = 'Settings updated'
      else:
        status_label.text = 'No settings changed'

    def restore_settings():
      config.relay.reset_all()
      num_active_mods.value = int(config.relay.num_active_mods)
      time_per_modifier.value = float(config.relay.time_per_modifier)
      softmax_factor.value = int(config.relay.softmax_factor)
      vote_options.value = int(config.relay.vote_options)
      voting_type.value = config.relay.voting_type
      voting_cycle.value = config.relay.voting_cycle
      startup_random_modifier.value = bool(config.relay.startup_random_modifier)
      vote_time.value = relay_config_float('vote_time', 60.0, 1.0, 3600.0)
      vote_delay.value = relay_config_float('vote_delay', 0.0, 0.0, 3600.0)
      announce_candidates.value = bool(config.relay.announce_candidates)
      announce_winner.value = bool(config.relay.announce_winner)
      announce_active.value = bool(config.relay.announce_active)
      bits_redemptions.value = bool(config.relay.bits_redemptions)
      points_redemptions.value = bool(config.relay.points_redemptions)
      raffles.value = bool(config.relay.raffles)
      multiple_credits.value = bool(config.relay.multiple_credits)
      redemption_cooldown.value = int(config.relay.redemption_cooldown)
      bits_per_credit.value = int(config.relay.bits_per_credit)
      points_reward_title.value = str(config.relay.points_reward_title)
      raffle_time.value = float(config.relay.raffle_time)
      mod_list_link.value = str(config.relay.mod_list_link or '')
      chaoscmd_link.value = str(config.relay.chaoscmd_link or '')
      mod_list_state['pending_user_edit'] = False
      mod_list_state['bound_game'] = str(config.relay.game_name or 'NONE')
      render_mod_toggles()
      status_label.text = 'Restored saved settings'

    def reset_softmax():
      config.relay.reset_softmax()
      status_label.text = 'Modifier history reset'

    with button_bar:
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
      ui.button('Reset Modifier History', on_click=reset_softmax)

    return refresh_game_selector
