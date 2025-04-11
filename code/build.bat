@echo off

:: Make the build folder.

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

:: Build sound (convert .wav files to .ogg)

IF NOT EXIST .\sounds mkdir .\sounds
set FFMPEG_OPTIONS=-hide_banner -loglevel warning -channel_layout mono -y
for %%f in (..\data\sounds\*.wav) do (
    ffmpeg %FFMPEG_OPTIONS% -i "%%f" -acodec libvorbis "sounds/%%~nf.ogg"
)

:: Build the game.

set WARNINGS=-WX -W4 -wd4100 -wd4189 -wd4201 -wd4505
set DEFINES=-DASTEROIDS_DEBUG=1 -DASSERTIONS_ENABLED=1 -DASTEROIDS_WIN32=1
set LINK=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

:: set OPTIMIZATIONS=-MTd -nologo -Gm- -GR- -EHa -O2 -Oi -FC -Z7
set OPTIMIZATIONS=-MTd -nologo -Gm- -GR- -EHa -Od -Oi -FC -Z7

del *.pdb > NUL 2> NUL
del *.rdi > NUL 2> NUL
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\asteroids.cpp -LD /link -incremental:no -opt:ref /PDB:handmade_%RANDOM%.pdb /EXPORT:GameUpdateAndRender
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\win32_asteroids.cpp /link -subsystem:windows %LINK%

popd
