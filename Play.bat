@echo off
cd /d "%~dp0"
powershell -ExecutionPolicy Bypass -File ".\Scripts\Play.ps1"
pause
