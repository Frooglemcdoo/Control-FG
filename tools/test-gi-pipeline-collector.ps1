$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$temp=Join-Path ([IO.Path]::GetTempPath()) ('Control pipeline test '+[guid]::NewGuid().ToString('N'))
try {
    $logs=Join-Path $temp 'logs';$out=Join-Path $temp 'output'
    New-Item -ItemType Directory -Path $logs,$out | Out-Null
    $script=Join-Path $repo 'tools/diagnostics/GI-Pipeline-Check.ps1'
    $fake=Join-Path $logs 'probe-today-1234.log'
    @('GI_FROZEN_R426 pipeline_prepared=1 native_hit_miss_same_state=1 deferred_dispatch_enabled=0',
      'GI_FROZEN_R426 compute_prepared=1 hr=0x00000000',
      'GI_FROZEN_R426 binding_ready=1 captured=1 attached=1') | Set-Content $fake -Encoding ASCII
    & $script -CollectOnly -GameProcessId 1234 -RunStartedUtc ([DateTime]::UtcNow.AddMinutes(-1).ToString('o')) -LogDirectory $logs -OutputDirectory $out
    $folders=@(Get-ChildItem $out -Directory);if ($folders.Count -ne 1) { throw 'Expected one result folder' }
    $r=Get-Content (Join-Path $folders[0].FullName 'result.json') -Raw | ConvertFrom-Json
    if (!$r.LogFound -or !$r.RayPipelineReady -or !$r.ComputePipelinesReady -or !$r.LiveBindingsReady -or $r.DeferredRenderingEnabled) { throw 'Pipeline result misclassified' }
    if (@(Get-ChildItem $out -Filter '*.zip').Count -ne 1) { throw 'ZIP missing' }
    $missing=Join-Path $temp 'missing';New-Item -ItemType Directory -Path $missing | Out-Null
    & $script -CollectOnly -GameProcessId 5678 -RunStartedUtc ([DateTime]::UtcNow.AddMinutes(-1).ToString('o')) -LogDirectory $logs -OutputDirectory $missing
    $folder=Get-ChildItem $missing -Directory | Select-Object -First 1
    $r=Get-Content (Join-Path $folder.FullName 'result.json') -Raw | ConvertFrom-Json
    if ($r.LogFound -or $r.RayPipelineReady) { throw 'Wrong-process log accepted' }
    $stale=Join-Path $temp 'stale';New-Item -ItemType Directory -Path $stale | Out-Null
    & $script -CollectOnly -GameProcessId 1234 -RunStartedUtc ([DateTime]::UtcNow.AddMinutes(1).ToString('o')) -LogDirectory $logs -OutputDirectory $stale
    $folder=Get-ChildItem $stale -Directory | Select-Object -First 1
    $r=Get-Content (Join-Path $folder.FullName 'result.json') -Raw | ConvertFrom-Json
    if ($r.LogFound) { throw 'Previous-session log accepted' }
    Write-Host 'PASS pipeline collector: exact process/session, three independent readiness checks, missing-log report, ZIP and paths with spaces'
} finally {if(Test-Path $temp){Remove-Item $temp -Recurse -Force}}
