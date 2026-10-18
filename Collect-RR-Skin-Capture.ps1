$ErrorActionPreference = 'Stop'
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$all = @(Get-ChildItem -LiteralPath $root -Directory -Filter 'rr-input-c1-*' | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') } | Sort-Object LastWriteTime -Descending)
if (-not $all.Count) { throw 'No completed F9 capture found. Keep RR on, press F9 once and wait 15 seconds.' }
$latest = Get-Content -Raw -LiteralPath (Join-Path $all[0].FullName 'metadata.json') | ConvertFrom-Json
$capturePid = $latest.pid
$captures = @($all | Where-Object { $_.Name -like "rr-input-c1-$capturePid-*" })
$shaders = @(Get-ChildItem -LiteralPath $root -Directory -Filter "rr-hair-shaders-$capturePid-*")
$paths = @($captures | ForEach-Object { $_.FullName }) + @($shaders | ForEach-Object { $_.FullName })
$out = Join-Path ([Environment]::GetFolderPath('Desktop')) ('Control-RR-Hair-Audit-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip')
Compress-Archive -LiteralPath $paths -DestinationPath $out
Write-Host "Saved $($captures.Count) snapshots and $($shaders.Count) shader folders: $out"
