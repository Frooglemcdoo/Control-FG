param([Parameter(Mandatory=$true)][int]$GameProcessId,[Parameter(Mandatory=$true)][string]$ModuleFolder)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Capture-NativeProcess.ps1')
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$out=Join-Path $env:LOCALAPPDATA "ControlFGProbe\GI-Button-$stamp-$GameProcessId"
New-Item -ItemType Directory -Path $out | Out-Null
Start-Transcript -Path (Join-Path $out 'recorder-console.txt') | Out-Null
$instance='ControlGI_'+[Guid]::NewGuid().ToString('N')
$recording=$false;$complete=$false;$saved=$false;$started=Get-Date
$game=$null
function Wpr([string[]]$Arguments,[int]$Timeout=120) {
    $result=Invoke-CaptureNative -Executable $wpr -Arguments ($Arguments+@('-instancename',$instance)) -LogPath (Join-Path $out 'wpr.txt') -TimeoutSeconds $Timeout
    if($result.ExitCode -ne 0){throw "WPR failed ($($result.ExitCode)): $($result.Text)"}
    return $result.Stdout
}
function Mark([string]$Name) {
    [ordered]@{phase=$Name;utc=[DateTime]::UtcNow.ToString('o');qpc=[Diagnostics.Stopwatch]::GetTimestamp();frequency=[Diagnostics.Stopwatch]::Frequency} | ConvertTo-Json -Compress | Add-Content (Join-Path $out 'phases.jsonl') -Encoding UTF8
    Wpr -Arguments @('-marker',$Name) | Out-Null
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
    $wpr=Join-Path $env:WINDIR 'System32\wpr.exe'
    if(-not (Test-Path -LiteralPath $wpr)){throw 'Windows Performance Recorder is missing from System32.'}
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
    [ordered]@{game_pid=$GameProcessId;instance=$instance;module=$ModuleFolder;measurement='current selected GI mode; no mode switching';start_utc=$started.ToUniversalTime().ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $out 'session.json')
    Get-FileHash -LiteralPath (Join-Path $ModuleFolder 'dxgi.dll') -Algorithm SHA256 | ConvertTo-Json | Set-Content (Join-Path $out 'installed-dll.json')
    # The only recording is the real game capture. No preliminary start/stop/save.
    Wpr -Arguments @('-start','GeneralProfile','-start','GPU','-filemode') | Out-Null
    $recording=$true
    Invoke-GIButtonMeasurement
    $complete=$true
    Write-Host 'Measurement finished. You can close the game now. Packaging starts automatically.'
    # Allow normal exit to flush logs; limit accidental unattended recording.
    if(-not $game.WaitForExit(180000)){Write-Host 'Game is still open; saving now. Logs may not be fully flushed.'}
} catch {
    $_ | Format-List * -Force | Out-String | Set-Content (Join-Path $out 'error.txt')
    Write-Host $_.Exception.Message -ForegroundColor Red
    try {Tone 3} catch {}
} finally {
    if($recording){
        $recording=$false
        try {
            Write-Host 'Saving the CPU/GPU trace. Progress is shown below.'
            Wpr -Arguments @('-stop',(Join-Path $out 'cpu-gpu-timeline.etl'),'Control GI button capture','-skipPdbGen') -Timeout 600 | Out-Null
            $saved=Test-Path -LiteralPath (Join-Path $out 'cpu-gpu-timeline.etl')
        } catch {
            $_ | Out-String | Add-Content (Join-Path $out 'error.txt')
            Write-Host "Save failed: $($_.Exception.Message)" -ForegroundColor Red
        }
    }
}
try {
    $logs=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    $target=Join-Path $out 'logs';New-Item -ItemType Directory -Path $target | Out-Null
    # Top-level, PID-scoped logs only; never recursively copy our output folder.
    Get-ChildItem -LiteralPath $logs -File | Where-Object {
        $_.Name -match "-$GameProcessId(?:-|\.)" -and $_.Extension -in '.log','.json'
    } | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $target}
    Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion | ConvertTo-Json | Set-Content (Join-Path $out 'gpu.json')
    [ordered]@{measurement_complete=$complete;trace_saved=$saved;game_exited=($game -and $game.HasExited);mode='verify GI_BUTTON_CAPTURE and admission records in logs';gpu_pass_timings='ETW scheduling, not individual shader durations'} | ConvertTo-Json | Set-Content (Join-Path $out 'capture-status.json')
    Stop-Transcript | Out-Null
    $packed=Invoke-CaptureNative -Executable $python.Source -Arguments @('-I',(Join-Path $PSScriptRoot 'package-lighting-capture.py'),$out) -TimeoutSeconds 1800
    if($packed.ExitCode -ne 0 -or $packed.Stdout -notmatch 'Send ALL ZIP files in:\s*([^\r\n]+)'){throw "Packaging failed: $($packed.Text)"}
    $send=$Matches[1].Trim()
    $desktop=Join-Path ([Environment]::GetFolderPath('Desktop')) "Control-GI-SEND-$stamp-$GameProcessId"
    New-Item -ItemType Directory -Path $desktop | Out-Null
    Copy-Item -Path (Join-Path $send '*.zip') -Destination $desktop
    if(-not $complete -or -not $saved){'PARTIAL CAPTURE: include all ZIPs so the error can be inspected.' | Set-Content (Join-Path $desktop 'PARTIAL-CAPTURE.txt')}
    Write-Host "Send ALL ZIPs in $desktop"
    Start-Process explorer.exe -ArgumentList ('"'+$desktop+'"')
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "Original files are retained in $out"
    Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show("Capture could not be packaged. Files are retained in:`n$out`n`n$($_.Exception.Message)",'Control GI capture')
    exit 1
}
