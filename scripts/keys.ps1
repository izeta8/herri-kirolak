# Pulsa teclas en la ventana de Dolphin (para probar sin mando).
# Mapeo del Wii Remote emulado: X = A, Z = B, flechas = cruceta, 1/2 = botones 1/2, Q = −, E = +, Enter = HOME
# Uso: scripts/keys.ps1 X,DOWN,DOWN,X -DelayMs 400
param([string[]]$Keys, [int]$DelayMs = 350, [int]$HoldMs = 90)
Add-Type @"
using System; using System.Runtime.InteropServices;
public class K {
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint code, uint type);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
"@
$vk = @{ X=0x58; Z=0x5A; Q=0x51; E=0x45; "1"=0x31; "2"=0x32; ENTER=0x0D; UP=0x26; DOWN=0x28; LEFT=0x25; RIGHT=0x27 }
$ext = @("UP","DOWN","LEFT","RIGHT")
$p = Get-Process Dolphin -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw "Dolphin no está abierto" }
[K]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 200
foreach ($k in $Keys) {
  $code = $vk[$k.ToUpper()]
  if (-not $code) { Start-Sleep -Milliseconds ([int]$k); continue }  # un número = pausa extra en ms
  $scan = [byte][K]::MapVirtualKey($code, 0)
  $f = if ($ext -contains $k.ToUpper()) { 1 } else { 0 }
  [K]::keybd_event([byte]$code, $scan, $f, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds $HoldMs
  [K]::keybd_event([byte]$code, $scan, $f -bor 2, [UIntPtr]::Zero)
  Start-Sleep -Milliseconds $DelayMs
}
