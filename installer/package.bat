@echo off
setlocal enabledelayedexpansion

:: ============================================================
:: Clarity Packaging Script
:: Stages a release build for Inno Setup installer creation.
::
:: Usage:
::   package.bat [build_dir]
::
::   build_dir  Path to the CMake build directory containing
::              Clarity.exe in a Release subdirectory.
::              Defaults to ..\build
::
:: Prerequisites:
::   - Release build of Clarity completed
::   - Qt bin directory on PATH (for windeployqt)
::   - Inno Setup on PATH (iscc.exe) — only needed for
::     the final step; staging works without it
::
:: What this does:
::   1. Cleans/creates installer\staging\
::   2. Copies Clarity.exe into staging
::   3. Runs windeployqt to gather Qt runtime dependencies
::   4. Copies app data (bible.db, songs.json, translations)
::   5. Runs Inno Setup compiler to produce the installer
:: ============================================================

:: --- Configuration ---
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=..\build"

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "STAGING_DIR=%SCRIPT_DIR%staging"
:: Ninja/MinGW puts the exe directly in the build dir; MSVC uses a Release\ subfolder.
set "EXE_PATH=%BUILD_DIR%\Clarity.exe"
if exist "%BUILD_DIR%\Release\Clarity.exe" set "EXE_PATH=%BUILD_DIR%\Release\Clarity.exe"

:: --- Validate build exists ---
if not exist "%EXE_PATH%" (
    echo ERROR: Clarity.exe not found at %EXE_PATH%
    echo.
    echo Make sure you have built Clarity:
    echo   cmake --build "%BUILD_DIR%"
    echo.
    echo Or pass the build directory as an argument:
    echo   package.bat path\to\build
    exit /b 1
)

:: --- Verify windeployqt is available ---
where windeployqt >nul 2>&1
if errorlevel 1 (
    echo ERROR: windeployqt not found on PATH.
    echo Add your Qt bin directory to PATH, e.g.:
    echo   set PATH=C:\Qt\6.8.0\msvc2022_64\bin;%%PATH%%
    exit /b 1
)

:: --- Clean and create staging directory ---
echo Cleaning staging directory...
if exist "%STAGING_DIR%" rmdir /s /q "%STAGING_DIR%"
mkdir "%STAGING_DIR%"

:: --- Copy executable ---
echo Copying Clarity.exe...
copy /y "%EXE_PATH%" "%STAGING_DIR%\" >nul

:: --- Run windeployqt ---
echo Running windeployqt...
windeployqt ^
    --release ^
    --no-translations ^
    --qmldir "%PROJECT_DIR%\src" ^
    "%STAGING_DIR%\Clarity.exe"

if errorlevel 1 (
    echo ERROR: windeployqt failed.
    exit /b 1
)

:: --- Copy application data ---
echo Copying application data...

:: Bible database
mkdir "%STAGING_DIR%\data" 2>nul
copy /y "%PROJECT_DIR%\data\bible.db" "%STAGING_DIR%\data\" >nul

:: Sample songs library
mkdir "%STAGING_DIR%\config\Clarity" 2>nul
copy /y "%PROJECT_DIR%\samples\songs.json" "%STAGING_DIR%\config\Clarity\" >nul

:: Translations (.qm files from build output)
:: Translations may be in Release\ subfolder (MSVC) or directly in build dir (Ninja)
set "QM_DIR=%BUILD_DIR%\Release\translations"
if not exist "%QM_DIR%" set "QM_DIR=%BUILD_DIR%\translations"
if exist "%QM_DIR%\*.qm" (
    mkdir "%STAGING_DIR%\translations" 2>nul
    copy /y "%QM_DIR%\*.qm" "%STAGING_DIR%\translations\" >nul
    echo Copied translation files.
) else (
    echo WARNING: No .qm translation files found. Skipping translations.
)

:: --- Summary ---
echo.
echo Staging complete. Contents of %STAGING_DIR%:
dir /s /b "%STAGING_DIR%" | %SystemRoot%\System32\find.exe /c /v ""
echo files staged.
echo.

:: --- Build installer ---
where iscc >nul 2>&1
if errorlevel 1 (
    echo Inno Setup compiler ^(iscc.exe^) not found on PATH.
    echo Staging is ready — open installer\clarity.iss in Inno Setup to compile manually.
    echo.
    echo To add iscc to PATH:
    echo   set PATH=C:\Program Files ^(x86^)\Inno Setup 6;%%PATH%%
    exit /b 0
)

echo Building installer...
iscc "%SCRIPT_DIR%clarity.iss"

if errorlevel 1 (
    echo ERROR: Inno Setup compilation failed.
    exit /b 1
)

echo.
echo Done! Installer written to: installer\output\ClaritySetup-0.1.0-beta.1.exe
