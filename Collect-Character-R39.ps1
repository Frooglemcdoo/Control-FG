$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$captures=@(Get-ChildItem -LiteralPath $root -Directory -Filter 'character-r39-*' | Where-Object {Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json')} | Sort-Object LastWriteTime -Descending)
if(-not $captures.Count){throw 'No R39 capture found. Stand at the glass reflection, press Ctrl+Shift+F7 once, and wait 10 seconds.'}
$latest=Get-Content -Raw -LiteralPath (Join-Path $captures[0].FullName 'metadata.json') | ConvertFrom-Json
$selected=@($captures | Where-Object {
    $m=Get-Content -Raw -LiteralPath (Join-Path $_.FullName 'metadata.json') | ConvertFrom-Json
    $m.pid -eq $latest.pid
})
$logs=@(Get-ChildItem -LiteralPath $root -Recurse -File -Filter "probe-*-$($latest.pid).log" -ErrorAction SilentlyContinue)
$paths=@($selected | ForEach-Object {$_.FullName})+@($logs | ForEach-Object {$_.FullName})
$out=Join-Path ([Environment]::GetFolderPath('Desktop')) ('Control-Character-R39-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.zip')
Compress-Archive -LiteralPath $paths -DestinationPath $out
Write-Host "Saved: $out"
Write-Host "Captured stages: $(@($latest.records | Select-Object -ExpandProperty kind -Unique).Count)/7; rejected reads: $($latest.rejected)"
Write-Host 'Send this ZIP even if stages are missing. This build intentionally keeps the R38 image unchanged.'
