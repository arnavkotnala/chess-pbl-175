@echo off
echo Downloading Emscripten SDK...
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
call emsdk install latest
call emsdk activate latest
echo Emscripten SDK installed.
echo To use it in this session, run: emsdk_env.bat
