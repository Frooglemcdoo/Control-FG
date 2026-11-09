param([Parameter(Mandatory=$true)][int]$GameProcessId,[Parameter(Mandatory=$true)][string]$ModuleFolder)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Capture-NativeProcess.ps1')
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$out=Join-Path $env:LOCALAPPDATA "ControlFGProbe\GI-Button-$stamp-$GameProcessId"
New-Item -ItemType Directory -Path $out | Out-Null
Start-Transcript -Path (Join-Path $out 'recorder-console.txt') | Out-Null
$complete=$false;$started=Get-Date
$game=$null
function Mark([string]$Name) {
    [ordered]@{phase=$Name;utc=[DateTime]::UtcNow.ToString('o');qpc=[Diagnostics.Stopwatch]::GetTimestamp();frequency=[Diagnostics.Stopwatch]::Frequency} | ConvertTo-Json -Compress | Add-Content (Join-Path $out 'phases.jsonl') -Encoding UTF8
}
function Tone([int]$Count=1) {for($i=0;$i -lt $Count;$i++){[Console]::Beep(900,180);if($i+1 -lt $Count){Start-Sleep -Milliseconds 160}}}
function Sample([int]$Seconds,[string]$Phase) {
    $clock=[Diagnostics.Stopwatch]::StartNew()
    $rows=New-Object System.Collections.Generic.List[object]
    while($clock.Elapsed.TotalSeconds -lt $Seconds){
        if($game.HasExited){throw 'Game closed before the measurement finished.'}
        $game.Refresh();[uint32]$foreground=0
        [void][GIButtonForeground]::GetWindowThreadProcessId([GIButtonForeground]::GetForegroundWindow(),[ref]$foreground)
        $rows.Add([pscustomobject]@{phase=$Phase;qpc=[Diagnostics.Stopwatch]::GetTimestamp();game_pid=$GameProcessId;foreground_pid=$foreground;cpu_total_ms=$game.TotalProcessorTime.TotalMilliseconds;working_set_bytes=$game.WorkingSet64})
        Start-Sleep -Milliseconds 250
    }
    $rows | Export-Csv -NoTypeInformation -Append -LiteralPath (Join-Path $out 'process-samples.csv')
}
function Invoke-GIButtonMeasurement {
    Write-Host 'Five seconds. Return to gameplay. One beep: stand still. Next beep: pan slowly. Two beeps: close game.'
    Start-Sleep -Seconds 5
    Mark 'STILL_BEGIN';Tone 1
    Sample 10 'STILL'
    Mark 'STILL_END';Mark 'PAN_BEGIN';Tone 1
    Sample 10 'PAN'
    Mark 'PAN_END';Tone 2
}
try {
    $game=Get-Process -Id $GameProcessId -ErrorAction Stop
    if($game.ProcessName -ne 'Control_DX12'){throw 'The requested process is not Control DX12.'}
    $python=Get-Command python.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if(-not $python){$python=Get-Command py.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1}
    $check=Invoke-CaptureNative -Executable $python.Source -Arguments @('-I',(Join-Path $PSScriptRoot 'check-capture-python.py')) -TimeoutSeconds 30
    if($check.ExitCode -ne 0 -or $check.Stdout -notmatch 'CONTROL_GI_PYTHON_OK'){throw "Python preflight failed: $($check.Text)"}
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class GIButtonForeground {
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint process);
}
'@
    [ordered]@{game_pid=$GameProcessId;capture_kind='compact-no-etw';module=$ModuleFolder;measurement='current selected GI mode; no mode switching';start_utc=$started.ToUniversalTime().ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $out 'session.json')
    Get-FileHash -LiteralPath (Join-Path $ModuleFolder 'dxgi.dll') -Algorithm SHA256 | ConvertTo-Json | Set-Content (Join-Path $out 'installed-dll.json')
    Invoke-GIButtonMeasurement
    $complete=$true
    Write-Host 'Measurement finished. You can close the game now. Packaging starts automatically.'
    # Allow normal exit to flush logs; limit accidental unattended recording.
    if(-not $game.WaitForExit(180000)){Write-Host 'Game is still open; saving now. Logs may not be fully flushed.'}
} catch {
    $_ | Format-List * -Force | Out-String | Set-Content (Join-Path $out 'error.txt')
    Write-Host $_.Exception.Message -ForegroundColor Red
    try {Tone 3} catch {}
}

try {
    $logs=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion | ConvertTo-Json | Set-Content (Join-Path $out 'gpu.json')
    [ordered]@{measurement_complete=$complete;trace_saved=$false;game_exited=($game -and $game.HasExited);mode='verify GI_BUTTON_CAPTURE and admission records in logs';gpu_pass_timings='not collected; native Present cadence and CPU stage timings only'} | ConvertTo-Json | Set-Content (Join-Path $out 'capture-status.json')
    Stop-Transcript | Out-Null
    $desktop=Join-Path ([Environment]::GetFolderPath('Desktop')) "Control-GI-SEND-$stamp-$GameProcessId"
    New-Item -ItemType Directory -Path $desktop | Out-Null
    $packed=Invoke-CaptureNative -Executable $python.Source -Arguments @('-I',(Join-Path $PSScriptRoot 'summarize-gi-performance.py'),$out,'--logs',$logs,'--pid',"$GameProcessId",'--send',$desktop) -TimeoutSeconds 120
    if($packed.ExitCode -ne 0){throw "Packaging failed: $($packed.Text)"}
    Write-Host "Send the single ZIP in $desktop"
    Start-Process explorer.exe -ArgumentList ('"'+$desktop+'"')
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Original files are retained in $out"
    Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show("Capture could not be packaged. Files are retained in:`n$out`n`n$($_.Exception.Message)",'Control GI capture')
    exit 1
}
