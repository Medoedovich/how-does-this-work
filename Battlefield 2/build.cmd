@echo off
setlocal
chcp 65001 >nul
echo Building lesson 01...
call "%~dp0Урок 01 — Движение и нормализация\build.cmd"
if errorlevel 1 exit /b 1
echo Building lesson 02...
call "%~dp0Урок 02 — Dot и поле зрения\build.cmd"
if errorlevel 1 exit /b 1
echo Building lesson 03...
call "%~dp0Урок 03 — Классы, здоровье и урон\build.cmd"
if errorlevel 1 exit /b 1
echo Building lesson 04...
call "%~dp0Урок 04 — Конструкторы, vector и циклы\build.cmd"
if errorlevel 1 exit /b 1
echo All lessons built and checked successfully.
exit /b 0
