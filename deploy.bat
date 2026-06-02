@echo off
call build.bat

copy cmake-build-release\index.html site\index.html
copy cmake-build-release\index.js site\index.js
copy cmake-build-release\index.wasm site\index.wasm

copy cmake-build-release\0.webp site\0.webp
copy cmake-build-release\1.webp site\1.webp
copy cmake-build-release\2.webp site\2.webp
copy cmake-build-release\3.webp site\3.webp
copy cmake-build-release\4.webp site\4.webp
copy cmake-build-release\5.webp site\5.webp
copy cmake-build-release\6.webp site\6.webp
copy cmake-build-release\7.webp site\7.webp
copy cmake-build-release\8.webp site\8.webp
copy cmake-build-release\9.webp site\9.webp
copy cmake-build-release\10.webp site\10.webp
copy cmake-build-release\11.webp site\11.webp
copy cmake-build-release\12.webp site\12.webp
copy cmake-build-release\13.webp site\13.webp
copy cmake-build-release\14.jpg site\14.jpg

copy cmake-build-release\fdb.otf site\fdb.otf
copy cmake-build-release\fdl.otf site\fdl.otf

copy cmake-build-release\faune.png site\faune.png
copy cmake-build-release\faune.dat site\faune.dat

pushd site
git add . && git commit -m "updates" && git push
popd
