# Copia de la Wii (con el juego abierto) a la NAND de Dolphin las piezas de los Miis y los Miis del Canal Mii:
#   /shared2/FaceLib/RFL_Res.dat       (modelos y texturas de las piezas)
#   /shared2/menu/FaceLib/RFL_DB.dat   (los Miis del usuario)
# Son ficheros de la consola del usuario: van a la NAND de Dolphin, NUNCA al repositorio.
# Antes: mandar a la Wii una versión con soporte de "HKPL" (scripts/deploy.ps1).
param([string]$Ip = "")
$root = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $root "build\wii-ip.txt"
if (-not $Ip -and (Test-Path $cache)) { $Ip = (Get-Content $cache).Trim() }
if (-not $Ip) { throw "No sé la IP de la Wii: usa -Ip o manda antes el juego con deploy.ps1" }
$nand = "D:\Tools\Dolphin\Dolphin-x64\User\Wii"

function Pull($path) {
    $c = New-Object Net.Sockets.TcpClient
    if (-not $c.ConnectAsync($Ip, 4301).Wait(3000)) { throw "La Wii ($Ip) no responde en el puerto 4301: ¿está el juego abierto?" }
    $s = $c.GetStream()
    $p = [Text.Encoding]::ASCII.GetBytes($path)
    $len = $p.Length
    $hdr = [byte[]](@([Text.Encoding]::ASCII.GetBytes("HKPL")) + @(0, 0, 0, $len))
    $s.Write($hdr, 0, 8)
    $s.Write($p, 0, $len)
    $s.ReadTimeout = 20000
    $h = New-Object byte[] 4
    $got = 0
    while ($got -lt 4) { $n = $s.Read($h, $got, 4 - $got); if ($n -le 0) { break }; $got += $n }
    $size = ([int]$h[0] -shl 24) -bor ([int]$h[1] -shl 16) -bor ([int]$h[2] -shl 8) -bor [int]$h[3]
    if ($size -le 0) { $c.Close(); throw "La Wii no pudo leer $path" }
    $data = New-Object byte[] $size
    $got = 0
    while ($got -lt $size) { $n = $s.Read($data, $got, $size - $got); if ($n -le 0) { break }; $got += $n }
    $c.Close()
    if ($got -ne $size) { throw "Llegaron $got de $size bytes de $path" }
    $dst = Join-Path $nand ($path.TrimStart('/') -replace '/', '\')
    New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
    [IO.File]::WriteAllBytes($dst, $data)
    Write-Host "OK $path -> $dst ($size bytes)"
}

Pull "/shared2/FaceLib/RFL_Res.dat"
Pull "/shared2/menu/FaceLib/RFL_DB.dat"
