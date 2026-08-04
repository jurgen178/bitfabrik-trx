#pragma once

#include <Arduino.h>

static const char CONFIG_EDITOR_HTML[] PROGMEM = R"html(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>TRX Band Tabelle</title>
  <style>
    html, body { height: 100%; margin: 0; background: #121417; color: white; overflow: hidden; }
    #top { padding: 10px; font-family: sans-serif; background: #1c1f26; border-bottom: 1px solid #2d3748; }
    #editor { height: calc(100% - 64px); }
    button { margin-right: 8px; padding: 5px 15px; cursor: pointer; background: #2d3748; border: 1px solid #4a5568; color: white; border-radius: 4px; }
    button:hover { background: #4a5568; }
    #msg { margin-left: 12px; font-size: 0.9em; color: #00aaff; }
  </style>
  <script src="https://unpkg.com/monaco-editor@0.54.0/min/vs/loader.js"></script>
</head>
<body>
  <div id="top">
    <button id="btnReload">Reload</button>
    <button id="btnSave" style="background:#00aaff; border-color:#00aaff">Save & Restart</button>
    <button id="btnReset" style="background:#e53e3e; border-color:#e53e3e">Reset to Defaults</button>
    <button id="btnExport">Export</button>
    <input type="file" id="fileImport" accept="application/json" style="display:none"/>
    <button onclick="document.getElementById('fileImport').click()">Import</button>
    <span id="msg" style="font-weight:bold">TRX BAND TABELLE</span>
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

    async function reloadConfig() {
      try {
        const res = await fetch('/bands.json', { cache: 'no-store' });
        if (!res.ok) return;
        const text = await res.text();
        editor.setValue(text);
      } catch (e) {}
    }

    async function saveConfig() {
      if(!confirm("Änderungen speichern? (Das Gerät startet neu. Bitte lade die Seite danach manuell neu.)")) return;
      try {
        await fetch('/bands.json', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: editor.getValue()
        });
      } catch (e) {}
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

      document.getElementById('btnReload').onclick = reloadConfig;
      document.getElementById('btnSave').onclick = saveConfig;
      document.getElementById('btnExport').onclick = () => download('bands.json', editor.getValue());

      document.getElementById('btnReset').onclick = async () => {
        if(!confirm("Alle Anpassungen löschen und auf Standardwerte zurücksetzen? (Das Gerät startet neu. Bitte lade die Seite danach manuell neu.)")) return;
        try {
            await fetch('/bands.json', { method: 'DELETE' });
        } catch (e) {}
      };

      const fileImport = document.getElementById('fileImport');
      fileImport.addEventListener('change', async (ev) => {
        const f = ev.target.files && ev.target.files[0];
        if (!f) return;
        editor.setValue(await f.text());
      });

      reloadConfig();
    });
  </script>
</body>

      const fileImport = document.getElementById('fileImport');
      fileImport.addEventListener('change', async (ev) => {
        const f = ev.target.files && ev.target.files[0];
        if (!f) return;
        editor.setValue(await f.text());
        msg('imported (not saved)');
      });

      reloadConfig();
    });
  </script>
</body>
</html>
)html";
