$ErrorActionPreference='Stop'
$dir=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$record=Get-ChildItem -LiteralPath $dir -Filter 'pt-scene-m9-*.jsonl' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $record) { throw 'No M9 geometry log found. Run Control with the M9 dxgi.dll first.' }
$runPid=($record.BaseName -split '-')[-1]
$stage=Join-Path $env:TEMP ('PT-Scene-M9-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    Copy-Item -LiteralPath $record.FullName -Destination $stage
    Get-ChildItem -LiteralPath $dir -Filter ('probe-*-'+$runPid+'.log') -File | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stage }
    $summary=[ordered]@{Build='M9';Calls=$null;BottomLevelBuilds=0;TopLevelBuilds=0;GeometryRecords=0;InlineCalls=$null;WorkerCalls=$null;InvalidCounts=$null;LeaseValidated=0;LeaseRejected=0;LeaseMismatched=0;ResourceCreates=0;ResourceReleases=0;ResourceReadErrors=0;BindingTables=0;BindingRecords=0;BindingReadErrors=0;TruncatedTables=0;FreshTriangles=0;UpdatedTriangles=0;ReadErrors=0;Complete=$false;LimitReached=$false;Dropped=$null;WriteOK=$null;Formats=@{};Scope='CPU recording-time metadata; historical geometry/binding samples, not GPU completion or lifetime verification'}
    Get-Content -LiteralPath $record.FullName | ForEach-Object {
        if ($_) {
            $e=$_ | ConvertFrom-Json
            switch ([string]$e.kind) {
                '1' {$summary.BottomLevelBuilds++}
                '2' {$summary.TopLevelBuilds++}
                '3' {
                    $summary.GeometryRecords++
                    if ($e.geometry_type -eq 0) {
                        if (($e.flags -band 32) -ne 0) {$summary.UpdatedTriangles++} else {$summary.FreshTriangles++}
                        $key='vertex='+$e.vertex_format+',index='+$e.index_format+',stride='+$e.stride
                        if (-not $summary.Formats.ContainsKey($key)) {$summary.Formats[$key]=0}
                        $summary.Formats[$key]++
                    }
                }
                '4' {$summary.ReadErrors++}
                '5' {$summary.BindingTables++;if ($e.vertex_count -lt $e.count) {$summary.TruncatedTables++}}
                '6' {$summary.BindingRecords++}
                '7' {$summary.BindingReadErrors++}
                '8' {$summary.ResourceCreates++}
                '9' {$summary.ResourceReleases++}
                '10' {$summary.ResourceReadErrors++}
                '11' {if ($e.geometry_flags -eq 5) {$summary.LeaseValidated++} elseif ($e.geometry_flags -eq 6) {$summary.LeaseMismatched++} else {$summary.LeaseRejected++}}
                'end' {$summary.InlineCalls=$e.inline_calls;$summary.WorkerCalls=$e.worker_calls;$summary.InvalidCounts=$e.invalid_counts;$summary.Complete=$true;$summary.LimitReached=($e.dropped -gt 0);$summary.Calls=$e.calls;$summary.Dropped=$e.dropped;$summary.WriteOK=$e.write_ok}
            }
        }
    }
    if (-not $summary.Complete) {Write-Warning 'Capture has no final record. Totals are unknown; retained rows remain usable. Send this archive as-is.'}
    $summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'Scene-M9-Summary.json') -Encoding UTF8
    $zip=Join-Path $PSScriptRoot ('PT-Scene-M9-Results-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.zip')
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
    $summary | ConvertTo-Json -Depth 6 | Write-Host
    Write-Host "Upload: $zip"
} finally {Remove-Item -LiteralPath $stage -Recurse -Force}
