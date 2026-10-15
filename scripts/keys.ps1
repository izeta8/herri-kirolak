# Pulsa teclas en la ventana de Dolphin (para probar sin mando).
# Mapeo del Wii Remote emulado: X = A, Z = B, flechas = cruceta, 1/2 = botones 1/2, Q = −, E = +, Enter = HOME
# Nunchuk: W/A/S/D = stick, LSHIFT = Z, LCONTROL = C
# Inclinar: I/K (adelante/atrás), J/L (izquierda/derecha); swing: M (abajo), N (izquierda), O (derecha)
# Uso: scripts/keys.ps1 X,DOWN,DOWN,X -DelayMs 400   ·   "K+800" = mantener K 800 ms   ·   "K>" pulsar, "K<" soltar
param([string[]]$Keys, [int]$DelayMs = 350, [int]$HoldMs = 90)
Add-Type @"
using System; using System.Runtime.InteropServices;
public class K {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$vk = @{ W=0x57; A=0x41; S=0x53; D=0x44; LSHIFT=0xA0; LCONTROL=0xA2; I=0x49; J=0x4A; K=0x4B; L=0x4C; M=0x4D; N=0x4E; O=0x4F; X=0x58; Z=0x5A; Q=0x51; E=0x45; "1"=0x31; "2"=0x32; ENTER=0x0D; UP=0x26; DOWN=0x28; LEFT=0x25; RIGHT=0x27 }
$ext = @("UP","DOWN","LEFT","RIGHT")
$p = Get-Process Dolphin -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw "Dolphin no está abierto" }
[K]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 200
function Send([string]$name, [bool]$up) {
  $code = $vk[$name.ToUpper()]
  $scan = [byte][K]::MapVirtualKey($code, 0)
  $f = if ($ext -contains $name.ToUpper()) { 1 } else { 0 }
  if ($up) { $f = $f -bor 2 }
  [K]::keybd_event([byte]$code, $scan, $f, [UIntPtr]::Zero)
}
foreach ($k in $Keys) {
  if ($k -match '^(\w+)\+(\d+)$') { Send $Matches[1] $false; Start-Sleep -Milliseconds ([int]$Matches[2]); Send $Matches[1] $true; Start-Sleep -Milliseconds $DelayMs; continue }
  if ($k -match '^(\w+)>$') { Send $Matches[1] $false; Start-Sleep -Milliseconds $DelayMs; continue }
  if ($k -match '^(\w+)<$') { Send $Matches[1] $true; Start-Sleep -Milliseconds $DelayMs; continue }
  $code = $vk[$k.ToUpper()]
  if (-not $code) { Start-Sleep -Milliseconds ([int]$k); continue }  # un número = pausa extra en ms
  $scan = [byte][K]::MapVirtualKey($code, 0)
  $f = if ($ext -contains $k.ToUpper()) { 1 } else { 0 }
  [K]::keybd_event([byte]$code, $scan, $f, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds $HoldMs
  [K]::keybd_event([byte]$code, $scan, $f -bor 2, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds $DelayMs
}
