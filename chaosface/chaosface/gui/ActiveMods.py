# This file is part of Twitch Controls Chaos, written by blegas78 and polysyl.
# License: GPL 3 or greater. See LICENSE file for details.
"""OBS overlay HTML for active modifiers."""


def active_mods_overlay_html() -> str:
  return """<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Active Mods</title>
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
      .title { text-align: center; font-weight: bold; font-size: 24px; margin-bottom: 8px; }
      .row {
        display: flex;
        gap: 10px;
        margin-bottom: 8px;
        align-items: center;
      }
      .mod-label { font-size: 20px; font-weight: bold; min-height: 28px; flex: 3; }
      .bar-cell { flex: 2; }
      .bar {
        width: 100%;
        height: 24px;
        border: 1px solid #000;
        border-radius: 6px;
        background: rgba(128, 128, 128, 0.7);
        overflow: hidden;
      }
      .bar-fill {
        height: 100%;
        width: 0%;
        background: rgba(245, 245, 245, 0.75);
      }
    </style>
  </head>
  <body>
    <div class="wrap">
      <div class="title">Active Mods</div>
      <div id="mods"></div>
    </div>
    <script>
      async function refresh() {
        const response = await fetch('/api/overlay/state', { cache: 'no-store' });
        const state = await response.json();
        const modsRoot = document.getElementById('mods');
        const textColor = String(state.overlay_active_mods_text_color || '#ffffff');
        const barColor = String(state.overlay_active_mods_bar_color || 'rgba(245, 245, 245, 0.75)');
        const gap = Math.max(0, Math.min(120, Number(state.overlay_active_mods_gap || 10)));
        const textSide = String(state.overlay_active_mods_text_side || 'right').toLowerCase() === 'left' ? 'left' : 'right';
        document.body.style.color = textColor;
        const names = state.active_mods || [];
        const times = state.mod_times || [];
        const count = Math.max(state.num_active_mods || 0, names.length, times.length);
        let html = '';
        for (let i = 0; i < count; i++) {
          const modName = names[i] || '';
          const progress = Math.max(0, Math.min(1, Number(times[i] || 0)));
          const labelHtml = `<div class="mod-label">${modName}</div>`;
          const barHtml = `<div class="bar-cell"><div class="bar"><div class="bar-fill" style="width:${progress * 100}%;background:${barColor}"></div></div></div>`;
          const rowHtml = (textSide === 'left') ? `${labelHtml}${barHtml}` : `${barHtml}${labelHtml}`;
          html += `
            <div class="row" style="gap:${gap}px">
              ${rowHtml}
            </div>`;
        }
        modsRoot.innerHTML = html;
      }
      setInterval(refresh, 250);
      refresh();
    </script>
  </body>
</html>
"""
