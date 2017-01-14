@echo off

echo [Specular generator]
echo.
echo Make sure you use file from:
echo http://www.filmetrics.com/refractive-index-database/
echo.
if "%1%"=="" (
echo Need txt file as paramert! 
)else (
echo Generating for %1% :
echo.
SpecularGenerator.exe %1
)
echo.
pause