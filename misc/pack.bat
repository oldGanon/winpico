@echo off

cd %~dp0
cd ..

IF NOT EXIST build mkdir build
pushd build

robocopy "..\data" "..\build" * /njh /njs /s /xx

call sfx_packer
call exe_packer

popd