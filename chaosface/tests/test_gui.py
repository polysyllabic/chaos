"""Import and smoke checks for NiceGUI-facing gui modules."""


def test_gui_module_exports_import():
  from chaosface.gui.ActiveMods import active_mods_overlay_html
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
