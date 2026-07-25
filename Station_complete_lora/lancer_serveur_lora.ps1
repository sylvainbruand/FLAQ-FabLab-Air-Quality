# Script PowerShell de lancement du Serveur EASE LoRa
Set-Location -Path $PSScriptRoot

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "  Lancement du Serveur Python - Station EASE LoRa   " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Dossier actuel : $PSScriptRoot" -ForegroundColor Gray
Write-Host "Lancement de data_lora.py..." -ForegroundColor Yellow
Write-Host ""

python data_lora.py

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[ERREUR] Le serveur s'est arrete avec un code d'erreur." -ForegroundColor Red
}

Write-Host ""
Read-Host -Prompt "Appuyez sur Entree pour fermer cette fenetre"
