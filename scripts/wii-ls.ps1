# Lista carpetas de la NAND de la Wii (con el juego abierto), solo las permitidas en updater.cpp (allowedPull).
# Uso: scripts/wii-ls.ps1 /shared1/ /title/00000001/00000002/content/
param([Parameter(Position = 0, ValueFromRemainingArguments = $true)][string[]]$Paths,
      [Parameter(Mandatory = $false)][string]$Ip = "")
$root = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $root "build\wii-ip.txt"
if (-not $Ip -and (Test-Path $cache)) { $Ip = (Get-Content $cache).Trim() }
if (-not $Ip) { throw "No sé la IP de la Wii: usa -Ip o manda antes el juego con deploy.ps1" }

foreach ($path in $Paths) {
    if (-not $path.EndsWith("/")) { $path += "/" }
    $c = $null
    for ($t = 0; $t -lt 8; $t++) {
        $c = New-Object Net.Sockets.TcpClient
        try { if ($c.ConnectAsync($Ip, 4301).Wait(3000)) { break } } catch {}
        Start-Sleep 3
    }
    if (-not $c.Connected) { Write-Host "$path : sin conexión"; continue }
    try {
        $s = $c.GetStream()
        $p = [Text.Encoding]::ASCII.GetBytes($path)
        $hdr = [byte[]](@([Text.Encoding]::ASCII.GetBytes("HKPL")) + @(0, 0, 0, $p.Length))
        $s.Write($hdr, 0, 8)
        $s.Write($p, 0, $p.Length)
        $s.ReadTimeout = 20000
        $h = New-Object byte[] 4
        $g = 0
        while ($g -lt 4) { $n = $s.Read($h, $g, 4 - $g); if ($n -le 0) { break }; $g += $n }
        $size = ([int]$h[0] -shl 24) -bor ([int]$h[1] -shl 16) -bor ([int]$h[2] -shl 8) -bor [int]$h[3]
        if ($size -le 0) { Write-Host "$path : vacía, no existe o no permitida" }
        else {
            $d = New-Object byte[] $size
            $g = 0
            while ($g -lt $size) { $n = $s.Read($d, $g, $size - $g); if ($n -le 0) { break }; $g += $n }
            Write-Host "$path :"
            ([Text.Encoding]::ASCII.GetString($d).Trim() -split "`n") | ForEach-Object { Write-Host "  $_" }
        }
    } catch { Write-Host "$path : error de red ($($_.Exception.Message))" }
    $c.Close()
    Start-Sleep 6   # la Wii atiende una conexión cada vez
}
