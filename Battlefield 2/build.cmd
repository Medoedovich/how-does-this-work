@echo off
setlocal
chcp 65001 >nul
set "LAB_ROOT=%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Visual Studio C++ tools were not found.
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "LAB_VS=%%i"
if not defined LAB_VS exit /b 1
call "%LAB_VS%\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
if not defined BF2_LAB_BUILD_DIR set "BF2_LAB_BUILD_DIR=%TEMP%\openbf2-vector-lab-%RANDOM%-%RANDOM%"
if not exist "%BF2_LAB_BUILD_DIR%" mkdir "%BF2_LAB_BUILD_DIR%"
pushd "%BF2_LAB_BUILD_DIR%"
cl /nologo /std:c++14 /EHsc /utf-8 /W4 /O2 /MT /DUNICODE /D_UNICODE /DDEDICATED=0 /FI"%LAB_ROOT%original\BuildConfig.h" "%LAB_ROOT%renderer.cpp" "%LAB_ROOT%original\Vec3.cpp" /Fe:"%LAB_ROOT%VectorLab.exe" /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib shell32.lib
if errorlevel 1 (
    popd
    exit /b 1
)
cl /nologo /std:c++14 /EHsc /utf-8 /W4 /O2 /MT /DDEDICATED=0 /FI"%LAB_ROOT%original\BuildConfig.h" "%LAB_ROOT%checks.cpp" "%LAB_ROOT%original\Vec3.cpp" /Fe:"%LAB_ROOT%VectorChecks.exe"
if errorlevel 1 (
    popd
    exit /b 1
)
popd
"%LAB_ROOT%VectorChecks.exe"
exit /b %errorlevel%
