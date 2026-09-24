#requires -Version 5.1
param([string]$LogDirectory='', [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if (-not $LogDirectory) { $LogDirectory=Join-Path $env:LOCALAPPDATA 'ControlFGProbe' }
if (-not $OutputDirectory) { $OutputDirectory=$PSScriptRoot }
$selected=$null
foreach ($f in @(Get-ChildItem -LiteralPath $LogDirectory -Filter 'probe-*.log' -File | Sort-Object LastWriteTimeUtc -Descending)) {
    $header=Get-Content -LiteralPath $f.FullName -TotalCount 8
    if (($header -join "`n") -match 'internal_build=2\.0\.0-Clean-Native-R12-MFG-Dynamic-Test(?:\s|$)') { $selected=$f; break }
}
if (-not $selected) { throw 'No Clean Native R12 MFG Dynamic Test log found. Build, install and run this diagnostic first.' }
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$stage=Join-Path $OutputDirectory ('Alignment-Logs-'+$stamp+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
[void](New-Item -ItemType Directory -Path $stage)
Copy-Item -LiteralPath $selected.FullName -Destination $stage
if ($selected.BaseName -notmatch '-(\d+)$') { throw 'Cannot determine capture process ID from log filename.' }
$captureDirectory=Join-Path $LogDirectory ('Native-R12-MFG-Dynamic-Test-'+$Matches[1])
if (Test-Path -LiteralPath $captureDirectory -PathType Container) { Copy-Item -LiteralPath $captureDirectory -Destination $stage -Recurse } else { 'No pixel samples saved; inspect PIXEL_REQUEST/ABORT/INVALID in the included log.' | Set-Content -LiteralPath (Join-Path $stage 'CAPTURE-MISSING.txt'); Write-Warning 'No samples saved. Send this ZIP so the capture failure can be diagnosed.' }

foreach ($name in @('Build.log','Build-Metadata.ps1','README.md')) {
    $source=Join-Path $PSScriptRoot $name
    if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $stage }
}
$zip=$stage+'.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $zip
Write-Host ('Send this ZIP: '+$zip)
