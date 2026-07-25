@echo off
chcp 65001 > nul
set PYTHONUTF8=1
title Serveur EASE LoRa

echo ====================================================
echo   Lancement du Serveur Python - Station EASE LoRa
echo ====================================================
echo.

:: Navigation automatique vers le dossier du script (gere le '!' et les espaces)
cd /d "%~dp0"

echo Dossier actuel : %CD%
echo Lancement de data_lora.py...
echo.

python data_lora.py

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] Le serveur s'est arrete ou Python n'est pas accessible.
)

echo.
pause
