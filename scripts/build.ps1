# Compila el juego con devkitPro (make dentro del MSYS2 de devkitPro).
# Uso:
#   scripts/build.ps1                       -> build/boot.dol (versión para jugar)
#   scripts/build.ps1 -Test -Scene 6        -> build-test/boot.dol con golpes por botones (1/2/-) y escena inicial
#   scripts/build.ps1 -Target clean
param([string]$Target = "", [switch]$Test, [int]$Scene = 0)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$bash = "C:\devkitPro\msys2\usr\bin\bash.exe"
if (-not (Test-Path $bash)) { throw "No encuentro devkitPro en C:\devkitPro" }
$unixRoot = "/" + $root.Substring(0,1).ToLower() + ($root.Substring(2) -replace '\\','/')
$env:MSYSTEM = "MSYS"
$env:DEVKITPRO = "/opt/devkitpro"
$env:DEVKITPPC = "/opt/devkitpro/devkitPPC"
$outdir = "build"
$defs = ""
if ($Test) {
    $outdir = "build-test"
    $defs = "-DDEBUG_INPUT -DSTART_SCENE=$Scene"
    # Las macros cambian entre pruebas: recompilar siempre
    & $bash -lc "cd '$unixRoot' && rm -rf build-test/obj"
}
& $bash -lc "cd '$unixRoot' && make -j8 OUTDIR=$outdir DEFS='$defs' $Target 2>&1"
if ($LASTEXITCODE -ne 0) { throw "La compilación ha fallado ($LASTEXITCODE)" }
if (-not $Target) { Write-Host "OK -> $root\$outdir\boot.dol" }
