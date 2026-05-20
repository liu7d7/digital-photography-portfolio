@echo off
call build.bat

copy cmake-build-release\index.html site\index.html
copy cmake-build-release\index.js site\index.js
copy cmake-build-release\index.wasm site\index.wasm

cd site
git add . && git commit -m "updates" && git push
