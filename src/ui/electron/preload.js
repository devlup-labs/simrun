const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('api', {
  ping: () => ipcRenderer.invoke('ping-electron'),
  simulate: (projectJson) => ipcRenderer.invoke('simulate-project', projectJson)
});
