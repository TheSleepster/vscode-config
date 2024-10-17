@echo off

REM THE SYNTAX FOR DISABLING A WARNING IS -wd<error-number> EX: -wd4201
REM -O2 for release builds

Set CommonCompilerFlags=-MD -GR- -EHa- -Oi -Zi -W4 -wd4189 -wd4200 -wd4996 -wd4706 -wd4530
Set CommonLinkerFlags="..\data\deps\Raylib\lib\raylib.lib" opengl32.lib kernel32.lib user32.lib shell32.lib gdi32.lib winmm.lib msvcrt.lib
Set RaylibIncludePath="..\data\deps\Raylib\include"

REM 64-bit building

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl %CommonCompilerFlags% \Clone\code\*.cpp /I%RaylibIncludePath% /link /MACHINE:X64 /OUT:"Clone.exe" %CommonLinkerFlags%
popd

@echo ====================
@echo Compilation Finished
@echo ====================