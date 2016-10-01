@echo off
setlocal enableextensions

echo [Specular generator from IOR]
echo.
echo For dielectric materials only!
echo.
set /p ior=Index of refraction:

SpecularGeneratorIOR.exe %ior%

echo.
pause