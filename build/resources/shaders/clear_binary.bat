@echo off
Setlocal enabledelayedexpansion

color 87
echo [Shaders binary delete]
echo.
echo.

For /r %%I In (*.bc) Do (
	del %%I
	echo Delete %%I
)

For /r %%I In (*.tq) Do (
	del %%I
	echo Delete %%I
)

echo.
echo.
color 07

pause&exit