@echo off

REM -Od disable Optimization
REM -O2 Optimization (Release Mode)
set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -O2 -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -DGAME_DEBUG=1 -FC -Z7
set CommonLinkerFlags=-opt:ref /LIBPATH:"..\lib" picolua.lib user32.lib gdi32.lib winmm.lib icon.res

cd %~dp0
cd ..

IF NOT EXIST build mkdir build
pushd build

IF EXIST icon.res goto skiprc
rc /r -nologo ..\misc\icon.rc
robocopy "..\misc" "..\build" icon.res /njh /njs
:skiprc

REM 64-bit build
cl %CommonCompilerFlags% ..\code\win32.cpp -Fmwin32_game.map /I..\include /link -subsystem:windows %CommonLinkerFlags%

popd