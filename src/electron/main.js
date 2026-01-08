const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');
const { execFile } = require('child_process');

const isDev = !app.isPackaged;

function createWindow() {
  const win = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  if (isDev) {
    win.loadURL('http://localhost:8080');
  } else {
    win.loadFile(
      path.join(__dirname, '../lovable/ui/dist/index.html')
    );
  }
}

ipcMain.handle('run-compiler', async (_, jsonString) => {
  return new Promise((resolve, reject) => {
    try {
      const inputPath = path.resolve(
        __dirname,
        '..',
        'compiler',
        'input.json'
      );

      fs.writeFileSync(inputPath, jsonString, 'utf-8');

      const compilerPath = path.resolve(
        __dirname,
        '..',
        'compiler',
        'compiler.exe'
      );

      execFile(compilerPath, [inputPath], (err, stdout, stderr) => {
        if (err) {
          reject(stderr || err.message);
        } else {
          resolve(stdout);
        }
      });
    } catch (e) {
      reject(e.message);
    }
  });
});

app.whenReady().then(createWindow);
