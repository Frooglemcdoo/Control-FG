#requires -Version 5.1
param([switch]$CollectOnly,[int]$GameProcessId=0,[string]$RunStartedUtc='',
      [string]$LogDirectory='', [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if (!$LogDirectory) { $LogDirectory=Join-Path $env:LOCALAPPDATA 'ControlFGProbe' }
if (!$OutputDirectory) { $OutputDirectory=$PSScriptRoot }
if (!$CollectOnly) {
    $gameRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
    $exe=Join-Path $gameRoot 'Control_DX12.exe'
    if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Install this inside ControlTOH\Diagnostics in the game folder.' }
    if (Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue) { throw 'Close Control before starting this check.' }
    $RunStartedUtc=[DateTime]::UtcNow.ToString('o')
    Write-Host 'Load a level with RT indirect lighting enabled, wait about 10 seconds, then exit normally.'
    Write-Host 'No camera route, video, screenshot or performance capture is needed.'
    Write-Host 'This checks pipeline compatibility. Deferred guided rendering is NOT enabled.'
    $game=Start-Process -FilePath $exe -WorkingDirectory $gameRoot -PassThru
    $GameProcessId=$game.Id
    $game.WaitForExit()
}
if ($GameProcessId -le 0 -or !$RunStartedUtc) { throw 'Missing process/session identity.' }
$started=[DateTime]::Parse($RunStartedUtc).ToUniversalTime()
$logs=@(Get-ChildItem -LiteralPath $LogDirectory -File -Filter "probe-*-$GameProcessId.log" | Where-Object {
    $_.LastWriteTimeUtc -ge $started -and $_.Length -le 32MB -and
    -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint)
} | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)
$tag=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$GameProcessId+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
$stage=Join-Path $OutputDirectory ('GI-Pipeline-R426-'+$tag)
New-Item -ItemType Directory -Path $stage | Out-Null
$lines=@()
foreach ($log in $logs) {
    Copy-Item -LiteralPath $log.FullName -Destination $stage
    $lines+=@([IO.File]::ReadAllLines($log.FullName) | Where-Object { $_ -match 'GI_FROZEN_R426|GI_TOGGLE_R308.*(failed|ready=)' })
}
$text=$lines -join "`r`n"
$result=[ordered]@{
    Schema='ControlTOH.GIPipelineCheck.R426'; ProcessId=$GameProcessId; StartedUtc=$RunStartedUtc
    LogFound=($logs.Count -eq 1)
    CheckRevision=2
    StartupObserved=($text -match 'GI_FROZEN_R426 startup=1 check_revision=2')
    RequestedInGame=($text -match 'GI_FROZEN_R426 startup=1 check_revision=2 requested=1')
    RayPipelineStatus=$(if ($text -match 'GI_FROZEN_R426 pipeline_prepared=1') {'ready'} elseif ($text -match 'GI_TOGGLE_R308.*(?:create_failed|record_failed|exception)=1|GI_FROZEN_R426 pipeline_prepared=0') {'failed'} else {'not_observed'})
    ComputePipelineStatus=$(if ($text -match 'GI_FROZEN_R426 compute_prepared=1') {'ready'} elseif ($text -match 'GI_FROZEN_R426 compute_prepared=0') {'failed'} else {'not_observed'})
    BindingStatus=$(if ($text -match 'GI_FROZEN_R426 binding_ready=1') {'ready'} elseif ($text -match 'GI_FROZEN_R426 binding_ready=0') {'rejected'} else {'not_observed'})
    RayPipelineReady=($text -match 'GI_FROZEN_R426 pipeline_prepared=1')
    ComputePipelinesReady=($text -match 'GI_FROZEN_R426 compute_prepared=1')
    LiveBindingsReady=($text -match 'GI_FROZEN_R426 binding_ready=1')
    DeferredRenderingEnabled=$false
    Scope='Pipeline creation and live root bindings only; no image quality, GPU execution, lifetime or FPS claim.'
    Evidence=$lines
}
$result | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $stage 'result.json') -Encoding UTF8
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath ($stage+'.zip')
Write-Host ('Send: '+$stage+'.zip')
if (!$result.LogFound) { Write-Warning 'No matching process log. Send the ZIP and the launch error; do not repeat visual captures.' }
elseif (!$result.RayPipelineReady -or !$result.ComputePipelinesReady -or !$result.LiveBindingsReady) {
    Write-Warning 'One or more checks failed or were not observed. Send the ZIP; do not repeat the run yet.'
}
