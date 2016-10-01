@echo off
Setlocal enabledelayedexpansion

color 87
echo [Luac bytecode compiler]
echo.
echo.

For /r ../resources/ %%I In (*.lua) Do (
	set "fstr=%%I"
	if "!fstr:luaproj=!"=="!fstr!" (
		set "str=%%~dpnI"
		"luajit.exe" -b %%I "!str!.luac"
		echo Process "!str!.luac"
	)
)

echo.
echo.
color 07

pause&exit