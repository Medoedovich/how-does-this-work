@echo off
if not exist "%~dp0VectorLab.exe" (
    call "%~dp0build.cmd"
    if errorlevel 1 (
        pause
        exit /b 1
    )
)
start "" "%~dp0VectorLab.exe"
