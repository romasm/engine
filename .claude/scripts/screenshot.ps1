param(
    [string]$ProcessName = "core_dev",
    [string]$WindowTitle = "",
    [string]$OutputPath = "$env:TEMP\engine_screenshot.png"
)

Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;

public class WindowCapture
{
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);

    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left, Top, Right, Bottom;
    }

    public static Bitmap CaptureWindow(IntPtr hWnd)
    {
        // If minimized, restore it first
        if (IsIconic(hWnd))
        {
            ShowWindow(hWnd, 9); // SW_RESTORE
            System.Threading.Thread.Sleep(500);
        }

        RECT rect;
        GetWindowRect(hWnd, out rect);
        int width = rect.Right - rect.Left;
        int height = rect.Bottom - rect.Top;

        if (width <= 0 || height <= 0) return null;

        Bitmap bmp = new Bitmap(width, height);
        Graphics gfx = Graphics.FromImage(bmp);
        IntPtr hdc = gfx.GetHdc();
        // PW_RENDERFULLCONTENT = 2 (captures DX/OpenGL content too)
        PrintWindow(hWnd, hdc, 2);
        gfx.ReleaseHdc(hdc);
        gfx.Dispose();
        return bmp;
    }
}
"@ -ReferencedAssemblies System.Drawing

# Find the target window
$hWnd = [IntPtr]::Zero

if ($WindowTitle -ne "") {
    # Search by window title substring
    $procs = Get-Process | Where-Object { $_.MainWindowTitle -like "*$WindowTitle*" -and $_.MainWindowHandle -ne [IntPtr]::Zero } | Select-Object -First 1
    if ($procs) {
        $hWnd = $procs.MainWindowHandle
        Write-Host "Found window: '$($procs.MainWindowTitle)' (process: $($procs.ProcessName))"
    }
} else {
    # Search by process name
    $proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero } | Select-Object -First 1
    if ($proc) {
        $hWnd = $proc.MainWindowHandle
        Write-Host "Found window: '$($proc.MainWindowTitle)' (process: $($proc.ProcessName))"
    }
}

if ($hWnd -eq [IntPtr]::Zero) {
    Write-Error "No window found for process='$ProcessName' title='$WindowTitle'"
    exit 1
}

$bmp = [WindowCapture]::CaptureWindow($hWnd)
if (-not $bmp) {
    Write-Error "Failed to capture window (zero-size or invisible)"
    exit 1
}

$bmp.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Output "Screenshot saved to: $OutputPath"
