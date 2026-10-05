@echo off
echo Setting up Emscripten environment...
call emsdk\emsdk_env.bat

echo Downloading Raylib source for web build...
if not exist "raylib" (
    git clone https://github.com/raysan5/raylib.git
)

echo Building Raylib for WebAssembly...
cd raylib\src
call emmake make PLATFORM=PLATFORM_WEB -B
cd ..\..

echo Building ChessVerse2D for Web...
call em++ main_gui.cpp chess/board.cpp dsa/stack.c dsa/queue.c dsa/linkedlist.c dsa/sort.c -o index.html -I raylib/src raylib/src/libraylib.web.a -s USE_GLFW=3 -s ASYNCIFY -O3 -s EXPORTED_RUNTIME_METHODS=["ccall","cwrap"] -s EXPORTED_FUNCTIONS=["_main","_onRoomCreated","_onRoomJoined","_onOpponentJoined","_onOpponentDisconnected","_applyOpponentMove","_onWsError"] --preload-file assets --shell-file raylib/src/minshell.html -s ALLOW_MEMORY_GROWTH=1 -D_DEFAULT_SOURCE -Wno-missing-braces -Wunused-result -Os

echo Build complete! You can run it locally with:
echo python -m http.server 8080
