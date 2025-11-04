<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <title>RGB + LED Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <style>
    body{font-family:system-ui,Segoe UI,Roboto,Inter,Arial;margin:2rem;line-height:1.35}
    .row{display:flex;gap:14px;align-items:center;margin:.5rem 0}
    input[type=range]{width:280px}
    fieldset{border:1px solid #ddd;border-radius:10px;padding:12px;margin-bottom:16px}
    legend{padding:0 6px}
    button{padding:.6rem 1rem;border-radius:10px;border:0;background:#2b5cff;color:#fff;font-weight:600}
    #status{margin-top:.5rem;font-size:.95rem}
    pre{background:#f6f8fa;border:1px solid #e2e2e2;padding:10px;border-radius:8px;overflow:auto}
    .pill{display:inline-block;padding:.25rem .55rem;border-radius:999px;background:#eef;border:1px solid #ccd;color:#223}
  </style>
</head>
<body>
  <h1>RGB + LED Control (Node1)</h1>

  <form id="controlForm">
    <fieldset>
      <legend>RGB Channels</legend>
      <div class="row">
        <label for="r">Red</label>
        <input type="range" id="r" name="r" min="0" max="255" value="0" oninput="rv.value=this.value">
        <output id="rv" class="pill">0</output>
      </div>
      <div class="row">
        <label for="g">Green</label>
        <input type="range" id="g" name="g" min="0" max="255" value="0" oninput="gv.value=this.value">
        <output id="gv" class="pill">0</output>
      </div>
      <div class="row">
        <label for="b">Blue</label>
        <input type="range" id="b" name="b" min="0" max="255" value="0" oninput="bv.value=this.value">
        <output id="bv" class="pill">0</output>
      </div>
    </fieldset>

    <fieldset>
      <legend>LED1 Power</legend>
      <div class="row">
        <label><input type="radio" name="led" value="on"> ON</label>
        <label><input type="radio" name="led" value="off"> OFF</label>
      </div>
      <small>Tip: You can keep LED1 coupled to RGB in PHP if you want (ON when any channel &gt; 0), or control it independently here.</small>
    </fieldset>

    <button type="submit">Submit</button>
    <div id="status" aria-live="polite"></div>
  </form>

  <h3>Current State</h3>
  <pre id="state">loading…</pre>

  <script>
    async function fetchState() {
      const r = await fetch('read_state.php', {cache:'no-store'});
      const j = await r.json();
      document.getElementById('state').textContent = JSON.stringify(j, null, 2);

      // Prefill UI from state if present
      const n1 = j?.nodes?.Node1;
      if (n1) {
        const rgb = n1.RGB || {r:0,g:0,b:0};
        const led = (n1.LED1 || 'off').toLowerCase();

        const rEl = document.getElementById('r');
        const gEl = document.getElementById('g');
        const bEl = document.getElementById('b');
        rEl.value = rgb.r ?? 0; gEl.value = rgb.g ?? 0; bEl.value = rgb.b ?? 0;
        document.getElementById('rv').value = rEl.value;
        document.getElementById('gv').value = gEl.value;
        document.getElementById('bv').value = bEl.value;

        const on = document.querySelector('input[name="led"][value="on"]');
        const off = document.querySelector('input[name="led"][value="off"]');
        if (on && off) (led === 'on' ? on : off).checked = true;
      }
    }

    async function saveState(e) {
      e.preventDefault();
      const form = document.getElementById('controlForm');
      const data = new FormData(form);

      // Ensure LED value exists (default to current UI selection or 'off')
      if (!data.get('led')) data.set('led', 'off');

      // Build JSON payload
      const payload = {
        node: 'Node1',
        r: Math.max(0, Math.min(255, parseInt(data.get('r') || '0', 10))),
        g: Math.max(0, Math.min(255, parseInt(data.get('g') || '0', 10))),
        b: Math.max(0, Math.min(255, parseInt(data.get('b') || '0', 10))),
        LED1: (data.get('led') || 'off').toLowerCase()
      };

      // POST JSON to slider_write.php
      const s = document.getElementById('status');
      s.textContent = 'Saving…';
      try {
        const res = await fetch('slider_write.php', {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify(payload)
        });
        const j = await res.json();
        if (!res.ok || !j.ok) throw new Error('Save failed');
        s.textContent = 'Saved ✓';
        // Refresh the bottom state panel with the latest server state
        document.getElementById('state').textContent = JSON.stringify(j.state, null, 2);
      } catch (err) {
        s.textContent = 'Error saving: ' + err.message;
      }
    }

    document.getElementById('controlForm').addEventListener('submit', saveState);
    fetchState();
  </script>
</body>
</html>
