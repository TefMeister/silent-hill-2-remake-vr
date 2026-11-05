@echo off
rem Takes the Silent Hill 2 VR mod out again: every file it added is removed, every file it replaced is put back.
cd /d "%~dp0"
SH2VR.exe uninstall
pause
