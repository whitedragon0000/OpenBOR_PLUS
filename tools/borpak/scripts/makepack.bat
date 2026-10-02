@echo off

rem ============================================================
rem OpenBOR PAK Builder
rem
rem This script builds a .pak file from the "data" folder.
rem
rem To use:
rem   1. Put this .bat file in the same folder as borpak.exe.
rem   2. Make sure your project files are inside a folder named "data".
rem   3. Change PROJECT_NAME below to your project name.
rem   4. Double-click this file to build the .pak.
rem ============================================================

rem Change this to the name you want for your pak file.
rem Example: set PROJECT_NAME=Final_Fight_Apocalypse
set PROJECT_NAME=my_project

rem This is the folder Borpak will pack.
rem Most OpenBOR projects use "data".
set DATA_FOLDER=data

rem This is the final output file.
set OUTPUT_FILE=%PROJECT_NAME%.pak

echo.
echo OpenBOR PAK Builder
echo -------------------
echo Project name: %PROJECT_NAME%
echo Data folder:  %DATA_FOLDER%
echo Output file:  %OUTPUT_FILE%
echo.

rem Make sure borpak.exe is next to this batch file.
if not exist "borpak.exe" (
    echo Error: borpak.exe was not found.
    echo.
    echo Put this .bat file in the same folder as borpak.exe.
    pause
    exit /b 1
)

rem Make sure the data folder exists before trying to build.
if not exist "%DATA_FOLDER%\" (
    echo Error: The "%DATA_FOLDER%" folder was not found.
    echo.
    echo Create a folder named "%DATA_FOLDER%" and put your OpenBOR project files inside it.
    pause
    exit /b 1
)

rem Warn before overwriting an existing pak file.
if exist "%OUTPUT_FILE%" (
    echo Warning: "%OUTPUT_FILE%" already exists.
    echo Building again will overwrite it.
    echo.
    choice /m "Do you want to continue"

    if errorlevel 2 (
        echo.
        echo Build cancelled.
        pause
        exit /b 0
    )
)

echo.
echo Building pak file...
echo.

borpak.exe -b -d "%DATA_FOLDER%" "%OUTPUT_FILE%"

if errorlevel 1 (
    echo.
    echo Build failed.
    echo.
    echo Check the error message above for details.
    pause
    exit /b 1
)

echo.
echo Build complete: %OUTPUT_FILE%
echo.
pause
