# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""OBS overlay HTML for current vote standings."""


def current_votes_overlay_html() -> str:
  return """<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Current Votes</title>
    <style>
      :root { color-scheme: dark; }
      body {
        margin: 0;
        font-family: Arial, sans-serif;
        background: transparent;
        color: #fff;
        text-shadow: -1px 0 #000, 0 1px #000, 1px 0 #000, 0 -1px #000;
      }
      .wrap { width: 100%; max-width: 900px; padding: 8px 12px; }
      .header {
        display: flex;
        justify-content: space-between;
        font-size: 24px;
        font-weight: bold;
        margin-bottom: 8px;
      }
      .row {
        display: grid;
        grid-template-columns: 3fr 2fr;
        gap: 10px;
        align-items: center;
        margin-bottom: 8px;
      }
      .label {
        font-size: 20px;
        font-weight: bold;
        min-height: 28px;
      }
      .bar {
        height: 24px;
        border: 1px solid #000;
        border-radius: 6px;
        background: rgba(128, 128, 128, 0.7);
        overflow: hidden;
        position: relative;
      }
      .bar-fill {
        position: absolute;
        left: 0;
        top: 0;
        height: 100%;
        width: 0%;
        background: rgba(245, 245, 245, 0.8);
      }
      .bar-text {
        position: absolute;
        width: 100%;
        line-height: 24px;
        text-align: center;
        font-size: 14px;
        font-weight: bold;
      }
    </style>
  </head>
  <body>
    <div class="wrap">
      <div class="header">
        <span id="voteTotal">Total Votes: 0</span>
        <span>&nbsp;</span>
      </div>
      <div id="votes"></div>
    </div>
    <script>
      async function refresh() {
        const response = await fetch('/api/overlay/state', { cache: 'no-store' });
        const state = await response.json();
        const names = state.candidate_mods || [];
        const votes = state.votes || [];
        const totalVotes = Number(state.vote_total || 0);
        const count = Math.max(names.length, votes.length);
        document.getElementById('voteTotal').textContent = `Total Votes: ${totalVotes}`;
        let html = '';
        for (let i = 0; i < count; i++) {
          const name = names[i] || '';
          const vote = Number(votes[i] || 0);
          const percent = totalVotes > 0 ? (vote / totalVotes) : (count > 0 ? 1 / count : 0);
          const pctText = `${Math.round(percent * 100)}%`;
          html += `
            <div class="row">
              <div class="label">${i + 1} ${name}</div>
              <div class="bar">
                <div class="bar-fill" style="width:${percent * 100}%"></div>
                <div class="bar-text">${pctText}</div>
              </div>
            </div>`;
        }
        document.getElementById('votes').innerHTML = html;
      }
      setInterval(refresh, 250);
      refresh();
    </script>
  </body>
</html>
"""
