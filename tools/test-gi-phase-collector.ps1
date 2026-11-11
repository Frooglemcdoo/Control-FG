$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$temp=Join-Path ([IO.Path]::GetTempPath()) ('Control phase test '+[guid]::NewGuid().ToString('N'))
$oldLocal=$env:LOCALAPPDATA
try {
    $logs=Join-Path $temp 'ControlFGProbe'
    $diagnostics=Join-Path $temp 'Diagnostics'
    New-Item -ItemType Directory -Path $logs,$diagnostics -Force | Out-Null
    $env:LOCALAPPDATA=$temp
    $collector=Join-Path $diagnostics 'Collect-GI-Phase-Check.ps1'
    Copy-Item (Join-Path $repo 'tools/diagnostics/Collect-GI-Phase-Check.ps1') $collector
    $fake=Join-Path $logs 'GI-Phase-R425-1234.jsonl'
    @('{"seq":1,"kind":1,"data":[100,1,1,1,1,1,1,1,1,1,1,1]}','{"status":true,"first_frame":100,"stopped":1,"reserved":1,"written":1,"dropped":0}') | Set-Content $fake -Encoding ASCII
    & $collector -RunStartedUtc ([DateTime]::UtcNow.AddMinutes(-1).ToString('o'))
    $zips=@(Get-ChildItem $diagnostics -Filter '*.zip')
    if ($zips.Count -ne 1) { throw 'Collector failed to produce one bounded ZIP' }
    $rejected=$false
    try { & $collector -RunStartedUtc ([DateTime]::UtcNow.AddMinutes(1).ToString('o')) } catch { $rejected=$true }
    if (!$rejected) { throw 'Collector accepted a previous-session log' }
    $launcher=Get-Content (Join-Path $repo 'tools/diagnostics/Start-GI-Phase-Check.cmd')
    $line=$launcher | Where-Object { $_ -like 'for /f *' }
    if (@($line).Count -ne 1) { throw 'Expected one timestamp command' }
    $cmd=Join-Path $temp 'timestamp.cmd'
    @('@echo off',$line,'echo %PHASE_RUN_START%') | Set-Content $cmd -Encoding ASCII
    $value=(& cmd /c $cmd | Select-Object -Last 1)
    $parsed=[DateTime]::Parse($value)
    if ([Math]::Abs(($parsed.ToUniversalTime()-[DateTime]::UtcNow).TotalSeconds) -gt 30) { throw 'CMD timestamp was not current' }
    Write-Host 'PASS phase collector: paths with spaces, bounded package, stale-run rejection, actual CMD timestamp'
} finally {
    $env:LOCALAPPDATA=$oldLocal
    if(Test-Path $temp){Remove-Item $temp -Recurse -Force}
}
