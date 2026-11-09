$ErrorActionPreference = 'Stop'
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$all = @(Get-ChildItem -LiteralPath $root -Directory -Filter 'rr-input-c1-*' | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') } | Sort-Object LastWriteTime -Descending)
$items = @($all | ForEach-Object { $m=Get-Content -Raw -LiteralPath (Join-Path $_.FullName 'metadata.json') | ConvertFrom-Json; if($m.schema -eq 'control-rr-gi-r28'){[PSCustomObject]@{Directory=$_;Metadata=$m}} })
if (-not $items.Count) { throw 'No completed R28 GI capture found. Keep RR on, press F9 once and wait 20 seconds.' }
$latest=$items[0].Metadata
$pair=@($items | Where-Object {$_.Metadata.pid -eq $latest.pid -and $_.Metadata.capture_ordinal -eq $latest.capture_ordinal} | Sort-Object {$_.Metadata.pair_index})
$shaders=@(Get-ChildItem -LiteralPath $root -Directory | Where-Object { $_.Name -like "rr-hair-shaders-$($latest.pid)-*" -or $_.Name -like "rr-material-shaders-$($latest.pid)-*" })
$logs=@(Get-ChildItem -LiteralPath $root -Recurse -File -Filter "probe-*-$($latest.pid).log" -ErrorAction SilentlyContinue)
$report=[ordered]@{schema='control-rr-gi-r28-report';pid=$latest.pid;pair=$latest.capture_ordinal;frame_count=$pair.Count;consecutive=($pair.Count -eq 2 -and $pair[1].Metadata.consecutive);output_complete=($pair.Count -eq 2 -and @($pair | Where-Object {-not $_.Metadata.output_captured}).Count -eq 0);frames=@($pair | ForEach-Object {$_.Metadata})}
$reportPath=Join-Path $root "rr-gi-r28-report-$($latest.pid)-$($latest.capture_ordinal).json"
$report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $reportPath -Encoding UTF8
$paths=@($pair | ForEach-Object {$_.Directory.FullName})+@($shaders | ForEach-Object {$_.FullName})+@($logs | ForEach-Object {$_.FullName})+@($reportPath)
$out=Join-Path ([Environment]::GetFolderPath('Desktop')) ('Control-RR-GI-Audit-R28-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.zip')
Compress-Archive -LiteralPath $paths -DestinationPath $out
Write-Host "Saved: $out"
Write-Host "Frames: $($pair.Count); consecutive: $($report.consecutive); RR output complete: $($report.output_complete)"
if(-not $report.consecutive -or -not $report.output_complete){Write-Warning 'Capture incomplete; send this ZIP anyway. Its report and logs identify the missing stage.'}


$giComplete=($pair.Count -eq 2 -and @($pair | Where-Object {-not (Test-Path -LiteralPath (Join-Path $_.Directory.FullName "raw-gi.rgba16f"))}).Count -eq 0)
Write-Host "Raw GI complete: $giComplete"
if(-not $giComplete){Write-Warning "Raw GI is missing; send the ZIP so its logs identify the rejected capture stage."}
