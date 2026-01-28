const { app, BrowserWindow, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const http = require('http');

let compilerProcess = null;

function startCompiler() {
  const compilerPath = path.resolve(__dirname, '../../compiler/compiler.exe');

  compilerProcess = spawn(compilerPath, [], {
    stdio: 'inherit'
  });

  compilerProcess.on('error', (err) => {
    console.error('Failed to start compiler:', err);
  });

  compilerProcess.on('exit', (code) => {
    console.log('Compiler exited with code', code);
  });
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  win.loadURL('http://localhost:8080');
}

/* ---------------- IPC HANDLERS ---------------- */

ipcMain.handle('ping-electron', async () => {
  return 'Pong from Electron main process';
});

function postToCompiler(pathname, data) {
  return new Promise((resolve, reject) => {
    const jsonData = JSON.stringify(data);

    const options = {
      hostname: '127.0.0.1',
      port: 18080,
      path: pathname,
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(jsonData)
      }
    };

    const req = http.request(options, (res) => {
      let body = '';
      res.on('data', chunk => body += chunk);
      res.on('end', () => {
        try {
          resolve(JSON.parse(body));
        } catch (e) {
          reject(e);
        }
      });
    });

    req.on('error', reject);
    req.write(jsonData);
    req.end();
  });
}

ipcMain.handle('simulate-project', async (_, projectJson) => {
  try {
    const response = await postToCompiler('/api/v1/simulate', {
      project: projectJson,
      options: {}
    });

    return response;
  } catch (err) {
    return {
      status: "error",
      phase: "system",
      valid: false,
      errors: [{
        code: "COMPILER_UNAVAILABLE",
        message: "Compiler server not reachable"
      }],
      warnings: [],
      results: null
    };
  }
});

/* ---------------- APP LIFECYCLE ---------------- */

app.whenReady().then(() => {
  startCompiler();
  createWindow();
});

app.on('before-quit', () => {
  if (compilerProcess) {
    compilerProcess.kill();
  }
});

app.on('window-all-closed', () => {
  app.quit();
});
