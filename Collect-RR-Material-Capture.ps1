$ErrorActionPreference = 'Stop'
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$latest = Get-ChildItem -LiteralPath $root -Directory -Filter 'rr-material-shaders-*' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $latest) { throw 'No material shader capture found. With RR ON and GI FILTER selected, close F10, press F9 and wait 20 seconds.' }
if ($latest.Name -notmatch '^rr-material-shaders-(\d+)-(\d+)$') { throw 'Unexpected capture directory name.' }
$capturePid = $Matches[1]
$paths = @($latest.FullName)
$inputs = @(Get-ChildItem -LiteralPath $root -Directory -Filter "rr-input-c1-$capturePid-*" | Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') })
$paths += @($inputs | ForEach-Object { $_.FullName })
$paths += @(Get-ChildItem -LiteralPath $root -Recurse -File -Filter "probe-*-$capturePid.log" | ForEach-Object { $_.FullName })
foreach ($base in @($env:LOCALAPPDATA, [IO.Path]::GetTempPath())) {
    $diagnostics = Join-Path $base 'Control-FG\Diagnostics'
    if (Test-Path -LiteralPath $diagnostics) {
        $paths += @(Get-ChildItem -LiteralPath $diagnostics -File -Filter "Lighting-R101-*-$capturePid-*.log" | ForEach-Object { $_.FullName })
    }
}
$paths = @($paths | Select-Object -Unique)
$complete = @(Get-ChildItem -LiteralPath $latest.FullName -File -Filter '*.complete').Count
$standard = @(Get-ChildItem -LiteralPath $latest.FullName -File -Filter 'family-1-*.complete').Count
$out = Join-Path ([Environment]::GetFolderPath('Desktop')) ('Control-RR-Material-R101-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip')
Compress-Archive -LiteralPath $paths -DestinationPath $out
Write-Host "Saved: $out"
Write-Host "Complete shader pairs: $complete; Standard material pairs: $standard; RR input frames: $($inputs.Count)"
if (-not $standard) { Write-Warning 'No complete Standard material pairs; send this ZIP and the saved Lighting-R101 log so capture admission can be checked.' }
if ($inputs.Count -lt 2) { Write-Warning 'RR input capture is incomplete; send the ZIP anyway. Shader evidence is still useful.' }
