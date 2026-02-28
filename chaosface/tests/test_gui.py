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
