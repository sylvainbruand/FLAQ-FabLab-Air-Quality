@echo off
chcp 65001 > nul
set "PYTHONUTF8=1"
set "EASE_OPEN_BROWSER=1"
title Serveur EASE LoRa

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
echo   Lancement du Serveur Python - Station EASE LoRa
echo ====================================================
echo.

:: Navigation automatique vers le dossier du script (gere le '!' et les espaces)
cd /d "%~dp0"

echo Dossier actuel : %CD%
echo Lancement de data_lora.py...
echo.

set "PYTHON_EXE=%~dp0.venv\Scripts\python.exe"

if not exist "%PYTHON_EXE%" (
    where py >nul 2>&1
    if not errorlevel 1 (
        py -3 data_lora.py
        goto :after_run
    )

    where python >nul 2>&1
    if not errorlevel 1 (
        python data_lora.py
        goto :after_run
    )

    echo [ERREUR] Aucun environnement Python utilisable n'a ete trouve.
    echo Reinstallez les dependances avec :
    echo   py -3 -m venv .venv
    echo   .venv\Scripts\python.exe -m pip install -r requirements.txt
    goto :end
)

"%PYTHON_EXE%" data_lora.py

:after_run
if errorlevel 1 (
    echo.
    echo [ERREUR] Le serveur s'est arrete avec une erreur.
)

:end
echo.
pause
