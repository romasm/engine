@echo off
Setlocal enabledelayedexpansion

color 87
echo [Luac-files delete]
echo.
echo.

For /r ../resources/ %%I In (*.luac) Do (
	del %%I
	echo Delete %%I
)

echo.
echo.
color 07

pause&exit