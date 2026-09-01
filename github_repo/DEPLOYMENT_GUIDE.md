# ChessVerse — Online Multiplayer & Cross-Platform Deployment Guide

This guide explains how **ChessVerse** works online, how players connect using **6-digit Room Codes** on **Phones & PCs**, and how to deploy both frontend and backend using **100% Free-Tier Resources** (Netlify, Cloud backend, LAN/WiFi, and Database).

---

## 1. System Architecture

```
                                  ┌─────────────────────────────┐
                                  │   Netlify / GitHub Pages    │
                                  │  (Free Static Web Hosting)  │
                                  │   HTML5 / CSS3 / Vanilla JS │
                                  └──────────────┬──────────────┘
                                                 │
                                                 │ HTTPS / JSON API
                                                 ▼
┌───────────────────────┐         ┌─────────────────────────────┐         ┌───────────────────────┐
│     Player 1 (PC)     │ ◄─────► │   C/C++ Backend Server      │ ◄─────► │    Player 2 (Phone)   │
│  White or Black Piece │  Room   │   Port :8080 (Multi-Room)   │  Room   │  Black or White Piece │
│   Browser / Mobile    │  Code   │   RoomManager + Thread Safe │  Code   │    Mobile Browser     │
└───────────────────────┘         └─────────────────────────────┘         └───────────────────────┘
```

- **Backend**: Pure C and C++ (`RoomManager.h`, `Board.cpp`, `main.cpp`) with mutex-safe room tracking (`std::map<string, GameRoom>`), legal move generation, turn validation, and state serialization.
- **Frontend**: Responsive web client with touch support, dynamic board flipping, and real-time synchronization.

---

## 2. Quick Local & LAN Play (Phone + PC on same WiFi)

### Step 1: Start the C++ Server on PC
1. Run `build.bat` or compile with:
   ```cmd
   g++ -std=c++17 -O2 -o ChessVerse.exe backend\main.cpp backend\chess\Board.cpp -lws2_32
   ```
2. Start the server:
   ```cmd
   .\ChessVerse.exe
   ```
3. Find your PC's local IP address:
   - In Command Prompt / PowerShell, run: `ipconfig`
   - Look for **IPv4 Address** (e.g., `192.168.1.15`).

### Step 2: Connect from PC and Phone
1. **On your PC Browser**: Open `http://localhost:8080` (or `frontend/index.html`).
2. **On your Phone Browser**: Open `http://192.168.1.15:8080` (replace with your PC's IP).
3. **Play**:
   - PC clicks **Online Multiplayer** &rarr; **Create Match** &rarr; obtains 6-digit code (e.g. `K9X2P8`).
   - Phone clicks **Online Multiplayer** &rarr; **Join with Code** &rarr; enters `K9X2P8`.
   - The match starts instantly with live board synchronization, turn enforcement, and sound effects!

---

## 3. Free Online Deployment: Netlify (Frontend)

You can host the web interface on **Netlify** for free with a custom `.netlify.app` URL and automatic HTTPS.

### Step-by-Step Netlify Deployment:
1. Create a free account at [netlify.com](https://www.netlify.com).
2. Go to **Sites** &rarr; **Add new site** &rarr; **Deploy manually** (or connect your GitHub repository).
3. Drag and drop the `frontend` folder into the Netlify deploy drop zone.
4. Your site will instantly go live at `https://your-app-name.netlify.app`.
5. When opening on Netlify:
   - Click the ⚙️ **Server Settings** icon in the navbar or login screen.
   - Enter your public backend URL (see Section 4).

---

## 4. Free Online Backend Hosting (C/C++ Server)

Since the C++ backend is a lightweight native HTTP server, you can host it publicly for free using any of the following options:

### Option A: Free Public Tunnel (Instant — 0 setup)
If you want to play with friends over the internet while running the server on your PC:
1. Download [ngrok](https://ngrok.com) or [localtunnel](https://localtunnel.github.io/www/).
2. Run your `ChessVerse.exe`.
3. In terminal run:
   ```bash
   ngrok http 8080
   ```
4. Copy the public URL (e.g., `https://abc1-23.ngrok-free.app`).
5. Open your Netlify site or phone browser, click ⚙️ **Server Settings**, paste the URL, and start playing!

### Option B: Free Cloud Container (Render.com / Fly.io / Railway)
The C++ backend is written to compile natively on Linux with standard POSIX sockets (`#ifdef _WIN32` vs `#else` POSIX standard).
1. Create a simple `Dockerfile` in the root:
   ```dockerfile
   FROM gcc:latest
   WORKDIR /app
   COPY . .
   RUN g++ -std=c++17 -O2 -o server backend/main.cpp backend/chess/Board.cpp -lpthread
   EXPOSE 8080
   CMD ["./server"]
   ```
2. Push your project to GitHub.
3. On [Render.com](https://render.com), create a free **Web Service** from your GitHub repo.
4. Render will provide a free permanent HTTPS backend URL (e.g. `https://chessverse-api.onrender.com`).

---

## 5. Database & Storage Architecture (MongoDB / Binary Dat)

- **Local / Embedded**: Uses fast structured binary data files in `data/` (`users.dat`, `store.dat`, `inventory.dat`, `trivia_records.dat`) preserving DSA concepts (linked lists, file streams, byte serialization).
- **MongoDB Free Tier (Atlas)**:
  - For cloud persistence, a free MongoDB Atlas M0 cluster can store player profiles, match histories, and leaderboard rankings.
  - In C++, you can connect to MongoDB Atlas REST Data API via standard HTTP requests or the `mongocxx` driver.

---

## 6. Testing & Matchmaking Verification Checklist

- [x] **Match Creation**: Generating 6-digit uppercase codes (e.g. `M4Q8P2`).
- [x] **Match Joining**: Second player joins with valid code.
- [x] **Color Assignment & Orientation**: White moves first, board automatically flips perspective for Black.
- [x] **Turn Enforcement**: White cannot move Black pieces; players cannot move on opponent's turn.
- [x] **Cross-Platform Responsive UI**: Fits seamlessly on mobile touchscreens (`max-width: 600px`) and PC monitors.
- [x] **Real-time Live Sync**: Board state, captures, checks, checkmate, stalemate, and rematch.
