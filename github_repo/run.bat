@echo off
REM ═══════════════════════════════════════════════════════════════
REM  ChessVerse Runner
REM  Starts the backend server and opens the frontend
REM ═══════════════════════════════════════════════════════════════

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║     Starting ChessVerse...                ║
echo  ╚═══════════════════════════════════════════╝
echo.

if not exist ChessVerse.exe (
    echo [ERROR] ChessVerse.exe not found. Run build.bat first!
    pause
    exit /b 1
)

REM Create data directory
if not exist data mkdir data

echo [1/2] Starting backend server on localhost:8080...
start "" ChessVerse.exe

REM Wait a moment for server to start
timeout /t 2 /nobreak >nul

echo [2/2] Opening frontend in browser...
start "" "frontend\index.html"

echo.
echo  ChessVerse is running!
echo  - Backend: http://localhost:8080
echo  - Frontend: opened in browser
echo  - Close this window to stop the server
echo.
echo  Login Credentials:
echo    Player: swastik / swas123
echo    Player: arnav / arv123
echo    Admin:  admin / admin123
echo.
pause
