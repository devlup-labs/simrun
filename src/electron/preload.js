const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('compilerAPI', {
  run: (json) => ipcRenderer.invoke('run-compiler', json)
});
