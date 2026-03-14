"""Import and smoke checks for NiceGUI-facing gui modules."""


def test_gui_module_exports_import():
  from chaosface.gui.ActiveMods import active_mods_overlay_html
  from chaosface.gui.ChatbotCommands import build_chatbot_commands_tab
  from chaosface.gui.ChaosInterface import build_chaos_interface
  from chaosface.gui.ConnectionSetup import build_connection_tab
  from chaosface.gui.CurrentVotes import current_votes_overlay_html
  from chaosface.gui.GameSettings import build_game_settings_tab
  from chaosface.gui.StreamerInterface import build_streamer_tab
  from chaosface.gui.VoteTimer import vote_timer_overlay_html
  from chaosface.gui.overlay_state import overlay_state_payload
  from chaosface.gui.ui_helpers import clamp01, relay_config_float, safe_float, safe_int, sync_enabled_mods

  assert callable(active_mods_overlay_html)
  assert callable(vote_timer_overlay_html)
  assert callable(current_votes_overlay_html)
  assert callable(build_chatbot_commands_tab)
  assert callable(build_streamer_tab)
  assert callable(build_game_settings_tab)
  assert callable(build_connection_tab)
  assert callable(build_chaos_interface)
  assert callable(overlay_state_payload)
  assert callable(clamp01)
  assert callable(safe_int)
  assert callable(safe_float)
  assert callable(relay_config_float)
  assert callable(sync_enabled_mods)


def test_overlay_html_builders_return_html():
  from chaosface.gui.ActiveMods import active_mods_overlay_html
  from chaosface.gui.CurrentVotes import current_votes_overlay_html
  from chaosface.gui.VoteTimer import vote_timer_overlay_html

  assert '<html>' in active_mods_overlay_html().lower()
  assert '<html>' in current_votes_overlay_html().lower()
  assert '<html>' in vote_timer_overlay_html().lower()
  assert 'overlay_current_votes_text_align' in current_votes_overlay_html()
  assert 'overlay_current_votes_show_total' in current_votes_overlay_html()
  assert 'overlay_active_mods_text_align' in active_mods_overlay_html()
  assert 'Voting Disabled' in current_votes_overlay_html()
  assert 'Waiting for Voting to Open' in current_votes_overlay_html()
  assert 'Voting Open' in current_votes_overlay_html()


def test_overlay_state_includes_text_side_settings():
  import chaosface.config.globals as config
  from chaosface.gui.overlay_state import overlay_state_payload

  prior_votes_side = getattr(config.relay, 'overlay_current_votes_text_side', 'right')
  prior_mods_side = getattr(config.relay, 'overlay_active_mods_text_side', 'right')
  try:
    config.relay.set_overlay_current_votes_text_side('left')
    config.relay.set_overlay_active_mods_text_side('right')
    payload = overlay_state_payload()
    assert payload['overlay_current_votes_text_side'] == 'left'
    assert payload['overlay_active_mods_text_side'] == 'right'
  finally:
    config.relay.set_overlay_current_votes_text_side(prior_votes_side)
    config.relay.set_overlay_active_mods_text_side(prior_mods_side)


def test_overlay_state_includes_voting_disabled_flag():
  import chaosface.config.globals as config
  from chaosface.gui.overlay_state import overlay_state_payload

  prior_voting_type = str(getattr(config.relay, 'voting_type', 'Proportional'))
  prior_voting_cycle = str(getattr(config.relay, 'voting_cycle', 'Continuous'))
  try:
    config.relay.set_voting_type('DISABLED')
    payload = overlay_state_payload()
    assert payload['voting_disabled'] is True

    config.relay.set_voting_type('Proportional')
    config.relay.set_voting_cycle('DISABLED')
    payload = overlay_state_payload()
    assert payload['voting_disabled'] is True

    config.relay.set_voting_cycle('Continuous')
    payload = overlay_state_payload()
    assert payload['voting_disabled'] is False
  finally:
    config.relay.set_voting_type(prior_voting_type)
    config.relay.set_voting_cycle(prior_voting_cycle)


def test_overlay_state_includes_voting_waiting_flag():
  import chaosface.config.globals as config
  from chaosface.gui.overlay_state import overlay_state_payload

  prior_voting_type = str(getattr(config.relay, 'voting_type', 'Proportional'))
  prior_voting_cycle = str(getattr(config.relay, 'voting_cycle', 'Continuous'))
  prior_vote_open = bool(getattr(config.relay, 'vote_open', False))
  try:
    config.relay.set_voting_type('Proportional')
    config.relay.set_voting_cycle('Interval')
    config.relay.set_vote_open(False)
    payload = overlay_state_payload()
    assert payload['voting_waiting'] is True

    config.relay.set_vote_open(True)
    payload = overlay_state_payload()
    assert payload['voting_waiting'] is False

    config.relay.set_voting_cycle('Continuous')
    config.relay.set_vote_open(False)
    payload = overlay_state_payload()
    assert payload['voting_waiting'] is False
  finally:
    config.relay.set_voting_type(prior_voting_type)
    config.relay.set_voting_cycle(prior_voting_cycle)
    config.relay.set_vote_open(prior_vote_open)


