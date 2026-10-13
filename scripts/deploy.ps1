# Envía build/boot.dol a la Wii por WiFi (el Homebrew Channel tiene que estar abierto en la Wii).
# IP por defecto: la de la Wii en casa (puede cambiar si el router le da otra; se ve con HOME en el HBC).
param([string]$Dol = "", [string]$Ip = "192.168.1.155")
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dol) { $Dol = Join-Path $root "build\boot.dol" }
$env:WIILOAD = "tcp:$Ip"
& "C:\devkitPro\tools\bin\wiiload.exe" $Dol
