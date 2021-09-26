@echo off

rem -FAs assembly output
set CompilerFlags=/I "..\include" -nologo -MT -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4127 -wd4201 -wd4281 -wd4100 -wd4189 -wd4505 -wd4706 -DDEBUG=1 -FC -Z7
set LinkerFlags=/LIBPATH:"..\lib" -incremental:no opengl32.lib glew32s.lib 

mkdir ..\build
pushd ..\build

REM 64-bit build
del *.pdb > NUL 2> NUL
cl %CompilerFlags% ..\src\win32_main.cpp -Fmmain.map /link -opt:ref user32.lib gdi32.lib winmm.lib %LinkerFlags%
popd 
