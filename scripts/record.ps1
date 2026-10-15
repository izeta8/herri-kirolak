# Graba un .dol en Dolphin volcando cada fotograma (funciona aunque la pantalla del PC esté apagada o tapada) y
# hace un GIF animado en build/screenshots/<Name>.gif para mandarlo al móvil.
#   scripts/record.ps1 -Dol build-test/boot.dol -Seconds 75 -Name tutorial [-First 30] [-Step 4]
# Ojo: en este PC Dolphin tarda ~40 s en abrir la ventana; el script espera a que aparezca.
param([string]$Dol = "", [int]$Seconds = 30, [string]$Name = "grabacion", [int]$First = 30, [int]$Step = 4)
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dol) { $Dol = Join-Path $root "build\boot.dol" }
$cfg = "D:\Tools\Dolphin\Dolphin-x64\User\Config"
$dump = Join-Path $root ("build\dump-" + (Get-Date -Format "yyyyMMdd-HHmmss") + "\")
New-Item -ItemType Directory -Force $dump | Out-Null
Copy-Item "$cfg\Dolphin.ini" "$cfg\Dolphin.ini.antes-record" -Force
Copy-Item "$cfg\GFX.ini" "$cfg\GFX.ini.antes-record" -Force
try {
    $d = [IO.File]::ReadAllText("$cfg\Dolphin.ini") -replace "(?m)^DumpPath = .*\r?\n", ""
    $d = $d -replace '\[General\]', "[General]`r`nDumpPath = $dump"
    $d += "`r`n[Movie]`r`nDumpFrames = True`r`n"
    [IO.File]::WriteAllText("$cfg\Dolphin.ini", $d)
    $g = [IO.File]::ReadAllText("$cfg\GFX.ini")
    if ($g -match '\[Settings\]') { $g = $g -replace '\[Settings\]', "[Settings]`r`nDumpFramesAsImages = True" }
    else { $g += "`r`n[Settings]`r`nDumpFramesAsImages = True`r`n" }
    [IO.File]::WriteAllText("$cfg\GFX.ini", $g)
    & (Join-Path $PSScriptRoot "run.ps1") -Dol $Dol | Out-Null
    $sw = [Diagnostics.Stopwatch]::StartNew()
    do { $p = Get-Process Dolphin -ErrorAction SilentlyContinue; if ($p -and $p.MainWindowHandle -ne 0) { break }; [Threading.Thread]::Sleep(1000) } while ($sw.Elapsed.TotalSeconds -lt 120)
    $sw.Restart()
    do { [Threading.Thread]::Sleep(1000) } while ($sw.Elapsed.TotalSeconds -lt $Seconds)
    Get-Process Dolphin -ErrorAction SilentlyContinue | Stop-Process -Force
} finally {
    Copy-Item "$cfg\Dolphin.ini.antes-record" "$cfg\Dolphin.ini" -Force
    Copy-Item "$cfg\GFX.ini.antes-record" "$cfg\GFX.ini" -Force
}
$gif = Join-Path $root "build\screenshots\$Name.gif"
python -I (Join-Path $root "tools\frames_to_gif.py") $dump $gif $First -1 $Step
