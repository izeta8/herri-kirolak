# Compila el juego con devkitPro (make dentro del MSYS2 de devkitPro).
# Uso: scripts/build.ps1 [objetivo make, p.ej. clean]
param([string]$Target = "")
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$bash = "C:\devkitPro\msys2\usr\bin\bash.exe"
if (-not (Test-Path $bash)) { throw "No encuentro devkitPro en C:\devkitPro" }
$unixRoot = "/" + $root.Substring(0,1).ToLower() + ($root.Substring(2) -replace '\\','/')
$env:MSYSTEM = "MSYS"
$env:DEVKITPRO = "/opt/devkitpro"
$env:DEVKITPPC = "/opt/devkitpro/devkitPPC"
& $bash -lc "cd '$unixRoot' && make -j8 $Target"
if ($LASTEXITCODE -ne 0) { throw "La compilación ha fallado ($LASTEXITCODE)" }
if (-not $Target) { Write-Host "OK -> $root\build\boot.dol" }
