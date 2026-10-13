param([string]$LogDirectory=(Join-Path $env:LOCALAPPDATA 'ControlFGProbe'),[string]$OutputDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
$dir=$LogDirectory
$record=Get-ChildItem -LiteralPath $dir -Filter 'probe-*.log' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $record) {throw 'No Control-FG probe log found. Run Control DX12 with M12R24 first.'}
$stage=Join-Path $env:TEMP ('PT-Primary-M12R24-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    $copy=Join-Path $stage $record.Name
    Copy-Item -LiteralPath $record.FullName -Destination $copy
    $lines=@(Get-Content -LiteralPath $copy | ForEach-Object { [string]::Copy($_) })
    $primary=@($lines | Where-Object {$_ -match 'PT_PRIMARY_'})
    if (@($primary | Where-Object {$_ -match 'PT_PRIMARY_SESSION_READY build=M12R24 ready=1'}).Count -eq 0) {throw 'The latest probe log has no M12R24 primary renderer records. Check that the M12R24 dxgi.dll and shader sidecars were installed together.'}
    $gpu=@($primary | Where-Object {$_ -match 'PT_PRIMARY_GPU '})
    $coverage=@($gpu | Where-Object {$_ -match 'covered=([1-9][0-9]*)\b'})
    $skips=@($primary | Where-Object {$_ -match 'PT_PRIMARY_SKIP '})
    $summary=[ordered]@{
        Build='M12R24';Log=$record.Name
        SessionReady=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_SESSION_READY .*ready=1'}).Count -gt 0)
        SawSubmittedDispatch=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_SUBMITTED '}).Count -gt 0)
        SawGPUCoveredSamples=($coverage.Count -gt 0)
        LastGPURecord=($gpu | Select-Object -Last 1)
        LastSkipRecord=($skips | Select-Object -Last 1)
        HandoffFailures=@($primary | Where-Object {$_ -match 'PT_PRIMARY_HANDOFF '})
        AdmissionFailures=@($primary | Where-Object {$_ -match 'PT_PRIMARY_(ADMISSION|LATE_REJECT) '})
        LatePending=@($primary | Where-Object {$_ -match 'PT_PRIMARY_LATE_PENDING '})
        LateExpired=@($primary | Where-Object {$_ -match 'PT_PRIMARY_LATE_EXPIRED '})
        DeferredDispatches=@($primary | Where-Object {$_ -match 'PT_PRIMARY_DEFERRED '})
        LateAdmissions=@($primary | Where-Object {$_ -match 'PT_PRIMARY_SUBMITTED .*late_signal=1'})
        PreflightFailures=@($primary | Where-Object {$_ -match 'PT_PRIMARY_PREFLIGHT '})
        LastDescriptorStats=($primary | Where-Object {$_ -match 'PT_PRIMARY_DESCRIPTOR_STATS '} | Select-Object -Last 1)
        DescriptorFailures=@($primary | Where-Object {$_ -match 'PT_PRIMARY_DESCRIPTOR '})
        LastOwnershipRecord=($primary | Where-Object {$_ -match 'PT_PRIMARY_OWNERS '} | Select-Object -Last 1)
        LastCaptureRecord=($primary | Where-Object {$_ -match 'PT_PRIMARY_CAPTURE '} | Select-Object -Last 1)
        LastRouteRecord=($primary | Where-Object {$_ -match 'PT_PRIMARY_ROUTE '} | Select-Object -Last 1)
        LastNativeRouteRecord=($primary | Where-Object {$_ -match 'PT_PRIMARY_NATIVE_ROUTE '} | Select-Object -Last 1)
        LastViewFailure=($primary | Where-Object {$_ -match 'PT_PRIMARY_VIEW_INVALID '} | Select-Object -Last 1)
        MatchedLightingVariants=@($primary | Where-Object {$_ -match 'PT_PRIMARY_PIPELINE_READY '})
        SkipReasons=@($skips | ForEach-Object {if($_ -match 'reason=([^ ]+)'){$Matches[1]}} | Sort-Object -Unique)
        BoundLightingCandidates=@($primary | Where-Object {$_ -match 'PT_PRIMARY_PSO_BOUND .*candidate=1'})
        ExecutedPipelineCount=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_PSO_USED '}).Count)
        ExecutedLightingCandidates=@($primary | Where-Object {$_ -match 'PT_PRIMARY_PSO_USED .*candidate=1'})
        CapturedLightingShaders=(@($primary | Where-Object {$_ -match 'PT_PRIMARY_SHADER_END '}).Count)
        UntrackedPipelines=@($primary | Where-Object {$_ -match 'PT_PRIMARY_PSO_UNTRACKED '})
        Scope='Integrated rendering test; GPU coverage samples one pixel per 16x16 block. Logged samples are not a full-frame coverage count or a visual-quality verdict.'
    }
    $summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'Primary-M12R24-Summary.json') -Encoding UTF8
    foreach($name in @('Build.json','SHA256SUMS.txt')) {
        $path=Join-Path $PSScriptRoot $name
        if(Test-Path -LiteralPath $path){Copy-Item -LiteralPath $path -Destination $stage}
    }
    $zip=Join-Path $OutputDirectory ('PT-Primary-M12R24-Results-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'.zip')
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
    $summary | ConvertTo-Json -Depth 4 | Write-Host
    Write-Host "Upload: $zip"
} finally {Remove-Item -LiteralPath $stage -Recurse -Force}
