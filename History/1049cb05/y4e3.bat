@echo off

Set CommonCompilerFlags=-MT -GR- -EHa- -Oi -Zi -W4 -wd4189 -wd4200 -wd4996 -wd4706 -wd4530 -wd4100
Set CommonLinkerFlags=-incremental:no kernel32.lib user32.lib gdi32.lib opengl32.lib
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
del *.pdb
cl -DSUGAR_SLOW=1 ..\code\win32_Sugar.cpp %CommonCompilerFlags% -link %CommonLinkerFlags% -OUT:"Sugar.exe" 
cl -DSUGAR_SLOW=1 ..\code\Sugar.cpp %CommonCompilerFlags% -LD -link -incremental:no -opt:ref /PDB:Sugar_%RANDOM%%RANDOM%.pdb -EXPORT:GameUpdateAndRender -OUT:"GameCode.dll" 
popd

@echo ====================
@echo Compilation Complete
@echo ====================