def test_overlay_text_side_setters_normalize_invalid_values():
  import chaosface.config.globals as config

  prior_votes_side = getattr(config.relay, 'overlay_current_votes_text_side', 'right')
  prior_mods_side = getattr(config.relay, 'overlay_active_mods_text_side', 'right')
  try:
    config.relay.set_overlay_current_votes_text_side('invalid')
    config.relay.set_overlay_active_mods_text_side('')
    assert config.relay.overlay_current_votes_text_side == 'right'
    assert config.relay.overlay_active_mods_text_side == 'right'
  finally:
    config.relay.set_overlay_current_votes_text_side(prior_votes_side)
    config.relay.set_overlay_active_mods_text_side(prior_mods_side)


def test_overlay_state_includes_text_alignment_settings():
  import chaosface.config.globals as config
  from chaosface.gui.overlay_state import overlay_state_payload

  prior_votes_align = getattr(config.relay, 'overlay_current_votes_text_align', 'left')
  prior_mods_align = getattr(config.relay, 'overlay_active_mods_text_align', 'left')
  try:
    config.relay.set_overlay_current_votes_text_align('center')
    config.relay.set_overlay_active_mods_text_align('right')
    payload = overlay_state_payload()
    assert payload['overlay_current_votes_text_align'] == 'center'
    assert payload['overlay_active_mods_text_align'] == 'right'
  finally:
    config.relay.set_overlay_current_votes_text_align(prior_votes_align)
    config.relay.set_overlay_active_mods_text_align(prior_mods_align)


def test_overlay_state_includes_current_votes_show_total_setting():
  import chaosface.config.globals as config
  from chaosface.gui.overlay_state import overlay_state_payload

  prior_show_total = bool(getattr(config.relay, 'overlay_current_votes_show_total', True))
  try:
    config.relay.set_overlay_current_votes_show_total(False)
    payload = overlay_state_payload()
    assert payload['overlay_current_votes_show_total'] is False

    config.relay.set_overlay_current_votes_show_total(True)
    payload = overlay_state_payload()
    assert payload['overlay_current_votes_show_total'] is True
  finally:
    config.relay.set_overlay_current_votes_show_total(prior_show_total)


def test_overlay_text_alignment_setters_normalize_invalid_values():
  import chaosface.config.globals as config

  prior_votes_align = getattr(config.relay, 'overlay_current_votes_text_align', 'left')
  prior_mods_align = getattr(config.relay, 'overlay_active_mods_text_align', 'left')
  try:
    config.relay.set_overlay_current_votes_text_align('invalid')
    config.relay.set_overlay_active_mods_text_align('')
    assert config.relay.overlay_current_votes_text_align == 'left'
    assert config.relay.overlay_active_mods_text_align == 'left'
  finally:
    config.relay.set_overlay_current_votes_text_align(prior_votes_align)
    config.relay.set_overlay_active_mods_text_align(prior_mods_align)


def test_ui_dark_mode_setter_updates_config():
  import chaosface.config.globals as config

  prior_dark_mode = bool(getattr(config.relay, 'ui_dark_mode', False))
  try:
    config.relay.set_ui_dark_mode(True)
    assert config.relay.ui_dark_mode is True
    assert bool(config.relay.get_attribute('ui_dark_mode')) is True

    config.relay.set_ui_dark_mode(False)
    assert config.relay.ui_dark_mode is False
    assert bool(config.relay.get_attribute('ui_dark_mode')) is False
  finally:
    config.relay.set_ui_dark_mode(prior_dark_mode)


def test_ui_streamer_text_scale_setter_updates_config():
  import chaosface.config.globals as config

  prior_scale = float(getattr(config.relay, 'ui_streamer_text_scale', 2.0))
  try:
    config.relay.set_ui_streamer_text_scale(2.5)
    assert abs(float(config.relay.ui_streamer_text_scale) - 2.5) < 1e-9
    assert abs(float(config.relay.get_attribute('ui_streamer_text_scale')) - 2.5) < 1e-9

    config.relay.set_ui_streamer_text_scale(0.1)
    assert abs(float(config.relay.ui_streamer_text_scale) - 0.5) < 1e-9

    config.relay.set_ui_streamer_text_scale(12.0)
    assert abs(float(config.relay.ui_streamer_text_scale) - 4.0) < 1e-9
  finally:
    config.relay.set_ui_streamer_text_scale(prior_scale)
