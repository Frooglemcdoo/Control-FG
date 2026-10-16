$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP ('PT-M12R86-Collector-Test-'+[guid]::NewGuid().ToString('N'))
$logs=Join-Path $root 'logs';$out=Join-Path $root 'out'
New-Item -ItemType Directory -Path $logs,$out | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Summary {
    $file=Get-ChildItem -LiteralPath $out -Filter '*.zip' | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    $archive=[IO.Compression.ZipFile]::OpenRead($file.FullName)
    try {
        $entry=$archive.GetEntry('Primary-M12R86-Summary.json')
        $reader=New-Object IO.StreamReader($entry.Open())
        try {return ($reader.ReadToEnd() | ConvertFrom-Json)} finally {$reader.Dispose()}
    } finally {$archive.Dispose()}
}
try {
    $log=Join-Path $logs 'probe-fixture-123.log'
    @('PT_PRIMARY_SESSION_READY build=M12R86 ready=1 hits=4','PT_PRIMARY_SUBMITTED count=1 frame=33 scene=exact admission_word=armed late_signal=1','PT_PRIMARY_ADMISSION ticket=99 reason=scene_dispatch_wait_pending','PT_PRIMARY_SKIP reason=fixture count=1','PT_PRIMARY_PSO_USED index=3 candidate=1 groups=320x180x1','PT_PRIMARY_SHADER_END index=3 bytes=200 sha256=fixture','PT_PRIMARY_QUEUE_CPU pt=0 operation=execute calls=10 lock_ms=1 observer_ms=2 native_ms=3 cumulative=1','PT_PRIMARY_QUEUE_CPU pt=1 operation=execute calls=20 lock_ms=4 observer_ms=5 native_ms=6 cumulative=1','PT_PRIMARY_PROGRESS signals=2 skipped=126 interval=64','PT_PRIMARY_HOT_CPU kind=bindings samples=100 sampled_ms=1 sampled_max_ms=0.1 interval=64 cumulative=1 scope=tracker','PT_PRIMARY_MIRROR_COPY single_calls=10 batch_calls=2 descriptors=108') |
        Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $cpu=Summary
    if(@($cpu.MirrorCopies).Count -ne 1){throw 'Mirror copy counts missing.'}
    if(@($cpu.HotPathTimings).Count -ne 1){throw 'Hot path timing evidence missing.'}
    if(@($cpu.QueueTimings).Count -ne 2 -or @($cpu.ProgressSamples).Count -ne 1){throw 'Queue profiling evidence missing.'}
    if(@($cpu.AdmissionFailures).Count -ne 1 -or $cpu.AdmissionFailures[0] -notmatch 'scene_dispatch_wait_pending'){throw 'Submission admission reason missing.'}
    if(@($cpu.LateAdmissions).Count -ne 1){throw 'Late signal admission evidence missing.'}
    if($cpu.ExecutedPipelineCount -ne 1 -or $cpu.CapturedLightingShaders -ne 1 -or @($cpu.ExecutedLightingCandidates).Count -ne 1){throw 'Executed shader evidence missing.'}
    if($cpu.LastSkipRecord -and $cpu.LastSkipRecord -isnot [string]){throw 'Collector serialized PowerShell metadata.'}
    if(-not $cpu.SessionReady -or -not $cpu.SawSubmittedDispatch -or $cpu.SawGPUCoveredSamples){throw 'CPU-only log was reported as GPU coverage.'}
    Add-Content -LiteralPath $log 'PT_PRIMARY_GPU frame=33 readback=1 armed=1 sampled=20 dimensions=20 primary_hit=19 material_match=18 depth_match=18 integrated=18 covered=0 fault=18 sparse_step=16'
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $zero=Summary
    if($zero.SawGPUCoveredSamples){throw 'Zero GPU coverage was reported as success.'}
    Add-Content -LiteralPath $log 'PT_PRIMARY_GPU frame=34 readback=1 armed=1 sampled=20 integrated=18 covered=17 fault=1 probe=1 sparse_step=16'
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $probe=Summary
    if($probe.SawGPUCoveredSamples){throw 'Sparse validation was reported as displayed PT coverage.'}
    Add-Content -LiteralPath $log 'PT_PRIMARY_GPU frame=34 readback=1 armed=1 sampled=20 dimensions=20 primary_hit=19 material_match=18 depth_match=18 integrated=18 covered=17 fault=1 sparse_step=16'
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $gpu=Summary
    if($gpu.LastGPURecord -isnot [string]){throw 'GPU record must be a plain string.'}
    if(-not $gpu.SawGPUCoveredSamples -or $gpu.LastGPURecord -notmatch 'covered=17'){throw 'GPU coverage record was lost.'}
    @('PT_PRIMARY_STARTUP build=M12R86 capture=0 restart_only=1 rr_fg_available=1','PT_PRIMARY_REFLEX_CPU present=240 sleep_ms=1 total_ms=240 max_ms=2 calls=240 capture=0') | Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $baseline=Summary
    if(-not $baseline.StartupCaptureDisabled -or $baseline.SessionReady -or $baseline.SawGPUCoveredSamples -or @($baseline.ReflexTimings).Count -ne 1){throw 'Capture-disabled baseline was not preserved honestly.'}
    @('PT_PRIMARY_STARTUP build=M12R86 capture=1','PT_PRIMARY_SESSION_READY build=M12R70 ready=1',
      'PT_PRIMARY_RECONSTRUCTION frame=40 history=1 previous_frame=39',
      'PT_PRIMARY_SECONDARY_TERMINATION frame=40 sampled=8 retained_primary=1') | Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $stale=Summary
    if(-not $stale.SessionReady -or $stale.StartupBuild -ne 'M12R86' -or $stale.SessionBuild -ne 'M12R70' -or @($stale.CollectionWarnings).Count -eq 0){throw 'Shipped R71 stale session marker not preserved.'}
    if(@($stale.Reconstruction).Count -ne 1 -or @($stale.SecondaryTerminations).Count -ne 1){throw 'Reconstruction evidence missing.'}
    @('PT_PRIMARY_STARTUP build=M12R86 capture=1','PT_PRIMARY_SKIP reason=startup_fixture') | Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $failed=Summary
    if($failed.SessionReady -or @($failed.CollectionWarnings).Count -eq 0){throw 'Failed startup not captured honestly.'}
    'Unrelated startup diagnostic' | Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R86.ps1" -LogDirectory $logs -OutputDirectory $out
    $missing=Summary
    if($missing.SessionReady -or $missing.SawGPUCoveredSamples -or @($missing.CollectionWarnings).Count -eq 0){throw 'Missing primary records not captured honestly.'}
    Write-Host 'PASS: stale R70 session marker in R71, failed startup, missing primary records, and reconstruction evidence retained.'
    Write-Host 'PASS: M12R86 collector distinguishes CPU submission, zero GPU coverage and covered GPU samples.'
} finally {Remove-Item -LiteralPath $root -Recurse -Force}
