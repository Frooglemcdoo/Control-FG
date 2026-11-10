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
# Exercise the real orchestration and incomplete-data path with a non-rendering
# stand-in. This deliberately cannot establish in-game fidelity or overhead.
$fixtureDir=Join-Path $env:TEMP ('Traversal-fixture-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $fixtureDir|Out-Null
$fixtureExe=Join-Path $fixtureDir 'Control_DX12.exe'
Add-Type -TypeDefinition 'public static class TraversalFixture { public static void Main() { System.Threading.Thread.Sleep(90000); } }' -OutputAssembly $fixtureExe -OutputType ConsoleApplication
$fixture=Start-Process $fixtureExe -PassThru
$before=@(Get-ChildItem (Join-Path $PSScriptRoot 'Runs') -Directory -ErrorAction SilentlyContinue | ForEach-Object FullName)
try {
 $rejected=$false
 try{& (Join-Path $PSScriptRoot 'Record-Traversal.ps1') -Seconds 15}catch{$rejected=$true;Write-Host "Expected no-frame result: $($_.Exception.Message)"}
 $created=@(Get-ChildItem (Join-Path $PSScriptRoot 'Runs') -Directory | Where-Object {$before -notcontains $_.FullName})
 if($created.Count -ne 1){throw 'Expected one complete recorder output directory'}
 $run=Get-Content (Join-Path $created[0].FullName 'run.json') -Raw | ConvertFrom-Json
 if(!$rejected -or $run.status -ne 'incomplete' -or $run.frame_rows -ne 0){throw 'Non-rendering target must be explicitly incomplete'}
 if(!(Test-Path (Join-Path $created[0].FullName 'timeline.etl')) -or !(Test-Path ($created[0].FullName+'.zip'))){throw 'Incomplete capture lost trace or ZIP'}
 if(@(Import-Csv (Join-Path $created[0].FullName 'process-samples.csv')).Count -lt 10){throw 'Continuous process samples missing'}
 $r=Invoke-CaptureNative -Executable $wpr -Arguments @('-status','-instancename',$run.session)
 if($r.ExitCode -eq 0 -and $r.Text -notmatch 'not recording'){throw 'Recorder left its WPR session active'}
 Write-Host 'PASS real recorder lifecycle: timed window, process samples, saved ETL, ZIP, missing-frame rejection and session cleanup'
}finally{
 if(!$fixture.HasExited){$fixture.Kill();$fixture.WaitForExit()}
 Remove-Item $fixtureDir -Recurse -Force
 Get-ChildItem (Join-Path $PSScriptRoot 'Runs') -Directory -ErrorAction SilentlyContinue | Where-Object {$before -notcontains $_.FullName} | ForEach-Object {Remove-Item ($_.FullName+'.zip') -ErrorAction SilentlyContinue;Remove-Item $_.FullName -Recurse -Force}
}
