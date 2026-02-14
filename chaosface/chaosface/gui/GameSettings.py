# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""Game settings tab UI for the NiceGUI operator interface."""

from __future__ import annotations

from nicegui import ui

import chaosface.config.globals as config

from .ui_helpers import relay_config_float, safe_float, safe_int, sync_enabled_mods


def build_game_settings_tab() -> None:
  with ui.card().classes('w-full'):
    ui.label('Game Settings').classes('text-h6')

    status_label = ui.label('').classes('text-sm')

    ui.label('Game Selection').classes('text-subtitle1')

    with ui.row().classes('w-full items-end gap-3'):
      game_selector = ui.select([], label='Available games').classes('w-96')
      game_status = ui.label('').classes('text-xs')

      def confirm_game_selection():
        selected = str(game_selector.value or '').strip()
        if not selected:
          status_label.text = 'Select a game before confirming'
          return
        config.relay.request_game_selection(selected)
        status_label.text = f'Requested game load: {selected}'

      ui.button('Confirm Game', on_click=confirm_game_selection)

    def refresh_game_selector():
      options = list(config.relay.available_games)
      if game_selector.options != options:
        game_selector.options = options
        game_selector.update()

      selected = str(config.relay.selected_game or '').strip()
      if selected.upper() == 'NONE':
        selected = ''
      if selected not in options:
        selected = options[0] if options else None
      if game_selector.value != selected:
        game_selector.value = selected

      game_status.text = (
        f'Current: {config.relay.game_name or "NONE"} | '
        f'Most recent: {config.relay.selected_game or "NONE"} | '
        f'Selections stored: {len(config.relay.selected_game_history)}'
      )

    refresh_game_selector()
    ui.timer(0.5, refresh_game_selector)

    ui.separator()

    with ui.row().classes('w-full gap-8'):
      with ui.column().classes('w-96'):
        num_active_mods = ui.number('Active modifiers', value=int(config.relay.num_active_mods), min=1, max=10, step=1)
        time_per_modifier = ui.number('Time per modifier (s)', value=float(config.relay.time_per_modifier), min=1, max=172800, step=1)
        softmax_factor = ui.slider(min=1, max=100, value=int(config.relay.softmax_factor))
        vote_options = ui.number('Vote options', value=int(config.relay.vote_options), min=2, max=5, step=1)
        voting_type = ui.select(['Proportional', 'Majority', 'Authoritarian'], value=config.relay.voting_type, label='Voting method')
        voting_cycle = ui.select(
          ['Continuous', 'Interval', 'Random', 'Triggered', 'DISABLED'],
          value=config.relay.voting_cycle,
          label='Voting cycle',
        )
        vote_time = ui.number(
          'Vote time (s)',
          value=relay_config_float('vote_time', 60.0, 1.0, 3600.0),
          min=1,
          max=3600,
          step=1,
        )
        vote_delay = ui.number(
          'Vote delay (s)',
          value=relay_config_float('vote_delay', 0.0, 0.0, 3600.0),
          min=0,
          max=3600,
          step=1,
        )

      with ui.column().classes('w-96'):
        announce_candidates = ui.checkbox('Announce candidates in chat', value=bool(config.relay.announce_candidates))
        announce_winner = ui.checkbox('Announce winner in chat', value=bool(config.relay.announce_winner))
        announce_active = ui.checkbox('Announce active mods in chat', value=bool(config.relay.announce_active))
        bits_redemptions = ui.checkbox('Allow bits redemptions', value=bool(config.relay.bits_redemptions))
        points_redemptions = ui.checkbox('Allow points redemptions', value=bool(config.relay.points_redemptions))
        raffles = ui.checkbox('Conduct raffles', value=bool(config.relay.raffles))
        multiple_credits = ui.checkbox('Allow multiple credits per cheer', value=bool(config.relay.multiple_credits))
        redemption_cooldown = ui.number('Redemption cooldown (s)', value=float(config.relay.redemption_cooldown), min=0, max=86400, step=1)
        bits_per_credit = ui.number('Bits per mod credit', value=int(config.relay.bits_per_credit), min=1, max=100000, step=1)
        points_reward_title = ui.input('Points reward title', value=str(config.relay.points_reward_title))
        raffle_time = ui.number('Default raffle time (s)', value=float(config.relay.raffle_time), min=10, max=3600, step=1)

    ui.separator()
    ui.label('Available Modifiers').classes('text-subtitle1')
    mod_toggles = ui.column().classes('w-full gap-1')

    def render_mod_toggles():
      mod_toggles.clear()
      mods = sorted(
        config.relay.modifier_data.items(),
        key=lambda item: str(item[1].get('name', item[0])).lower(),
      )
      with mod_toggles:
        for key, mod_data in mods:
          def _on_toggle(event, mod_key=key):
            config.relay.enable_mod(mod_key, bool(event.value))
            sync_enabled_mods()
            config.relay.save_mod_info()

          ui.checkbox(mod_data.get('name', key), value=bool(mod_data.get('active', True)), on_change=_on_toggle)

    def set_all_mods(value: bool):
      for key in list(config.relay.modifier_data.keys()):
        config.relay.enable_mod(key, value)
      sync_enabled_mods()
      config.relay.save_mod_info()
      render_mod_toggles()

    with ui.row().classes('gap-2'):
      ui.button('Enable All', on_click=lambda: set_all_mods(True))
      ui.button('Disable All', on_click=lambda: set_all_mods(False))

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
      current_vote_time = relay_config_float('vote_time', 60.0, 1.0, 3600.0)
      new_vote_time = safe_float(vote_time.value, current_vote_time, 1.0, 3600.0)
      if current_vote_time != new_vote_time:
        config.relay.chaos_config['vote_time'] = new_vote_time
        need_save = True
      current_vote_delay = relay_config_float('vote_delay', 0.0, 0.0, 3600.0)
      new_vote_delay = safe_float(vote_delay.value, current_vote_delay, 0.0, 3600.0)
      if current_vote_delay != new_vote_delay:
        config.relay.chaos_config['vote_delay'] = new_vote_delay
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
      set_if_changed(config.relay.raffle_time, safe_float(raffle_time.value, config.relay.raffle_time, 10.0, 3600.0), config.relay.set_raffle_time)
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
      render_mod_toggles()
      status_label.text = 'Restored saved settings'

    def reset_softmax():
      config.relay.reset_softmax()
      status_label.text = 'Modifier history reset'

    with ui.row().classes('gap-2'):
      ui.button('Save', on_click=save_settings)
      ui.button('Restore', on_click=restore_settings)
      ui.button('Reset Modifier History', on_click=reset_softmax)
