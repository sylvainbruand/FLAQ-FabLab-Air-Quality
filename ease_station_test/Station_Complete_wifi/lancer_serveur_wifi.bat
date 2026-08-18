@echo off
chcp 65001 > nul
set PYTHONUTF8=1
title Serveur EASE Wi-Fi

echo(
echo(             .----------------.
echo(          .-'              /   '-.
echo(        .'              / /       '.
echo(       /             / / /          \
echo(      ;          / / /               ;
echo(      ^|       / / /                  ^|
echo(      ^|    / / /                     ^|
echo(      ; _/_/_/                        ;
echo(       /                            /
echo(        '.                        .'
echo(          '-.__________________.-'
echo(
echo(        _________   _____ ______
echo(       / ____/   ^| / ___// ____/
echo(      / __/ / /^| ^| \__ \/ __/
echo(     / /___/ ___ ^|___/ / /___
echo(    /_____/_/  ^|_/____/_____/
echo(
echo ====================================================
echo   Lancement du Serveur Python - Station EASE Wi-Fi
echo ====================================================
echo(

cd /d "%~dp0"
if not exist "server_secrets.bat" (
    echo [ERREUR] server_secrets.bat est introuvable.
    echo Copiez server_secrets.bat.example puis utilisez le meme jeton dans arduino_secrets.h.
    pause
    exit /b 1
)
call "server_secrets.bat"

set "PYTHON_EXE=%~dp0.venv\Scripts\python.exe"
if not exist "%PYTHON_EXE%" (
    echo [ERREUR] L'environnement Python local est introuvable.
    echo Reinstallez les dependances avec :
    echo   py -3 -m venv .venv
    echo   .venv\Scripts\python.exe -m pip install -r requirements.txt
    pause
    exit /b 1
)

"%PYTHON_EXE%" data_wifi.py
if errorlevel 1 (
    echo.
    echo [ERREUR] Le serveur s'est arrete avec une erreur.
)
pause
