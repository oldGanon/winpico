@echo off

REM -Od disable Optimization
REM -O2 Optimization (Release Mode)
set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -W4 -wd4201 -wd4100 -wd4189 -wd4505 -FC -Z7
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
cl -DEXE_SIZE=0 %CommonCompilerFlags% ..\code\win32.cpp -Fmwin32.map /I..\include /link -subsystem:windows %CommonLinkerFlags%
call :getfilesize win32.exe
cl -DEXE_SIZE=%filesize% %CommonCompilerFlags% ..\code\win32.cpp -Fmwin32.map /I..\include /link -subsystem:windows %CommonLinkerFlags%

popd
call pack
goto :eof

:getfilesize
set filesize=%~z1
goto :eof
