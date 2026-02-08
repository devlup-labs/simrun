const { app, BrowserWindow, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const http = require('http');

let compilerProcess = null;

function startCompiler() {
  const compilerPath = path.resolve(__dirname, '../../compiler/compiler.exe');
  const compilerDir = path.dirname(compilerPath);

  console.log("Launching compiler at:", compilerPath);
  console.log("Working directory:", compilerDir);

  compilerProcess = spawn(compilerPath, [], {
    stdio: 'inherit',
    windowsHide: true,
    cwd: compilerDir  // Set working directory to compiler folder
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

  win.loadURL('http://localhost:5173');
}

/* ---------------- IPC HANDLERS ---------------- */

ipcMain.handle('ping-electron', async () => {
  return 'Pong from Electron main process';
});

function postToCompiler(data) {
  return new Promise((resolve, reject) => {
    const jsonData = JSON.stringify(data);

    console.log("\n[Electron Main] ➡ Sending POST to http://127.0.0.1:8081/compile");
    console.log("[Electron Main] Request body:", JSON.stringify(data, null, 2));
    console.log("[Electron Main] Payload size:", jsonData.length, "bytes");

    const options = {
      hostname: '127.0.0.1',
      port: 8081,
      path: '/compile',
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(jsonData)
      }
    };

    const req = http.request(options, (res) => {
      console.log("[Electron Main] ⬅ Response status:", res.statusCode);

      let body = '';
      res.on('data', chunk => body += chunk);
      res.on('end', () => {
        console.log("[Electron Main] ⬅ Response received");
        console.log("[Electron Main] Raw response body:", body);
        try {
          const parsed = JSON.parse(body);
          console.log("[Electron Main] Response parsed successfully");
          resolve(parsed);
        } catch {
          console.error("[Electron Main] Failed to parse response as JSON");
          resolve({ status: "error", raw: body });
        }
      });
    });

    req.on('error', (err) => {
      console.error("[Electron Main] HTTP request failed:", err.message);
      reject(err);
    });

    req.write(jsonData);
    req.end();
  });
}


ipcMain.handle('simulate-project', async (_, projectJson) => {
  console.log("IPC simulate-project received in Electron");
  try {
    const response = await postToCompiler(projectJson);
    return response;
  } catch (err) {
    console.error("Compiler request failed:", err.message);
    return {
      status: "error",
      phase: "system",
      valid: false,
      errors: [{ message: "Compiler server not reachable" }]
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
