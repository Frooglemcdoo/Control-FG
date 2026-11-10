param([ValidateRange(15,900)][int]$Seconds=180,[switch]$ValidateOnly)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Capture-NativeProcess.ps1')
$pm=Join-Path $PSScriptRoot 'PresentMon-2.3.1-x64.exe'
$expected='364e5d98d4d134bd54dd25c22ed2ca2f4883f8bc3ed6502bee0c151e3436d30c'
if(!(Test-Path -LiteralPath $pm) -or (Get-FileHash -LiteralPath $pm -Algorithm SHA256).Hash -ne $expected){throw 'PresentMon is missing or its hash does not match the verified package.'}
$wpr=(Get-Command wpr.exe -ErrorAction Stop).Source
$profileNames=@('CPU.light','GPU.light','DiskIO.light')
# Check exactly the light profiles; never silently substitute verbose tracing.
foreach($profile in $profileNames){
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-profiledetails',$profile)
 if($r.ExitCode -ne 0){throw "Required light profile unavailable: $profile. $($r.Text)"}
}
if($ValidateOnly){Write-Host 'PASS recorder dependency hashes and all three WPR light profiles';return}
$admin=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if(!$admin.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Run Record-Traversal.cmd as administrator.'}
$games=@(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
if($games.Count -ne 1){throw 'Start Control DX12, load your save, then run this recorder. Exactly one Control_DX12 process must be running.'}
$game=$games[0]
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$session="ControlTraversal-$PID-$stamp"
$out=Join-Path $PSScriptRoot "Runs\$session"
New-Item -ItemType Directory -Path $out -Force | Out-Null
$log=Join-Path $out 'recorder.log'
$etl=Join-Path $out 'timeline.etl'
$csv=Join-Path $out 'frames.csv'
$active=$false;$pmProcess=$null;$failure=$null
$meta=[ordered]@{version=1;process_id=$game.Id;process_name=$game.ProcessName;seconds_requested=$Seconds;profiles=$profileNames;session=$session;presentmon_sha256=$expected;qpc_frequency=[Diagnostics.Stopwatch]::Frequency;start_utc=[DateTime]::UtcNow.ToString('o');capture_start_qpc=$null;capture_end_qpc=$null;status='starting';stop_reason=$null;frame_rows=0;limitations=@('ETW overhead is not zero and must be checked against normal play.','Light CPU data is not a full function-call trace.','Disk I/O does not include all cache-served reads.','PresentMon CPU metrics are frame timing, not summed CPU thread utilization.','FG frame labels depend on provider support; unknown labels must not be treated as native FPS.','Menus and loading remain in raw data; use capture QPC bounds.','Trace event loss must be reviewed in wpr-status.txt before attribution.')}
$rows=New-Object 'Collections.Generic.List[object]'
function Wpr([string[]]$A){Invoke-CaptureNative -Executable $wpr -Arguments ($A+@('-instancename',$session)) -LogPath $log -TimeoutSeconds 180}
try {
 $drive=New-Object IO.DriveInfo([IO.Path]::GetPathRoot($out))
 if($drive.AvailableFreeSpace -lt 10GB){throw 'At least 10 GB free space is required on the recorder drive.'}
 # Retain actual provider definitions, so profile differences between Windows versions are visible.
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-exportprofile',($profileNames -join '+'),(Join-Path $out 'profiles.wprp'),'-filemode') -LogPath $log
 if($r.ExitCode -ne 0){throw 'Could not export the selected light profiles.'}
 $argsStart=@();foreach($p in $profileNames){$argsStart+=@('-start',$p)}
 $r=Wpr ($argsStart+@('-filemode','-recordtempto',$out))
 if($r.ExitCode -ne 0){throw "WPR could not start its private session. $($r.Text)"}
 $active=$true
 $pmArgs=@('--process_id',"$($game.Id)",'--output_file',$csv,'--qpc_time','--no_console_stats','--no_track_input','--track_frame_type','--session_name',$session,'--timed',"$($Seconds+10)",'--terminate_after_timed','--terminate_on_proc_exit')
 $psi=New-Object Diagnostics.ProcessStartInfo
 $psi.FileName=$pm;$psi.Arguments=($pmArgs|ForEach-Object{'"'+$_+'"'}) -join ' ';$psi.UseShellExecute=$false;$psi.CreateNoWindow=$true
 $psi.RedirectStandardOutput=$true;$psi.RedirectStandardError=$true
 $pmProcess=New-Object Diagnostics.Process;$pmProcess.StartInfo=$psi
 if(!$pmProcess.Start()){throw 'PresentMon failed to start.'}
 $stdout=$pmProcess.StandardOutput.ReadToEndAsync();$stderr=$pmProcess.StandardError.ReadToEndAsync()
 Write-Host "Return to gameplay. One beep in five seconds starts the $Seconds-second traversal window. Two beeps end it."
 Start-Sleep -Seconds 5
 if($pmProcess.HasExited){throw 'PresentMon exited before recording. See its logs.'}
 $meta.capture_start_qpc=[Diagnostics.Stopwatch]::GetTimestamp();$meta.status='recording'
 [Console]::Beep(900,150)
 $clock=[Diagnostics.Stopwatch]::StartNew()
 while($clock.Elapsed.TotalSeconds -lt $Seconds){
  $game.Refresh();if($game.HasExited){$meta.stop_reason='game_exit';break}
  if($pmProcess.HasExited){throw 'PresentMon stopped unexpectedly during the run.'}
  $pmProcess.Refresh()
  # One sample per second, buffered in memory. No per-frame polling/logging or graphics hooks.
  $rows.Add([pscustomobject]@{QPC=[Diagnostics.Stopwatch]::GetTimestamp();GameCPUSeconds=$game.TotalProcessorTime.TotalSeconds;GameWorkingSetBytes=$game.WorkingSet64;GamePrivateBytes=$game.PrivateMemorySize64;RecorderCPUSeconds=(Get-Process -Id $PID).TotalProcessorTime.TotalSeconds;PresentMonCPUSeconds=$pmProcess.TotalProcessorTime.TotalSeconds})
  if($drive.AvailableFreeSpace -lt 3GB){$meta.stop_reason='low_disk_space';break}
  Start-Sleep -Milliseconds 1000
 }
 if(!$meta.stop_reason){$meta.stop_reason='time_limit'}
 $meta.capture_end_qpc=[Diagnostics.Stopwatch]::GetTimestamp()
 [Console]::Beep(1100,150);[Console]::Beep(1100,150)
 Write-Host 'Capture window ended. You may stop moving. Saving trace; ignore performance during saving.'
 $meta.status='saving'
} catch { $failure=$_.Exception.Message;$meta.status='failed';$meta.error=$failure }
finally {
 if(!$meta.capture_end_qpc){$meta.capture_end_qpc=[Diagnostics.Stopwatch]::GetTimestamp()}
 if($pmProcess){
  try {
  if(!$pmProcess.HasExited){
   $r=Invoke-CaptureNative -Executable $pm -Arguments @('--session_name',$session,'--terminate_existing_session') -LogPath $log
   if(!$pmProcess.WaitForExit(15000)){$pmProcess.Kill();$failure='PresentMon required forced termination; CSV may be incomplete.'}
  }
  if($stdout -and $stdout.Wait(5000)){$stdout.Result|Set-Content (Join-Path $out 'presentmon-stdout.txt')}
  if($stderr -and $stderr.Wait(5000)){$stderr.Result|Set-Content (Join-Path $out 'presentmon-stderr.txt')}
  $meta.presentmon_exit_code=$pmProcess.ExitCode
  if($pmProcess.ExitCode -ne 0){$failure="PresentMon exit code $($pmProcess.ExitCode); inspect its logs."}
  $pmProcess.Dispose()
  } catch {$failure='PresentMon cleanup failed: '+$_.Exception.Message}
 }
 if($active){
  try {
   $r=Wpr @('-status','collectors','-details');$r.Text|Set-Content (Join-Path $out 'wpr-status.txt')
   $r=Wpr @('-stop',$etl,'Control traversal light trace','-skipPdbGen')
   if($r.ExitCode -ne 0){throw 'WPR could not save the trace.'}
   $active=$false
  } catch {
   $failure=$_.Exception.Message
   # Preserve temporary ETLs. Cancel only our unique session to avoid leaving tracing active.
   try{$r=Wpr @('-cancel')}catch{}
  }
 }
 $rows|Export-Csv (Join-Path $out 'process-samples.csv') -NoTypeInformation
 if(Test-Path $csv){$meta.frame_rows=@(Import-Csv $csv).Count}
 if(!$failure -and $meta.frame_rows -gt 0 -and (Test-Path $etl)){$meta.status='complete'}else{$meta.status='incomplete';if(!$failure){$failure='Missing frame rows or timeline; keep all files for diagnosis.'};$meta.error=$failure}
 $meta.end_utc=[DateTime]::UtcNow.ToString('o')
 $meta|ConvertTo-Json -Depth 6|Set-Content (Join-Path $out 'run.json') -Encoding UTF8
}
# Copy only the matching process log, never unrelated historic captures.
$logs=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
Get-ChildItem $logs -Filter "*-$($game.Id).log" -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1 | Copy-Item -Destination $out
Write-Host "Recording status: $($meta.status). Output: $out"
# ZIP after tracing; .NET streaming avoids Compress-Archive's 2GB per-file limit.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip="$out.zip"
try{[IO.Compression.ZipFile]::CreateFromDirectory($out,$zip,[IO.Compression.CompressionLevel]::Fastest,$false);Write-Host "Send: $zip"}catch{Write-Warning "ZIP failed; raw recording is preserved in $out. $($_.Exception.Message)"}
if($failure){throw $failure}
