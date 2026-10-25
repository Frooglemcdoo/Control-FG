$ErrorActionPreference = 'Stop'
$games = @(Get-Process -Name 'Control_DX12' -ErrorAction SilentlyContinue)
if ($games.Count -ne 1) { throw 'Start exactly one Control DX12 game session before running this helper.' }
$gameId = $games[0].Id
$root = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$existing = @(Get-ChildItem -LiteralPath $root -Directory -Filter "rr-input-c1-$gameId-*" -ErrorAction SilentlyContinue)
if ($existing.Count) { throw 'This game session already has RR captures. Restart Control once before this two-position test.' }
$collector = Join-Path $PSScriptRoot 'collect-rr-boundary.py'
if (-not (Test-Path -LiteralPath $collector)) { $collector = Join-Path $PSScriptRoot 'tools\collect-rr-boundary.py' }
$output = [Environment]::GetFolderPath('Desktop')
if (Get-Command py -ErrorAction SilentlyContinue) { & py -3 $collector --pid $gameId --output $output }
else { & python $collector --pid $gameId --output $output }
if ($LASTEXITCODE -ne 0) { throw "RR boundary capture failed with exit code $LASTEXITCODE. Keep the output folder." }
