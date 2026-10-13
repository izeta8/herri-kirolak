# Envía build/boot.dol a la Wii por red (Homebrew Channel abierto en la Wii).
# Necesita $env:WIILOAD = "tcp:<IP de la Wii>"
$root = Split-Path $PSScriptRoot -Parent
if (-not $env:WIILOAD) { throw 'Define antes la IP: $env:WIILOAD = "tcp:192.168.1.XX"' }
& "C:\devkitPro\tools\bin\wiiload.exe" (Join-Path $root "build\boot.dol")
