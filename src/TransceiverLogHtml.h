#pragma once

#include <Arduino.h>

static const char LOG_EDITOR_HTML[] PROGMEM = R"html(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>TRX Sende Log</title>
  <style>
    html, body { height: 100%; margin: 0; background: #121417; color: white; overflow: hidden; }
    #top { padding: 10px; font-family: sans-serif; background: #1c1f26; border-bottom: 1px solid #2d3748; display: flex; align-items: center; }
    #editor { height: calc(100% - 64px); }
    button { margin-right: 8px; padding: 5px 15px; cursor: pointer; background: #2d3748; border: 1px solid #4a5568; color: white; border-radius: 4px; font-weight: bold; }
    button:hover { background: #4a5568; }
    .btn-save { background: #00ff88 !important; color: #121417 !important; border-color: #00ff88 !important; }
    .btn-danger { background: #e53e3e !important; }
    #msg { margin-left: auto; font-weight: bold; color: #00ff88; letter-spacing: 1px; }
  </style>
  <script src="https://unpkg.com/monaco-editor@0.54.0/min/vs/loader.js"></script>
</head>
<body>
  <div id="top">
    <button onclick="location.href='/'">Dashboard</button>
    <button id="btnReload">Reload</button>
    <button id="btnSave" class="btn-save">Save Log</button>
    <button id="btnReset" class="btn-danger">Clear All</button>
    <button id="btnExport">Export</button>
    <span id="msg">TRX SENDE LOG</span>
  </div>
  <div id="editor"></div>

  <script>
    let editor;

    function download(filename, text) {
      const a = document.createElement('a');
      a.setAttribute('href', 'data:application/json;charset=utf-8,' + encodeURIComponent(text));
      a.setAttribute('download', filename);
      a.style.display = 'none';
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
    }

    async function reloadLog() {
      try {
        const res = await fetch('/log.json', { cache: 'no-store' });
        if (!res.ok) return;
        const text = await res.text();
        editor.setValue(text);
      } catch (e) { console.error(e); }
    }

    async function saveLog() {
      if(!confirm("Änderungen im Logbuch speichern?")) return;
      try {
        const res = await fetch('/log.json', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: editor.getValue()
        });
        if(res.ok) alert("Log gespeichert.");
        else alert("Fehler beim Speichern.");
      } catch (e) { console.error(e); }
    }

    require.config({ paths: { 'vs': 'https://unpkg.com/monaco-editor@0.54.0/min/vs' } });
    require(['vs/editor/editor.main'], function () {
      editor = monaco.editor.create(document.getElementById('editor'), {
        value: '[\n]',
        language: 'json',
        theme: 'vs-dark',
        automaticLayout: true,
        tabSize: 2
      });

      document.getElementById('btnReload').onclick = reloadLog;
      document.getElementById('btnSave').onclick = saveLog;
      document.getElementById('btnExport').onclick = () => download('tx_log.json', editor.getValue());

      document.getElementById('btnReset').onclick = async () => {
        if(!confirm("Gesamtes Logbuch unwiderruflich löschen?")) return;
        try {
            const res = await fetch('/log.json', { method: 'DELETE' });
            if(res.ok) {
                editor.setValue('[]');
                alert("Log gelöscht.");
            }
        } catch (e) { console.error(e); }
      };

      reloadLog();
    });
  </script>
</body>
</html>
)html";
