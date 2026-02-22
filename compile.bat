@echo off
if not exist "%~dp0build_output" mkdir "%~dp0build_output"
del /q "%~dp0source\obj\x64\Development\vc143.pdb" 2>nul
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "%~dp0Engine.sln" /p:Configuration=Development /p:Platform=x64 /m:1 /v:minimal > "%~dp0build_output\build_output.txt" 2>&1
type "%~dp0build_output\build_output.txt"
pause
