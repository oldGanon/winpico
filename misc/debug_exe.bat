@echo off

cd %~dp0
cd ..

pushd build
devenv exe_packer.exe
popd build