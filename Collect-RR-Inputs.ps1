#requires -Version 5.1
$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$captures=@(Get-ChildItem -LiteralPath $root -Directory -Filter 'rr-input-c1-*' | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') } | Sort-Object LastWriteTime -Descending)
if (-not $captures.Count) { throw 'No completed C1 captures. In gameplay select F, press F9, wait five seconds, then collect again.' }
$latest=Get-Content -LiteralPath (Join-Path $captures[0].FullName 'metadata.json') -Raw | ConvertFrom-Json
$selected=@($captures | Where-Object { $_.Name -like ('rr-input-c1-'+$latest.pid+'-*') } | Select-Object -First 4)
$output=Join-Path $PSScriptRoot ('RR-Inputs-C1-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
[void](New-Item -ItemType Directory -Path $output)
foreach ($capture in $selected) { Copy-Item -LiteralPath $capture.FullName -Destination $output -Recurse }
& (Join-Path $PSScriptRoot 'Collect-ControlFG-Compact-Logs.ps1') -OutputDirectory $output
$zip=$output+'.zip'
Compress-Archive -LiteralPath $output -DestinationPath $zip -CompressionLevel Optimal
Write-Host ('Send this file: '+$zip)
