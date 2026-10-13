# Envía un .dol a la Wii por WiFi.
#  1) Si el juego está abierto, se lo manda a él (puerto 4301) y el juego salta a la versión nueva.
#  2) Si no, se lo manda al Homebrew Channel con wiiload (puerto 4299; el HBC tiene que estar abierto).
# Busca la Wii por su MAC si la IP guardada no responde (el router a veces le cambia la IP).
param([string]$Dol = "", [string]$Ip = "")
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dol) { $Dol = Join-Path $root "build\boot.dol" }
$mac = "78-A2-A0-91-11-36"
$cache = Join-Path $root "build\wii-ip.txt"

function PortOpen($addr, $port) {
    if (-not $addr) { return $false }
    $c = New-Object Net.Sockets.TcpClient
    try { $ok = $c.ConnectAsync($addr, $port).Wait(1500); return $ok -and $c.Connected } catch { return $false } finally { $c.Close() }
}
function Reachable($addr) { (PortOpen $addr 4301) -or (PortOpen $addr 4299) }

if (-not $Ip -and (Test-Path $cache)) { $Ip = (Get-Content $cache).Trim() }
if (-not (Reachable $Ip)) {
    $Ip = $null
    $subnet = ((Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like '192.168.*' } | Select-Object -First 1).IPAddress -replace '\.\d+$', '')
    foreach ($round in 1..3) {
        $jobs = 1..254 | ForEach-Object { (New-Object System.Net.NetworkInformation.Ping).SendPingAsync("$subnet.$_", 800) }
        [System.Threading.Tasks.Task]::WaitAll($jobs) | Out-Null
        foreach ($n in (Get-NetNeighbor -AddressFamily IPv4 | Where-Object { $_.LinkLayerAddress -eq $mac })) {
            if (Reachable $n.IPAddress) { $Ip = $n.IPAddress; break }
        }
        if ($Ip) { break }
    }
    if (-not $Ip) { throw "No encuentro la Wii: ¿está el juego o el Homebrew Channel abierto y con WiFi?" }
}
if ($Ip -ne "127.0.0.1") { Set-Content $cache $Ip }

if (PortOpen $Ip 4301) {
    Write-Host "Juego abierto en $Ip -> actualizando en caliente"
    $bytes = [IO.File]::ReadAllBytes($Dol)
    $c = New-Object Net.Sockets.TcpClient($Ip, 4301)
    $s = $c.GetStream()
    $len = $bytes.Length
    $hdr = [byte[]](@([Text.Encoding]::ASCII.GetBytes("HKUP")) + @((($len -shr 24) -band 255), (($len -shr 16) -band 255), (($len -shr 8) -band 255), ($len -band 255)))
    $s.Write($hdr, 0, 8)
    $s.Write($bytes, 0, $bytes.Length)
    $s.Flush()
    $s.ReadTimeout = 15000
    $buf = New-Object byte[] 8
    $n = $s.Read($buf, 0, 8)
    $c.Close()
    $reply = [Text.Encoding]::ASCII.GetString($buf, 0, $n).Trim()
    if ($reply -ne "OK") { throw "La Wii respondió: $reply" }
    Write-Host "OK: la Wii está arrancando la versión nueva"
} else {
    Write-Host "Homebrew Channel en $Ip -> wiiload"
    $env:WIILOAD = "tcp:$Ip"
    & "C:\devkitPro\tools\bin\wiiload.exe" $Dol
}
