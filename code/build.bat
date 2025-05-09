@echo off

:: Set the following variables to 1 to build specific platforms / libraries.
set BUILD_WIN32=0
set BUILD_SDL3=1
set BUILD_STB_VORBIS=0

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
set LINK_PLATFORM=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib ole32.lib
set LINK_GAME=-incremental:no -opt:ref stb_vorbis.lib /PDB:handmade_%RANDOM%.pdb /EXPORT:GameUpdateAndRender

:: set OPTIMIZATIONS=-O2 -MTd -nologo -Gm- -GR- -EHa -Oi -FC -Z7
set OPTIMIZATIONS=-Od -MTd -nologo -Gm- -GR- -EHa -Oi -FC -Z7

:: Compile stb_vorbis.
IF %BUILD_STB_VORBIS% EQU 1 (
    call cl %OPTIMIZATIONS% -W0 ..\code\include\stb_vorbis.c -c
    call lib stb_vorbis.obj
)

:: WIN32 BUILD
IF %BUILD_WIN32% EQU 1 (
    IF NOT EXIST win32 mkdir win32
    pushd win32
    del *.pdb > NUL 2> NUL
    del *.rdi > NUL 2> NUL
    call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\..\code\asteroids.cpp -LD /link %LINK_GAME%
    call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\..\code\win32_asteroids.cpp /link -subsystem:windows %LINK_PLATFORM%
    popd
)

:: SDL BUILD
set INCLUDES=/I "C:\work\cpp\third_party\SDL-release-3.2.10\include" /I "C:\work\cpp\third_party\openal-soft-1.24.3\include"
IF %BUILD_SDL3% EQU 1 (
    IF NOT EXIST sdl3 mkdir sdl3
    pushd sdl3
    del *.pdb > NUL 2> NUL
    del *.rdi > NUL 2> NUL
    call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% ..\..\code\asteroids.cpp -LD /link %LINK_GAME%
    call cl %WARNINGS% %DEFINES% %OPTIMIZATIONS% %INCLUDES% ..\..\code\sdl3_asteroids.cpp /link -incremental:no -opt:ref SDL3.lib
    popd
)

popd
