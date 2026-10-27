param([switch]$DiagnosticsOnly, [switch]$CollectExisting, [int]$GameProcessId=0, [switch]$RequireRR)
$ErrorActionPreference = 'Stop'
$games = @(Get-Process -Name 'Control_DX12' -ErrorAction SilentlyContinue)
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
if ($GameProcessId -gt 0) { $gameId = $GameProcessId }
elseif ($games.Count -eq 1) { $gameId = $games[0].Id }
elseif ($DiagnosticsOnly -and $games.Count -eq 0) {
    $latest = Get-ChildItem -LiteralPath $root -File -Filter 'probe-*.log' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $latest -or $latest.Name -notmatch '-(\d+)\.log$') { throw 'No Control probe log found.' }
    $gameId = [int]$Matches[1]
    Write-Host "Game closed: collecting the most recent logged session, PID $gameId."
}
else { throw 'Start exactly one Control DX12 game session before running this helper.' }
$existing = @(Get-ChildItem -LiteralPath $root -Directory -Filter "rr-input-c1-$gameId-*" -ErrorAction SilentlyContinue)
if ($existing.Count -and -not $DiagnosticsOnly -and -not $CollectExisting) { throw 'This game session already has RR captures. Restart Control once before this two-position test.' }
$collector = Join-Path $PSScriptRoot 'collect-rr-boundary.py'
if (-not (Test-Path -LiteralPath $collector)) { $collector = Join-Path $PSScriptRoot 'tools\collect-rr-boundary.py' }
$output = [Environment]::GetFolderPath('Desktop')
$collectorArgs = @($collector, '--pid', $gameId, '--output', $output)
if ($DiagnosticsOnly) { $collectorArgs += '--diagnostics-only' }
if ($CollectExisting) { $collectorArgs += '--collect-existing' }
if ($RequireRR) { $collectorArgs += '--require-rr' }
if (Get-Command py -ErrorAction SilentlyContinue) { & py -3 @collectorArgs }
else { & python @collectorArgs }
if ($LASTEXITCODE -ne 0) { throw "RR boundary capture failed with exit code $LASTEXITCODE. Keep the output folder." }
