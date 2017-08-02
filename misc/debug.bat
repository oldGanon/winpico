@echo off

cd %~dp0
cd ..

pushd build
devenv win32.exe
popd build