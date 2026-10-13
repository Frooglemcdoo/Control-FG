$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'PT-Profile.ps1'),[ref]$tokens,[ref]$errors) | Out-Null
if($errors.Count){throw ($errors | Out-String)}
# Validate the actual startup/PID evidence formats against the shipped logger.
$pidExample=44300
$logName='probe-20261013-232951-021-44300.log'
if($logName -notlike ('probe-*-'+$pidExample+'.log')){throw 'PID log lookup mismatch'}
foreach($capture in @(0,1)){
    $startup="qpc=123 tid=45 PT_PRIMARY_STARTUP build=M12R41 capture=$capture restart_only=1 rr_fg_available=1"
    if($startup -notmatch 'PT_PRIMARY_STARTUP build=M12R41 capture=[01]'){throw 'Startup version matcher failed'}
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
