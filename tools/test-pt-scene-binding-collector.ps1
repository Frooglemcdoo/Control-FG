$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP ('M11-Collector-Test-'+[guid]::NewGuid().ToString('N'))
$oldLocal=$env:LOCALAPPDATA
New-Item -ItemType Directory -Path (Join-Path $root 'ControlFGProbe') | Out-Null
try {
    $env:LOCALAPPDATA=$root
    Copy-Item (Join-Path $PSScriptRoot 'Collect-PT-Scene-M11.ps1') $root
    $lines=@(
      '{"kind":"header","schema":1}',
      '{"kind":1}',
      '{"kind":2}',
      '{"kind":3,"geometry_type":0,"flags":0,"vertex_format":6,"index_format":57,"stride":12}',
      '{"kind":3,"geometry_type":0,"flags":32,"vertex_format":6,"index_format":57,"stride":12}',
      '{"kind":4}',
      '{"kind":5,"count":9000,"vertex_count":8191}',
      '{"kind":6}',
      '{"kind":7}',
      '{"kind":8}',
      '{"kind":9}',
      '{"kind":10}',
      '{"kind":11,"geometry_flags":5}',
      '{"kind":11,"geometry_flags":3}',
      '{"kind":11,"geometry_flags":6}',
      '{"kind":"topology","uploads":2,"matched":3,"renderer_ready":false}',
      '{"kind":"registry","retained":3,"candidate":4,"renderer_ready":false}',
      '{"kind":"end","inline_calls":3,"worker_calls":7,"invalid_counts":1,"calls":2,"records":65536,"dropped":17,"write_ok":true}'
    )
    $lines | Set-Content (Join-Path $root 'ControlFGProbe/pt-scene-m11-123-999.jsonl') -Encoding UTF8
    'fixture' | Set-Content (Join-Path $root 'ControlFGProbe/probe-20261012-000000-000-999.log')
    & (Join-Path $root 'Collect-PT-Scene-M11.ps1')
    $zip=Get-ChildItem $root -Filter '*.zip' | Select-Object -First 1
    if (-not $zip) {throw 'No archive created'}
    Expand-Archive -LiteralPath $zip.FullName -DestinationPath (Join-Path $root 'result')
    $s=Get-Content (Join-Path $root 'result/Scene-M11-Summary.json') -Raw | ConvertFrom-Json
    if ($s.FreshTriangles -ne 1 -or $s.UpdatedTriangles -ne 1 -or $s.ReadErrors -ne 1 -or -not $s.LimitReached -or $s.Dropped -ne 17 -or -not $s.Complete -or -not $s.WriteOK) {throw 'Wrong summary'}
    if ($s.InlineCalls -ne 3 -or $s.WorkerCalls -ne 7 -or $s.InvalidCounts -ne 1) {throw 'Missing route counters'}
    if ($s.LeaseValidated -ne 1 -or $s.LeaseRejected -ne 1 -or $s.LeaseMismatched -ne 1) {throw 'Wrong lease summary'}
    if ($s.ResourceCreates -ne 1 -or $s.ResourceReleases -ne 1 -or $s.ResourceReadErrors -ne 1) {throw 'Wrong resource counters'}
    if ($s.BindingTables -ne 1 -or $s.BindingRecords -ne 1 -or $s.BindingReadErrors -ne 1 -or $s.TruncatedTables -ne 1) {throw 'Wrong binding summary'}
    if (-not (Test-Path (Join-Path $root 'result/probe-20261012-000000-000-999.log'))) {throw 'Missing matching probe'}
    if ($s.Registry.retained -ne 3 -or $s.Registry.candidate -ne 4 -or $s.Registry.renderer_ready) {throw 'Wrong registry summary'}
    if ($s.Topology.uploads -ne 2 -or $s.Topology.matched -ne 3 -or $s.Topology.renderer_ready) {throw 'Wrong topology summary'}
    Write-Host 'PASS M11 collector: archive, matching probe, fresh/update counts, errors, cap and dropped records.'
} finally {$env:LOCALAPPDATA=$oldLocal;Remove-Item $root -Recurse -Force}
