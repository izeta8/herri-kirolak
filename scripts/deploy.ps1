# Envía un .dol a la Wii por WiFi (el Homebrew Channel tiene que estar abierto en la Wii).
# Busca la Wii por su MAC si la IP guardada no responde (el router a veces le cambia la IP).
param([string]$Dol = "", [string]$Ip = "")
$root = Split-Path $PSScriptRoot -Parent
if (-not $Dol) { $Dol = Join-Path $root "build\boot.dol" }
$mac = "78-A2-A0-91-11-36"
$cache = Join-Path $root "build\wii-ip.txt"
function Listening($addr) { $addr -and (Test-NetConnection $addr -Port 4299 -WarningAction SilentlyContinue).TcpTestSucceeded }
if (-not $Ip -and (Test-Path $cache)) { $Ip = (Get-Content $cache).Trim() }
if (-not (Listening $Ip)) {
    $Ip = $null
    $subnet = ((Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -like '192.168.*' } | Select-Object -First 1).IPAddress -replace '\.\d+$', '')
    foreach ($round in 1..3) {
        $jobs = 1..254 | ForEach-Object { (New-Object System.Net.NetworkInformation.Ping).SendPingAsync("$subnet.$_", 800) }
        [System.Threading.Tasks.Task]::WaitAll($jobs) | Out-Null
        foreach ($n in (Get-NetNeighbor -AddressFamily IPv4 | Where-Object { $_.LinkLayerAddress -eq $mac })) {
            if (Listening $n.IPAddress) { $Ip = $n.IPAddress; break }
        }
        if ($Ip) { break }
    }
    if (-not $Ip) { throw "No encuentro la Wii: ¿está el Homebrew Channel abierto y conectado a la WiFi?" }
}
Set-Content $cache $Ip
Write-Host "Wii en $Ip"
$env:WIILOAD = "tcp:$Ip"
& "C:\devkitPro\tools\bin\wiiload.exe" $Dol
