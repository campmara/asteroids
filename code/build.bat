@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

set WARNINGS=-WX -W4 -wd4100 -wd4189 -wd4201 -wd4505
set DEFINES=-DASTEROIDS_DEBUG=1 -DASSERTIONS_ENABLED=1 -DASTEROIDS_WIN32=1
set OPTIMIZATIONS=-MTd -nologo -Gm- -GR- -EHa -Od -Oi -FC -Z7
set LINK=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

del *.pdb > NUL 2> NUL
del *.rdi > NUL 2> NUL
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\asteroids.cpp -LD /link -incremental:no -opt:ref /PDB:handmade_%RANDOM%.pdb /EXPORT:GameUpdateAndRender
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\win32_asteroids.cpp /link -subsystem:windows %LINK%

popd
