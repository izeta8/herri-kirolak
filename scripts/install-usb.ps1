# Copia el juego al USB/SD de la Wii (FAT32) en <unidad>:\apps\herri-kirolak\
param([string]$Drive = "F")
$root = Split-Path $PSScriptRoot -Parent
$vol = Get-Volume -DriveLetter $Drive -ErrorAction Stop
if ($vol.FileSystem -ne "FAT32") { throw "La unidad ${Drive}: es $($vol.FileSystem); el Homebrew Channel necesita FAT32" }
$dst = "${Drive}:\apps\herri-kirolak"
New-Item -ItemType Directory -Force $dst | Out-Null
Copy-Item (Join-Path $root "build\boot.dol"), (Join-Path $root "hbc\herri-kirolak\*") $dst -Force
Get-ChildItem $dst | Select-Object Name, Length
