@echo off

cd %~dp0
cd ..

pushd build
devenv lemonhunter.exe
popd build