@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -FC -Od

cd %~dp0
cd ..

IF NOT EXIST build mkdir build
pushd build

REM 64-bit build
cl %CommonCompilerFlags% ..\code\sfx_packer.cpp /link -subsystem:console

popd