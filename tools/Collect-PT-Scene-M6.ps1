$ErrorActionPreference='Stop'
$dir=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$record=Get-ChildItem -LiteralPath $dir -Filter 'pt-scene-m6-*.jsonl' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $record) { throw 'No M6 geometry log found. Run Control with the M6 dxgi.dll first.' }
$runPid=($record.BaseName -split '-')[-1]
$stage=Join-Path $env:TEMP ('PT-Scene-M6-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    Copy-Item -LiteralPath $record.FullName -Destination $stage
    Get-ChildItem -LiteralPath $dir -Filter ('probe-*-'+$runPid+'.log') -File | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stage }
    $summary=[ordered]@{Build='M6';Calls=0;BottomLevelBuilds=0;TopLevelBuilds=0;GeometryRecords=0;FreshTriangles=0;UpdatedTriangles=0;ReadErrors=0;Complete=$false;LimitReached=$false;Dropped=0;WriteOK=$null;Formats=@{};Scope='CPU recording-time metadata; not GPU completion or verified SBT bindings'}
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
                'end' {$summary.Complete=$true;$summary.LimitReached=($e.records -ge 65536);$summary.Calls=$e.calls;$summary.Dropped=$e.dropped;$summary.WriteOK=$e.write_ok}
            }
        }
    }
    $summary | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'Scene-M6-Summary.json') -Encoding UTF8
    $zip=Join-Path $PSScriptRoot ('PT-Scene-M6-Results-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.zip')
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
    $summary | ConvertTo-Json -Depth 6 | Write-Host
    Write-Host "Upload: $zip"
} finally {Remove-Item -LiteralPath $stage -Recurse -Force}
