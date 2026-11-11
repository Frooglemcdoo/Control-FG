#requires -Version 5.1
$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$file=Get-ChildItem -LiteralPath $root -Filter 'GI-Phase-R425-*.jsonl' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (!$file) { throw 'No phase log found. Launch using Start-GI-Phase-Check.cmd.' }
if ($file.Length -gt 8MB) { throw 'Phase log unexpectedly exceeds the bounded size.' }
$stage=Join-Path $PSScriptRoot ('GI-Phase-R425-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $stage | Out-Null
Copy-Item -LiteralPath $file.FullName -Destination $stage
$rows=@(Get-Content -LiteralPath $file.FullName | ForEach-Object { $_ | ConvertFrom-Json })
$status=$rows | Where-Object { $_.status } | Select-Object -Last 1
$phase=@($rows | Where-Object { $_.kind -eq 1 -or $_.kind -eq 2 })
[ordered]@{File=$file.Name;SHA256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash;Status=$status;PhaseRecords=$phase.Count;Purpose='CPU command recording/submission order only; not GPU completion or a noise fix'} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $stage 'collection.json') -Encoding UTF8
Compress-Archive -LiteralPath (Join-Path $stage $file.Name),(Join-Path $stage 'collection.json') -DestinationPath ($stage+'.zip')
Write-Host ('Send: '+$stage+'.zip')
if (!$phase.Count) { Write-Warning 'No GI phases observed. Send the ZIP; do not repeat a visual capture.' }
