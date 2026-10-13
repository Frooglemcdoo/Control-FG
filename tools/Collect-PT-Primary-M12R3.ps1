param([string]$LogDirectory=(Join-Path $env:LOCALAPPDATA 'ControlFGProbe'),[string]$OutputDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
$dir=$LogDirectory
$record=Get-ChildItem -LiteralPath $dir -Filter 'probe-*.log' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $record) {throw 'No Control-FG probe log found. Run Control DX12 with M12R3 first.'}
$stage=Join-Path $env:TEMP ('PT-Primary-M12R3-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    $copy=Join-Path $stage $record.Name
    Copy-Item -LiteralPath $record.FullName -Destination $copy
    $lines=@(Get-Content -LiteralPath $copy | ForEach-Object { [string]::Copy($_) })
    $primary=@($lines | Where-Object {$_ -match 'PT_PRIMARY_'})
    if (@($primary | Where-Object {$_ -match 'PT_PRIMARY_SESSION_READY build=M12R3 ready=1'}).Count -eq 0) {throw 'The latest probe log has no M12R3 primary renderer records. Check that the M12R3 dxgi.dll and shader sidecars were installed together.'}
    $gpu=@($primary | Where-Object {$_ -match 'PT_PRIMARY_GPU '})
    $coverage=@($gpu | Where-Object {$_ -match 'covered=([1-9][0-9]*)\b'})
    $skips=@($primary | Where-Object {$_ -match 'PT_PRIMARY_SKIP '})
    $summary=[ordered]@{
        Build='M12R3';Log=$record.Name
        SessionReady=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_SESSION_READY .*ready=1'}).Count -gt 0)
        SawSubmittedDispatch=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_SUBMITTED '}).Count -gt 0)
        SawGPUCoveredSamples=($coverage.Count -gt 0)
        LastGPURecord=($gpu | Select-Object -Last 1)
        LastSkipRecord=($skips | Select-Object -Last 1)
        LastCaptureRecord=($primary | Where-Object {$_ -match 'PT_PRIMARY_CAPTURE '} | Select-Object -Last 1)
        LastViewFailure=($primary | Where-Object {$_ -match 'PT_PRIMARY_VIEW_INVALID '} | Select-Object -Last 1)
        Scope='Integrated rendering test; GPU coverage samples one pixel per 16x16 block. Logged samples are not a full-frame coverage count or a visual-quality verdict.'
    }
    $summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'Primary-M12R3-Summary.json') -Encoding UTF8
    foreach($name in @('Build.json','SHA256SUMS.txt')) {
        $path=Join-Path $PSScriptRoot $name
        if(Test-Path -LiteralPath $path){Copy-Item -LiteralPath $path -Destination $stage}
    }
    $zip=Join-Path $OutputDirectory ('PT-Primary-M12R3-Results-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'.zip')
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
    $summary | ConvertTo-Json -Depth 4 | Write-Host
    Write-Host "Upload: $zip"
} finally {Remove-Item -LiteralPath $stage -Recurse -Force}
