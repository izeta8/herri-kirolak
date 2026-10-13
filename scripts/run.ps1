# Abre build/boot.dol en Dolphin (cierra antes cualquier Dolphin abierto).
param([string]$Dol = "")
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dol) { $Dol = Join-Path $root "build\boot.dol" }
$dolphin = "D:\Tools\Dolphin\Dolphin-x64\Dolphin.exe"
Get-Process Dolphin -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
Start-Process $dolphin -ArgumentList @("-b", "-e", "`"$Dol`"")
Write-Host "Dolphin lanzado con $Dol"
