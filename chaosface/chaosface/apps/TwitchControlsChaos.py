#!/usr/bin/env python3
# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""
Main entrypoint for the Twitch Controls Chaos operator UI and OBS overlays.

The operator UI is rendered with NiceGUI. Overlay pages are plain HTML served by FastAPI
endpoints for use as OBS browser sources.
"""

import argparse
import asyncio
import logging
import threading
from typing import Any, Dict, Optional

from fastapi.responses import HTMLResponse
from nicegui import app, ui

import chaosface.config.globals as config
from chaosface.ChaosModel import ChaosModel
from chaosface.chatbot.ChaosBot import ChaosBot
from chaosface.chatbot.context import RelayBotContext
from chaosface.gui import ui_dispatch
from chaosface.gui.ActiveMods import active_mods_overlay_html
from chaosface.gui.ChaosInterface import build_chaos_interface
from chaosface.gui.CurrentVotes import current_votes_overlay_html
from chaosface.gui.VoteTimer import vote_timer_overlay_html
from chaosface.gui.overlay_state import overlay_state_payload

logging.basicConfig(level=logging.DEBUG)

_runtime_started = False
_runtime_lock = threading.Lock()
_model: Optional[ChaosModel] = None


def _on_bot_connected(connected: bool):
  ui_dispatch.call_soon(config.relay.set_connected, connected)


def _on_bot_vote(vote_num: int, user: str):
  ui_dispatch.call_soon(config.relay.tally_vote, vote_num, user)


def ensure_runtime_started() -> None:
  """
  Bind relay updates to the running asyncio loop and start model thread once.
  """
  global _runtime_started, _model
  if _runtime_started:
    return
  try:
    loop = asyncio.get_running_loop()
  except RuntimeError:
    return

  with _runtime_lock:
    if _runtime_started:
      return
    ui_dispatch.set_ui_loop(loop)
    if _model is None:
      _model = ChaosModel()
    if _model.thread is None or not _model.thread.is_alive():
      _model.start_threaded()
    _runtime_started = True


def shutdown_runtime() -> None:
  config.relay.keep_going = False
  if config.relay.chatbot:
    config.relay.chatbot.shutdown()


@ui.page('/')
async def chaos_interface() -> None:
  build_chaos_interface(
    ensure_runtime_started=ensure_runtime_started,
    shutdown_runtime=shutdown_runtime,
  )


@app.get('/api/overlay/state')
async def api_overlay_state() -> Dict[str, Any]:
  ensure_runtime_started()
  return overlay_state_payload()


@app.get('/ActiveMods', response_class=HTMLResponse)
@app.get('/overlays/active-mods', response_class=HTMLResponse)
async def active_mods_overlay() -> HTMLResponse:
  ensure_runtime_started()
  return HTMLResponse(active_mods_overlay_html())


@app.get('/VoteTimer', response_class=HTMLResponse)
@app.get('/overlays/vote-timer', response_class=HTMLResponse)
async def vote_timer_overlay() -> HTMLResponse:
  ensure_runtime_started()
  return HTMLResponse(vote_timer_overlay_html())


@app.get('/CurrentVotes', response_class=HTMLResponse)
@app.get('/overlays/current-votes', response_class=HTMLResponse)
async def current_votes_overlay() -> HTMLResponse:
  ensure_runtime_started()
  return HTMLResponse(current_votes_overlay_html())


def twitch_controls_chaos(args):
  del args

  bot_context = RelayBotContext(config.relay)
  config.relay.chatbot = ChaosBot(
    context=bot_context,
    on_connected=_on_bot_connected,
    on_vote=_on_bot_vote,
  )

  try:
    ui.run(
      host='0.0.0.0',
      port=config.relay.get_attribute('ui_port'),
      title='Twitch Controls Chaos',
      show=False,
      reload=False,
    )
  finally:
    shutdown_runtime()


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--reauthorize", help="Delete previous OAuth tokens and wait for new ones to attempt to connect")
  args = parser.parse_args()
  twitch_controls_chaos(args)
