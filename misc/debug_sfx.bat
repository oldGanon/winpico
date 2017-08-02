@echo off

cd %~dp0
cd ..

pushd build
devenv sfx_packer.exe
popd build