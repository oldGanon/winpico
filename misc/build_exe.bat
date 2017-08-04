@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -FC -O2

cd %~dp0
cd ..

IF NOT EXIST build mkdir build
pushd build

REM 64-bit build
cl %CommonCompilerFlags% ..\code\exe_packer.cpp /I..\include /link -subsystem:console /LIBPATH:"..\lib" picolua.lib

popd