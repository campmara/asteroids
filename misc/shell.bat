@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=c:\work\cpp\learning\exercises\asteroids\misc;%path%
cd "C:\work\cpp\learning\exercises\asteroids\"
call cls