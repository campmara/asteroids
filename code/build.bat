@echo off

:: Make the build folder.

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

:: Build sound (convert .wav files to .ogg)

::IF NOT EXIST .\sounds mkdir .\sounds
::set FFMPEG_OPTIONS=-hide_banner -loglevel warning -channel_layout stereo -y
::for %%f in (..\data\sounds\*.wav) do (
::    ffmpeg %FFMPEG_OPTIONS% -i "%%f" -acodec libvorbis "sounds/%%~nf.ogg"
::)

:: Setup config variables.
set WARNINGS=-WX -W4 -wd4100 -wd4189 -wd4201 -wd4505
set DEFINES=-DASTEROIDS_DEBUG=1 -DASSERTIONS_ENABLED=1 -DASTEROIDS_WIN32=1
set LINK_PLATFORM=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib
set LINK_GAME=-incremental:no -opt:ref stb_vorbis.lib /PDB:handmade_%RANDOM%.pdb /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundSamples

:: set OPTIMIZATIONS=-O2 -MTd -nologo -Gm- -GR- -EHa -Oi -FC -Z7
set OPTIMIZATIONS=-Od -MTd -nologo -Gm- -GR- -EHa -Oi -FC -Z7

:: Delete previously-built .pdb and .rdi files.
del *.pdb > NUL 2> NUL
del *.rdi > NUL 2> NUL

:: Compile stb_vorbis.
::call cl %OPTIMIZATIONS% -W0 ..\code\include\stb_vorbis.c -c
::call lib stb_vorbis.obj

:: Compile game and platform.
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\asteroids.cpp -LD /link %LINK_GAME%
call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\code\win32_asteroids.cpp /link -subsystem:windows %LINK_PLATFORM%

popd
