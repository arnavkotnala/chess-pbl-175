# ♔ ChessVerse — Cross-Platform Online Multiplayer Chess

A full-stack online chess platform built with a **Pure C/C++ Multi-Room Game Server** and a **Responsive Cross-Platform Web Client** (supporting Phone and PC).

---

## 🌟 Key Features

- **🌐 Online Multiplayer Matchmaking**: Create or join match rooms using unique 6-digit codes (e.g. `ZCNM42`).
- **📱 Cross-Platform (Phone & PC)**: Fully responsive UI with touch optimization and board flipping for Black perspective.
- **🤖 Single-Player AI**: Easy, Medium, and Hard AI with Minimax alpha-beta search.
- **👥 Local Versus**: Hot-seat 2-player mode on the same device.
- **🎯 Daily Trivia & 🛒 Skins Store**: Points progression, board/piece skins, EXP ranking tiers, and leaderboard.
- **⚡ Pure C/C++ Game Engine**: Thread-safe room management, legal move generation, move validation, and DSA data structures.

---

## 📁 Repository Structure

```
├── backend/                  # C/C++ Game Server & Engine
│   ├── chess/                # Board, AI, HintEngine, MoveQueue, RoomManager
│   ├── auth/                 # User Authentication & Rank Manager
│   ├── store/                # Store & Inventory Management
│   ├── trivia/               # Trivia Engine
│   └── main.cpp              # Cross-platform HTTP Server & API Router
│
├── frontend/                 # Cross-Platform Web UI (Deployable to Netlify)
│   ├── css/styles.css        # Responsive mobile & PC stylesheet
│   ├── js/chess.js           # Client logic, online polling, board rendering
│   ├── js/sounds.js          # Audio engine
│   ├── assets/               # Icons & media assets
│   └── index.html            # Main web interface
│
├── data/                     # Embedded binary data files (.dat)
├── build.bat                 # Build script for Windows (g++ / MSVC)
├── DEPLOYMENT_GUIDE.md       # Step-by-step free-tier deployment guide
└── .gitignore
```

---

## 🚀 Quick Start

### 1. Compile and Run the C++ Backend
```bash
# Windows
build.bat
# or with g++ directly:
g++ -std=c++17 -O2 -o ChessVerse.exe backend/main.cpp backend/chess/Board.cpp -lws2_32

# Linux / Mac
g++ -std=c++17 -O2 -o server backend/main.cpp backend/chess/Board.cpp -lpthread
./server
```

### 2. Open the Web Frontend
- Simply open `frontend/index.html` in any browser on Phone or PC.
- Or deploy `frontend/` directly to **Netlify** or **GitHub Pages**.
