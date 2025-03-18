@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

set WARNINGS=-WX -W4 -wd4100
set DEFINES=-DASTEROIDS_DEBUG=1 -DASSERTIONS_ENABLED=1 -DASTEROIDS_WIN32=1
set OPTIMIZATIONS=-MTd -nologo -Gm- -GR- -EHa -Od -Oi -FC -Z7
set LINK=-incremental:no -opt:ref user32.lib

call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\win32_asteroids.cpp /link -subsystem:windows %LINK%

popd
