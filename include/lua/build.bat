call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
cd %~dp0
IF NOT EXIST build mkdir build
pushd build

cl /MT /O2 /c ..\*.c
lib /OUT:picolua.lib *.obj

popd
pause