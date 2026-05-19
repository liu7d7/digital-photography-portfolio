@echo off

pushd c:\dev\port

if not exist .\cmake-build-release (mkdir .\cmake-build-release)

pushd cmake-build-release
call emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -G "Ninja"
ninja
popd

if %errorlevel% EQU 0 echo build success

popd
