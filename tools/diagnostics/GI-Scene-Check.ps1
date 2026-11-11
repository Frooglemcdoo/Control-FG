#requires -Version 5.1
param([switch]$CollectOnly,[int]$GameProcessId=0,[string]$RunStartedUtc='',
      [string]$LogDirectory='', [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if (!$LogDirectory) { $LogDirectory=Join-Path $env:LOCALAPPDATA 'ControlFGProbe' }
if (!$OutputDirectory) { $OutputDirectory=$PSScriptRoot }
if (!$CollectOnly) {
    $gameRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
    $exe=Join-Path $gameRoot 'Control_DX12.exe'
    if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Install inside ControlTOH\Diagnostics in the game folder.' }
    if (Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue) { throw 'Close Control before starting this check.' }
    $RunStartedUtc=[DateTime]::UtcNow.ToString('o')
    Write-Host 'R427: load a level with RT indirect lighting enabled, wait 15 seconds, then exit normally.'
    Write-Host 'No video, camera route, hotkey or image comparison is needed.'
    Write-Host 'This records scene-update metadata only. Guided rendering remains disabled.'
    $game=Start-Process -FilePath $exe -WorkingDirectory $gameRoot -PassThru
    $GameProcessId=$game.Id
    $game.WaitForExit()
}
if ($GameProcessId -le 0 -or !$RunStartedUtc) { throw 'Missing process/session identity.' }
$started=[DateTime]::Parse($RunStartedUtc).ToUniversalTime()
$tag=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$GameProcessId+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
$stage=Join-Path $OutputDirectory ('GI-Scene-R427-'+$tag)
New-Item -ItemType Directory -Path $stage | Out-Null
$file=Get-Item -LiteralPath (Join-Path $LogDirectory "GI-Phase-R425-$GameProcessId.jsonl") -ErrorAction SilentlyContinue
$found=$file -and $file.LastWriteTimeUtc -ge $started -and $file.Length -le 8MB -and -not ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)
$rows=@();$parseError='';$digest=''
if ($found) {
    Copy-Item -LiteralPath $file.FullName -Destination $stage
    $digest=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    try { $rows=@([IO.File]::ReadAllLines($file.FullName) | ForEach-Object { $_ | ConvertFrom-Json }) }
    catch { $parseError=$_.Exception.Message }
}
$install=@($rows | Where-Object { $_.kind -eq 25 })
$samples=@($rows | Where-Object { $_.kind -eq 22 })
$ray=@($samples | Where-Object { $_.data[2] -eq 1 })
$shade=@($samples | Where-Object { $_.data[2] -eq 2 })
$before=@($samples | Where-Object { $_.data[2] -eq 3 })
$after=@($samples | Where-Object { $_.data[2] -eq 8 })
$status=$rows | Where-Object { $_.status } | Select-Object -Last 1
$warnings=@()
if (!$found) { $warnings+='matching_session_log_missing' }
if ($parseError) { $warnings+='invalid_jsonl' }
$installed=$install.Count -eq 1 -and $install[0].data[0] -eq 427 -and $install[0].data[1] -eq 1
if (!$installed) { $warnings+='scene_hooks_not_confirmed' }
if (!$ray.Count -or !$shade.Count -or !$before.Count -or !$after.Count) { $warnings+='required_observation_points_missing' }
if (!$status -or $status.dropped -ne 0 -or $status.written -ne $status.reserved -or !$status.stopped) { $warnings+='trace_incomplete_or_overflowed' }
$active=@($samples | Where-Object { $_.data[5] -ne 0 -or $_.data[6] -ne 0 -or $_.data[7] -ne 0 })
$torn=@($samples | Where-Object { $_.data[3] -ne $_.data[4] })
$fault=@($samples | Where-Object { $_.data[9] -ne 0 })
if ($fault.Count) { $warnings+='native_metadata_read_fault' }
$result=[ordered]@{
    Schema='ControlTOH.GISceneCheck.R427';ProcessId=$GameProcessId;StartedUtc=$RunStartedUtc
    LogFound=[bool]$found;SHA256=$digest;ParseError=$parseError;HooksInstalled=$installed
    RaySamples=$ray.Count;ShadeSamples=$shade.Count;BeforeSubmitSamples=$before.Count;AfterSubmitSamples=$after.Count
    ActiveWriterSamples=$active.Count;ConcurrentChangeSamples=$torn.Count;ReadFaultSamples=$fault.Count
    Status=$status;Warnings=$warnings;DeferredRenderingEnabled=$false;SceneLeaseAuthorized=$false
    Scope='Observed writer activity and raw native epochs only. A quiet sample is not ownership proof, a noise fix or an FPS result.'
}
$result | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $stage 'result.json') -Encoding UTF8
# Include the same process startup log, when available, to diagnose unsupported hooks without another run.
$probe=Get-ChildItem -LiteralPath $LogDirectory -File -Filter "probe-*-$GameProcessId.log" -ErrorAction SilentlyContinue | Where-Object {
    $_.LastWriteTimeUtc -ge $started -and $_.Length -le 32MB -and -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint)
} | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if ($probe) { Copy-Item -LiteralPath $probe.FullName -Destination $stage }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath ($stage+'.zip')
Write-Host ('Send: '+$stage+'.zip')
if ($warnings.Count) { Write-Warning ('Send this ZIP without repeating the run: '+($warnings -join ', ')) }
