# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""OBS overlay HTML for vote timer progress."""


def vote_timer_overlay_html() -> str:
  return """<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Vote Timer</title>
    <style>
      :root { color-scheme: dark; }
      body {
        margin: 0;
        font-family: Arial, sans-serif;
        background: transparent;
      }
      .wrap { width: 100%; max-width: 1100px; padding: 8px 12px; }
      .bar {
        height: 28px;
        border: 1px solid #000;
        border-radius: 8px;
        background: rgba(128, 128, 128, 0.8);
        overflow: hidden;
      }
      .bar-fill {
        width: 50%;
        height: 100%;
        background: rgba(240, 240, 240, 0.85);
      }
    </style>
  </head>
  <body>
    <div class="wrap">
      <div class="bar"><div id="timerFill" class="bar-fill"></div></div>
    </div>
    <script>
      async function refresh() {
        const response = await fetch('/api/overlay/state', { cache: 'no-store' });
        const state = await response.json();
        const progress = Math.max(0, Math.min(1, Number(state.vote_time || 0)));
        document.getElementById('timerFill').style.width = `${progress * 100}%`;
      }
      setInterval(refresh, 250);
      refresh();
    </script>
  </body>
</html>
"""
