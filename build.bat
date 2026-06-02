@echo off

pushd c:\dev\port

if not exist .\cmake-build-release (mkdir .\cmake-build-release)

pushd cmake-build-release
call emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_C_FLAGS="-isystem C:\dev\emsdk\upstream\emscripten\cache\sysroot\include" -G "Ninja"
copy compile_commands.json ..\compile_commands.json
ninja
popd

if %errorlevel% EQU 0 echo build success

popd
