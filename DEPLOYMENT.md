# ChessVerse 2D - Deployment Guide

ChessVerse 2D runs a unified Python server (`server.py`) that serves the compiled WebAssembly/HTML game and handles real-time WebSockets on a single port (`$PORT` or 8080).

## 🚀 1-Click / Fast Deployment on Render (Free)

1. Push your code to GitHub:
   ```bash
   git add .
   git commit -m "Deploy online multiplayer"
   git push origin main
   ```

2. Go to [render.com](https://render.com) and log in.
3. Click **New +** -> **Web Service**.
4. Connect your GitHub repository: `https://github.com/arnavkotnala/chess-pbl-175`.
5. Configure the settings:
   - **Environment:** `Python 3`
   - **Build Command:** `pip install -r requirements.txt`
   - **Start Command:** `python server.py`
   - **Plan:** Free
6. Click **Deploy Web Service**!
7. Render will provide a public URL like `https://chess-pbl-175.onrender.com`.

### Features for Web-Hosted Cloud Environments:
- **Unified Port:** Both the web game (`index.html`, `.wasm`, `.data`, assets) and WebSocket connections (`/ws`) run on the single `$PORT` provided by Render/Railway/Heroku.
- **Dynamic Protocol Detection:** Automatically connects over `wss://` on HTTPS and `ws://` on HTTP.
- **WebSocket Heartbeat:** Keeps connection alive and prevents cloud proxy timeouts every 30 seconds.
- **Health Check Endpoint:** `/health` endpoint for uptime monitoring and health checks.
- **Cross-Platform:** Responsive aspect-ratio canvas scaling with touch event forwarding for mobile devices.

---

## 💻 Running Locally

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. Start the server:
   ```bash
   python server.py
   ```

3. Open your browser:
   `http://localhost:8080`
