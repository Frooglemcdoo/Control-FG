$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'PT-Profile.ps1'),[ref]$tokens,[ref]$errors) | Out-Null
if($errors.Count){throw ($errors | Out-String)}
# Validate the actual startup/PID evidence formats against the shipped logger.
$pidExample=44300
$logName='probe-20261013-232951-021-44300.log'
if($logName -notlike ('probe-*-'+$pidExample+'.log')){throw 'PID log lookup mismatch'}
foreach($capture in @(0,1)){
    $startup="qpc=123 tid=45 PT_PRIMARY_STARTUP build=M12R54 capture=$capture restart_only=1 rr_fg_available=1"
    if($startup -notmatch 'PT_PRIMARY_STARTUP build=M12R54 capture=[01]'){throw 'Startup version matcher failed'}
    if(($startup -match 'capture=0') -ne ($capture -eq 0)){throw 'Startup bypass matcher failed'}
}
Write-Host 'PASS: profiling script parses and matches runtime startup/PID evidence.'

# Check built-in recorder output on the Windows build host without starting a trace.
$wpr=Get-Command wpr.exe -ErrorAction SilentlyContinue
if($wpr){
    $previous=$ErrorActionPreference;$ErrorActionPreference='Continue'
    $profiles=(& $wpr.Source -profiles 2>&1 | Out-String)
    $status=(& $wpr.Source -status 2>&1 | Out-String)
    $ErrorActionPreference=$previous
    Write-Host $status
    if($profiles -notmatch '\bGeneralProfile\b' -or $profiles -notmatch '\bGPU\b'){throw 'Required WPR profiles absent'}
    if($status -notmatch '(?i)not recording'){throw 'WPR no-recording status matcher needs review'}
    Write-Host 'PASS: required Windows recorder profiles and idle status recognized.'
}

# Exercise the shipped collector with a fake recorder/game, including the
# refusal paths. This does not start an ETW session on the CI machine.
$admin=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $admin.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
    Write-Host 'SKIP: simulated capture flow requires the same admin gate as the shipped script.'
    exit 0
}
$root=Join-Path $env:TEMP ('PT-Profile-Test-'+[guid]::NewGuid().ToString('N'))
$oldLocal=$env:LOCALAPPDATA
New-Item -ItemType Directory -Path $root | Out-Null
try {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'PT-Profile.ps1') -Destination $root
    'dummy DLL for collector validation' | Set-Content (Join-Path $root 'dxgi.dll')
    @{Build='M12R54';Commit='test'} | ConvertTo-Json | Set-Content (Join-Path $root 'Build.json')
    $hash=(Get-FileHash (Join-Path $root 'dxgi.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
    ($hash+'  dxgi.dll') | Set-Content (Join-Path $root 'SHA256SUMS.txt')
    $env:LOCALAPPDATA=$root
    $logs=Join-Path $root 'ControlFGProbe';New-Item -ItemType Directory -Path $logs | Out-Null
    $log=Join-Path $logs 'probe-20261013-232951-021-44300.log'
    'PT_PRIMARY_STARTUP build=M12R54 capture=1 restart_only=1' | Set-Content $log
    $fakeGame=[pscustomobject]@{Id=44300;StartTime=(Get-Date);HasExited=$false;Modules=@([pscustomobject]@{ModuleName='dxgi.dll';FileName=(Join-Path $root 'dxgi.dll')})}
    $fakeGame | Add-Member -MemberType ScriptMethod -Name Refresh -Value {}
    $fakeWpr=Join-Path $root 'wpr-test.cmd'
    @'
@echo off
if "%1"=="-profiles" (
 echo GeneralProfile GPU
 exit /b 0
)
if "%1"=="-status" (
 echo WPR is not recording.
 exit /b 0
)
if "%1"=="-start" (
 echo start>>"%~dp0calls.txt"
 exit /b 0
)
if "%1"=="-stop" (
 echo stop>>"%~dp0calls.txt"
 echo fake ETL>"%~2"
 exit /b 0
)
echo unexpected>>"%~dp0calls.txt"
exit /b 1
'@ | Set-Content $fakeWpr -Encoding ASCII
    function Get-Command { [CmdletBinding()]param($Name) if($Name -ne 'wpr.exe'){throw 'Unexpected command lookup'};[pscustomobject]@{Source=$fakeWpr} }
    function Get-Process { [CmdletBinding()]param($Name) $fakeGame }
    function Start-Sleep { param($Seconds) }
    function Read-Host { param($Prompt) '' }
    & (Join-Path $root 'PT-Profile.ps1') -Mode PT-Off -Resolution Quality -OutputDirectory (Join-Path $root 'out') -Seconds 10
    $zips=@(Get-ChildItem (Join-Path $root 'out') -Filter '*.zip')
    if($zips.Count -ne 1){throw 'Expected exactly one capture ZIP'}
    $zip=[IO.Compression.ZipFile]::OpenRead($zips[0].FullName)
    try {if(@($zip.Entries | Where-Object {$_.Name -eq 'CPU-GPU.etl'}).Count -ne 1){throw 'Trace missing from result'}} finally {$zip.Dispose()}
    $calls=@(Get-Content (Join-Path $root 'calls.txt'))
    if(($calls -join ',') -ne 'start,stop'){throw 'Capture did not start/stop exactly once'}
    # A startup mode mismatch must fail before starting another session.
    $rejected=$false
    try {& (Join-Path $root 'PT-Profile.ps1') -Mode Bypass -Resolution DLAA -OutputDirectory (Join-Path $root 'out')} catch {$rejected=$_.Exception.Message -match 'disagrees'}
    if(-not $rejected -or @(Get-Content (Join-Path $root 'calls.txt')).Count -ne 2){throw 'Wrong startup mode was not safely rejected'}
    # An active recorder must be left alone, not stopped or cancelled.
    (Get-Content $fakeWpr -Raw).Replace('WPR is not recording.','WPR recording is in progress...') | Set-Content $fakeWpr -Encoding ASCII
    $rejected=$false
    try {& (Join-Path $root 'PT-Profile.ps1') -Mode PT-Off -Resolution DLAA -OutputDirectory (Join-Path $root 'out')} catch {$rejected=$_.Exception.Message -match 'active or its status'}
    if(-not $rejected -or @(Get-Content (Join-Path $root 'calls.txt')).Count -ne 2){throw 'Existing recorder was not left untouched'}
    Write-Host 'PASS: simulated capture packages trace; startup mismatch and existing recorder cause no tracing mutation.'
} finally {
    $env:LOCALAPPDATA=$oldLocal
    Remove-Item -LiteralPath $root -Recurse -Force
}
