$ErrorActionPreference = 'Stop'
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$capture = Get-ChildItem -LiteralPath $root -Directory -Filter 'rr-input-c1-*' | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $capture) { throw 'No completed F9 capture found. Keep RR on, press F9 in gameplay, and wait several seconds.' }
$out = Join-Path ([Environment]::GetFolderPath('Desktop')) ('Control-RR-Skin-Capture-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip')
Compress-Archive -LiteralPath $capture.FullName -DestinationPath $out
Write-Host "Capture saved: $out"
