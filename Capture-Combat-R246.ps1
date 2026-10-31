# Capture-only: does not modify Control, its settings, or any DLL.
[CmdletBinding()]
param([switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
if ($ValidateOnly) { Write-Output 'PASS: capture script loaded'; return }
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Right-click Capture-Combat-R246.cmd and select Run as administrator.'
}
$wpr = Join-Path $env:SystemRoot 'System32\wpr.exe'
if (-not (Test-Path -LiteralPath $wpr)) { throw 'Windows Performance Recorder (wpr.exe) is unavailable.' }
$games = @(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
if ($games.Count -ne 1) { throw 'Start one Control DX12 instance using the R245 4ms pinned launcher first.' }
$game = $games[0]
$instance = 'ControlFGCombatR246'
$mutex = New-Object Threading.Mutex($false, 'Local\ControlFGCombatR246')
$locked = $false
$ownsSession = $false
$stage = $null
$manifest = [ordered]@{}
function Stamp {
    [ordered]@{ utc = [DateTime]::UtcNow.ToString('o'); qpc = [Diagnostics.Stopwatch]::GetTimestamp() }
}
function Invoke-Recorder([string[]]$Arguments, [string]$LogName) {
    $result = & $wpr @Arguments 2>&1
    $code = $LASTEXITCODE
    $result | Out-File -LiteralPath (Join-Path $stage $LogName) -Encoding utf8
    if ($code -ne 0) { throw "WPR failed ($code): $result. See $stage. No existing recording will be cancelled." }
}
try {
    $locked = $mutex.WaitOne(0)
    if (-not $locked) { throw 'Another combat capture tool is already running.' }
    $stage = Join-Path $PSScriptRoot ('Combat-R246-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $PID)
    New-Item -ItemType Directory -Path $stage | Out-Null
    $manifest = [ordered]@{
        tool = 'R246 capture only'; processId = $game.Id; processStartUtc = $game.StartTime.ToUniversalTime().ToString('o')
        qpcFrequency = [Diagnostics.Stopwatch]::Frequency; durationSeconds = 30; calmSeconds = 5
        note = 'CPU/GPU ETW adds overhead. Use for attribution, not FPS benchmarking. No DLL or setting changes.'
        modules = @($game.Modules | ForEach-Object { [ordered]@{ name=$_.ModuleName; path=$_.FileName; baseAddress=$_.BaseAddress.ToInt64().ToString('X'); size=$_.ModuleMemorySize } })
    }
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class CombatCaptureKeys {
    [DllImport("user32.dll")] public static extern short GetAsyncKeyState(int key);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint process);
    public static bool GameForeground(int pid) { uint actual; GetWindowThreadProcessId(GetForegroundWindow(), out actual); return actual == (uint)pid; }
}
'@
    Write-Host 'Return to the game near a fight. Press F8 to start (within 5 minutes).'
    Write-Host 'First beep: stay in the quiet scene for 5 seconds. Second beep: enter combat for 25 seconds.'
    Write-Host 'Third beep: recording finished. Return here and wait for the ZIP. No video capture is needed.'
    $armed = [Diagnostics.Stopwatch]::StartNew()
    $wasDown = $true
    while ($true) {
        if ($armed.Elapsed.TotalMinutes -ge 5) { throw 'Timed out waiting for F8; no trace was started.' }
        if ($game.HasExited) { throw 'Control exited before recording.' }
        $down = [CombatCaptureKeys]::GetAsyncKeyState(0x77) -lt 0
        if ($down -and -not $wasDown -and [CombatCaptureKeys]::GameForeground($game.Id)) { break }
        $wasDown = $down
        Start-Sleep -Milliseconds 50
    }
    $manifest.startRequested = Stamp
    Invoke-Recorder @('-start','CPU','-start','GPU','-instancename',$instance) 'wpr-start.txt'
    $ownsSession = $true
    $manifest.recordingStarted = Stamp
    $timer = [Diagnostics.Stopwatch]::StartNew()
    [Console]::Beep(700,150)
    $cue = $false
    while ($timer.Elapsed.TotalSeconds -lt 30 -and -not $game.HasExited) {
        if (-not $cue -and $timer.Elapsed.TotalSeconds -ge 5) {
            $manifest.combatCue = Stamp
            [Console]::Beep(1100,200)
            $cue = $true
        }
        Start-Sleep -Milliseconds 50
    }
    $manifest.recordingEndRequested = Stamp
    $manifest.gameExited = $game.HasExited
    [Console]::Beep(500,300)
    # Status preserves dropped-event information before the session is stopped.
    & $wpr -status -instancename $instance 2>&1 | Out-File (Join-Path $stage 'wpr-status.txt') -Encoding utf8
    Write-Host 'Saving trace; this may take a minute. Do not close this window.'
    Invoke-Recorder @('-stop',(Join-Path $stage 'combat.etl'),'Control FG quiet scene then combat','-skipPdbGen','-compress','-instancename',$instance) 'wpr-stop.txt'
    $ownsSession = $false
    $manifest.saved = Stamp
    if (-not (Test-Path -LiteralPath (Join-Path $stage 'combat.etl'))) { throw 'WPR returned success without producing combat.etl.' }
    $logRoot = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    $probe = Get-ChildItem -LiteralPath $logRoot -Filter "probe-*-$($game.Id).log" -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if ($probe) {
        try { Copy-Item -LiteralPath $probe.FullName -Destination $stage }
        catch { $manifest.probeCopyWarning = $_.Exception.Message }
    } else { $manifest.probeCopyWarning = 'No probe log for this game PID found.' }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $stage 'capture.json') -Encoding utf8
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = $stage + '.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
    Write-Host "DONE: send $zip"
    Write-Host 'The uncompressed trace is also retained beside the ZIP.'
} finally {
    if ($ownsSession) {
        # Only a session successfully started by THIS process may be cancelled.
        & $wpr -cancel -instancename $instance 2>&1 | Out-File (Join-Path $stage 'wpr-cleanup.txt') -Encoding utf8
    }
    if ($stage -and $manifest.Count) {
        $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $stage 'capture.json') -Encoding utf8
    }
    if ($locked) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
