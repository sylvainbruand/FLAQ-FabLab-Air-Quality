# Script PowerShell de lancement du Serveur EASE LoRa
Set-Location -Path $PSScriptRoot
$env:EASE_OPEN_BROWSER = "1"

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "  Lancement du Serveur Python - Station EASE LoRa   " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Dossier actuel : $PSScriptRoot" -ForegroundColor Gray
Write-Host "Lancement de data_lora.py..." -ForegroundColor Yellow
Write-Host ""

$localPython = Join-Path $PSScriptRoot ".venv\Scripts\python.exe"

if (Test-Path -LiteralPath $localPython) {
    & $localPython "data_lora.py"
}
elseif (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 "data_lora.py"
}
elseif (Get-Command python -ErrorAction SilentlyContinue) {
    & python "data_lora.py"
}
else {
    Write-Host "[ERREUR] Aucun environnement Python utilisable n'a ete trouve." -ForegroundColor Red
    Write-Host "Reinstallez les dependances avec :" -ForegroundColor Yellow
    Write-Host "  py -3 -m venv .venv"
    Write-Host "  .venv\Scripts\python.exe -m pip install -r requirements.txt"
    Write-Host ""
    Read-Host -Prompt "Appuyez sur Entree pour fermer cette fenetre"
    exit 1
}

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[ERREUR] Le serveur s'est arrete avec un code d'erreur." -ForegroundColor Red
}

Write-Host ""
Read-Host -Prompt "Appuyez sur Entree pour fermer cette fenetre"
