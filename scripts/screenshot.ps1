# Captura la ventana de juego de Dolphin en build/screenshots/<nombre>.png
# Por defecto usa PrintWindow: funciona aunque Dolphin esté tapado por otra ventana y no le quita el foco al usuario.
# -Focus: método antiguo (traer Dolphin al frente y copiar la pantalla), por si PrintWindow saliera en negro.
param([string]$Name = "shot", [int]$DelayMs = 0, [switch]$Focus)
$root = Split-Path $PSScriptRoot -Parent
if ($DelayMs -gt 0) { Start-Sleep -Milliseconds $DelayMs }
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W32 {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
}
"@
[W32]::SetProcessDPIAware() | Out-Null
$p = Get-Process Dolphin -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw "Dolphin no está abierto" }
$h = $p.MainWindowHandle
$r = New-Object W32+RECT; [W32]::GetClientRect($h, [ref]$r) | Out-Null
$w = $r.R - $r.L; $hgt = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap $w, $hgt
$g = [System.Drawing.Graphics]::FromImage($bmp)
$ok = $false
if (-not $Focus) {
    # 1 = solo el área cliente, 2 = PW_RENDERFULLCONTENT (incluye lo que pinta la GPU)
    $hdc = $g.GetHdc()
    $ok = [W32]::PrintWindow($h, $hdc, 3)
    $g.ReleaseHdc($hdc)
    if ($ok) {
        # Si sale todo negro (algunos backends), recurrir al método con foco
        $black = $true
        foreach ($fx in 0.25, 0.5, 0.75) { foreach ($fy in 0.3, 0.6) {
            $c = $bmp.GetPixel([int]($w * $fx), [int]($hgt * $fy))
            if ($c.R + $c.G + $c.B -gt 30) { $black = $false } } }
        if ($black) { $ok = $false }
    }
}
if (-not $ok) {
    [W32]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 300
    $pt = New-Object W32+POINT; [W32]::ClientToScreen($h, [ref]$pt) | Out-Null
    $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
}
$dir = Join-Path $root "build\screenshots"; New-Item -ItemType Directory -Force $dir | Out-Null
$out = Join-Path $dir "$Name.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Host $out
