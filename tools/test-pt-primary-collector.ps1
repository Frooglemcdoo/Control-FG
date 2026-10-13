$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP ('PT-M12R5-Collector-Test-'+[guid]::NewGuid().ToString('N'))
$logs=Join-Path $root 'logs';$out=Join-Path $root 'out'
New-Item -ItemType Directory -Path $logs,$out | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Summary {
    $file=Get-ChildItem -LiteralPath $out -Filter '*.zip' | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    $archive=[IO.Compression.ZipFile]::OpenRead($file.FullName)
    try {
        $entry=$archive.GetEntry('Primary-M12R5-Summary.json')
        $reader=New-Object IO.StreamReader($entry.Open())
        try {return ($reader.ReadToEnd() | ConvertFrom-Json)} finally {$reader.Dispose()}
    } finally {$archive.Dispose()}
}
try {
    $log=Join-Path $logs 'probe-fixture-123.log'
    @('PT_PRIMARY_SESSION_READY build=M12R5 ready=1 hits=4','PT_PRIMARY_SUBMITTED count=1 frame=33 scene=exact alpha_guard=armed','PT_PRIMARY_SKIP reason=fixture count=1') |
        Set-Content -LiteralPath $log -Encoding UTF8
    & "$PSScriptRoot/Collect-PT-Primary-M12R5.ps1" -LogDirectory $logs -OutputDirectory $out
    $cpu=Summary
    if($cpu.LastSkipRecord -and $cpu.LastSkipRecord -isnot [string]){throw 'Collector serialized PowerShell metadata.'}
    if(-not $cpu.SessionReady -or -not $cpu.SawSubmittedDispatch -or $cpu.SawGPUCoveredSamples){throw 'CPU-only log was reported as GPU coverage.'}
    Add-Content -LiteralPath $log 'PT_PRIMARY_GPU frame=33 readback=1 armed=1 sampled=20 dimensions=20 primary_hit=19 material_match=18 depth_match=18 integrated=18 covered=0 fault=18 sparse_step=16'
    & "$PSScriptRoot/Collect-PT-Primary-M12R5.ps1" -LogDirectory $logs -OutputDirectory $out
    $zero=Summary
    if($zero.SawGPUCoveredSamples){throw 'Zero GPU coverage was reported as success.'}
    Add-Content -LiteralPath $log 'PT_PRIMARY_GPU frame=34 readback=1 armed=1 sampled=20 dimensions=20 primary_hit=19 material_match=18 depth_match=18 integrated=18 covered=17 fault=1 sparse_step=16'
    & "$PSScriptRoot/Collect-PT-Primary-M12R5.ps1" -LogDirectory $logs -OutputDirectory $out
    $gpu=Summary
    if($gpu.LastGPURecord -isnot [string]){throw 'GPU record must be a plain string.'}
    if(-not $gpu.SawGPUCoveredSamples -or $gpu.LastGPURecord -notmatch 'covered=17'){throw 'GPU coverage record was lost.'}
    Write-Host 'PASS: M12R5 collector distinguishes CPU submission, zero GPU coverage and covered GPU samples.'
} finally {Remove-Item -LiteralPath $root -Recurse -Force}
