@echo off
chcp 65001 > nul
set PYTHONUTF8=1
title Serveur EASE Wi-Fi
cd /d "%~dp0"
if not exist "server_secrets.bat" (
    echo [ERREUR] server_secrets.bat est introuvable.
    echo Copiez server_secrets.bat.example puis utilisez le meme jeton dans arduino_secrets.h.
    pause
    exit /b 1
)
call "server_secrets.bat"
python data_wifi.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] Le serveur s'est arrêté ou Python n'est pas accessible.
)
pause
