$ErrorActionPreference='Stop'
foreach($f in Get-ChildItem $PSScriptRoot -Filter '*.ps1'){
 $tokens=$null;$errors=$null
 [void][Management.Automation.Language.Parser]::ParseFile($f.FullName,[ref]$tokens,[ref]$errors)
 if($errors.Count){throw ($errors|Out-String)}
}
& (Join-Path $PSScriptRoot 'Record-Traversal.ps1') -ValidateOnly
. (Join-Path $PSScriptRoot 'Capture-NativeProcess.ps1')
$pm=Join-Path $PSScriptRoot 'PresentMon-2.3.1-x64.exe'
$session='ControlTraversal-CI-'+[guid]::NewGuid().ToString('N')
$folder=Join-Path $env:TEMP $session
New-Item -ItemType Directory $folder|Out-Null
$wpr=(Get-Command wpr.exe).Source
$active=$false
try {
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-start','CPU.light','-start','GPU.light','-start','DiskIO.light','-filemode','-recordtempto',$folder,'-instancename',$session)
 if($r.ExitCode -ne 0){throw $r.Text};$active=$true
 $r=Invoke-CaptureNative -Executable $pm -Arguments @('--process_id',"$PID",'--output_file',(Join-Path $folder 'frames.csv'),'--qpc_time','--no_console_stats','--no_track_input','--track_frame_type','--session_name',$session,'--timed','2','--terminate_after_timed','--terminate_on_proc_exit')
 if($r.ExitCode -ne 0){throw $r.Text}
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-status','collectors','-details','-instancename',$session)
 if($r.ExitCode -ne 0){throw $r.Text}
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-stop',(Join-Path $folder 'timeline.etl'),'-skipPdbGen','-instancename',$session)
 if($r.ExitCode -ne 0){throw $r.Text};$active=$false
 if((Get-Item (Join-Path $folder 'timeline.etl')).Length -eq 0){throw 'Empty trace'}
 Write-Host 'PASS real Windows WPR private light session start/status/save and PresentMon timed command; no graphical workload or overhead claim'
} finally {
 if($active){$r=Invoke-CaptureNative -Executable $wpr -Arguments @('-cancel','-instancename',$session)}
 Remove-Item $folder -Recurse -Force
}
