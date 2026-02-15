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

from fastapi import HTTPException, Request
from fastapi.responses import HTMLResponse, RedirectResponse
from nicegui import app, ui

import chaosface.config.globals as config
import chaosface.config.token_utils as token_utils
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


def _oauth_callback_html() -> str:
  return """<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Twitch OAuth Complete</title>
    <style>
      body { font-family: sans-serif; margin: 2rem; line-height: 1.5; }
      code { background: #f3f3f3; padding: 0.1rem 0.3rem; border-radius: 0.2rem; }
    </style>
  </head>
  <body>
    <h2>Twitch OAuth</h2>
    <p id="status">Finalizing token capture...</p>
    <p><a href="/">Return to Chaosface connection setup</a></p>
    <script>
      (async () => {
        const status = document.getElementById('status');
        const hashParams = new URLSearchParams(window.location.hash.slice(1));
        const queryParams = new URLSearchParams(window.location.search);
        const token = hashParams.get('access_token');
        const state = hashParams.get('state');
        const error = hashParams.get('error') || queryParams.get('error');
        const errorDescription = hashParams.get('error_description') || queryParams.get('error_description') || '';

        if (error) {
          status.textContent = `OAuth failed: ${error}${errorDescription ? ` (${errorDescription})` : ''}`;
          return;
        }
        if (!token || !state) {
          status.textContent = 'Missing OAuth token or state in callback URL.';
          return;
        }

        try {
          const response = await fetch('/api/oauth/complete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ access_token: token, state }),
          });
          const payload = await response.json();
          if (!response.ok) {
            status.textContent = payload.detail || 'Failed to store OAuth token.';
            return;
          }
          status.textContent = `Token saved for ${payload.target}. Return to Connection Setup and click "Load generated tokens".`;
        } catch (err) {
          status.textContent = `Failed to store token: ${String(err)}`;
        }
      })();
    </script>
  </body>
</html>
"""


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


@app.get('/api/oauth/start')
async def api_oauth_start(request: Request, target: str) -> RedirectResponse:
  oauth_target = (target or '').strip().lower()
  if oauth_target not in token_utils.SUPPORTED_OAUTH_TARGETS:
    raise HTTPException(status_code=400, detail='Invalid OAuth target')

  client_id = str(config.relay.get_attribute('client_id') or '').strip()
  if not client_id:
    raise HTTPException(status_code=400, detail='Set Twitch client_id before generating OAuth tokens')

  state = token_utils.create_oauth_state(oauth_target)
  redirect_uri = str(request.url_for('api_oauth_callback'))
  scopes = token_utils.get_scopes_for_target(oauth_target)
  auth_url = token_utils.build_authorize_url(client_id, redirect_uri, list(scopes), state)
  return RedirectResponse(auth_url)


@app.get('/api/oauth/callback', name='api_oauth_callback', response_class=HTMLResponse)
async def api_oauth_callback() -> HTMLResponse:
  return HTMLResponse(_oauth_callback_html())


@app.post('/api/oauth/complete')
async def api_oauth_complete(payload: Dict[str, str]) -> Dict[str, str]:
  state = str(payload.get('state') or '').strip()
  access_token = str(payload.get('access_token') or '').strip()
  if not state or not access_token:
    raise HTTPException(status_code=400, detail='Missing OAuth callback state or token')

  target = token_utils.consume_oauth_state(state)
  if target is None:
    raise HTTPException(status_code=400, detail='OAuth state is invalid or expired. Try generating a new token.')

  token_utils.store_generated_oauth(target, access_token)
  return {'target': target}


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
