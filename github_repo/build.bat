@echo off
REM ═══════════════════════════════════════════════════════════════
REM  ChessVerse Build Script (Windows)
REM  Compiles the C++ backend into ChessVerse.exe
REM ═══════════════════════════════════════════════════════════════

echo.
echo  ╔═══════════════════════════════════════════╗
echo  ║   ChessVerse Build System                 ║
echo  ╚═══════════════════════════════════════════╝
echo.

REM Try g++ first (MinGW)
where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [BUILD] Using g++ (MinGW)...
    g++ -std=c++17 -O2 -o ChessVerse.exe backend\main.cpp backend\chess\Board.cpp -lws2_32
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Build successful: ChessVerse.exe
        goto :done
    ) else (
        echo [ERROR] g++ build failed.
        goto :trymsvc
    )
)

:trymsvc
REM Try MSVC cl.exe
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [BUILD] Using MSVC cl.exe...
    cl /std:c++17 /O2 /EHsc /Fe:ChessVerse.exe backend\main.cpp backend\chess\Board.cpp ws2_32.lib
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Build successful: ChessVerse.exe
        goto :done
    ) else (
        echo [ERROR] MSVC build failed.
        goto :nocompiler
    )
)

:nocompiler
echo.
echo  [ERROR] No C++ compiler found!
echo  Please install one of:
echo    1. MinGW-w64 (g++)  - https://www.mingw-w64.org/
echo    2. Visual Studio Build Tools (cl.exe)
echo.
pause
exit /b 1

:done
echo.
echo  ═════════════════════════════════════════════
echo   To run ChessVerse:
echo     1. Run: ChessVerse.exe
echo     2. Open frontend\index.html in your browser
echo     3. Login with: swastik/swas123 or arnav/arv123
echo        Admin: admin/admin123
echo  ═════════════════════════════════════════════
echo.
pause
