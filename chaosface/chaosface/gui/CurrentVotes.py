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
      * { box-sizing: border-box; }
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
        display: flex;
        gap: 10px;
        align-items: center;
        margin-bottom: 8px;
      }
      .label {
        font-size: 20px;
        font-weight: bold;
        min-height: 28px;
        flex: 3;
      }
      .bar-cell { flex: 2; }
      .bar {
        width: 100%;
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
        const textColor = String(state.overlay_current_votes_text_color || '#ffffff');
        const barColor = String(state.overlay_current_votes_bar_color || 'rgba(245, 245, 245, 0.8)');
        const gap = Math.max(0, Math.min(120, Number(state.overlay_current_votes_gap || 10)));
        const textSide = String(state.overlay_current_votes_text_side || 'right').toLowerCase() === 'left' ? 'left' : 'right';
        const textAlignRaw = String(state.overlay_current_votes_text_align || 'left').toLowerCase();
        const textAlign = (textAlignRaw === 'left' || textAlignRaw === 'center' || textAlignRaw === 'right') ? textAlignRaw : 'left';
        document.body.style.color = textColor;
        const names = state.candidate_mods || [];
        const votes = state.votes || [];
        const totalVotes = Number(state.vote_total || 0);
        const votingDisabled = Boolean(state.voting_disabled);
        const votingWaiting = Boolean(state.voting_waiting);
        const displayNames = votingWaiting ? [] : names;
        const displayVotes = votingWaiting ? [] : votes;
        const count = Math.max(displayNames.length, displayVotes.length);
        if (votingDisabled) {
          document.getElementById('voteTotal').textContent = 'Voting Disabled';
        } else if (votingWaiting) {
          document.getElementById('voteTotal').textContent = 'Waiting for Voting to Open';
        } else {
          document.getElementById('voteTotal').textContent = `Total Votes: ${totalVotes}`;
        }
        let html = '';
        for (let i = 0; i < count; i++) {
          const name = displayNames[i] || '';
          const vote = Number(displayVotes[i] || 0);
          const percent = totalVotes > 0 ? (vote / totalVotes) : (count > 0 ? 1 / count : 0);
          const pctText = `${Math.round(percent * 100)}%`;
          const labelHtml = `<div class="label" style="text-align:${textAlign}">${i + 1} ${name}</div>`;
          const barHtml = `<div class="bar-cell"><div class="bar"><div class="bar-fill" style="width:${percent * 100}%;background:${barColor}"></div><div class="bar-text">${pctText}</div></div></div>`;
          const rowHtml = (textSide === 'left') ? `${labelHtml}${barHtml}` : `${barHtml}${labelHtml}`;
          html += `
            <div class="row" style="gap:${gap}px">
              ${rowHtml}
            </div>`;
        }
        document.getElementById('votes').innerHTML = html;
      }
      setInterval(refresh, 500);
      refresh();
    </script>
  </body>
</html>
"""
