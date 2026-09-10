#requires -Version 5.1
$ErrorActionPreference='Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $root 'assets\control_fg_logo.bgra'
$parts = Get-ChildItem -LiteralPath (Join-Path $root 'assets') -Filter 'control_fg_logo.bgra.b64.part*' | Sort-Object Name
if (-not $parts) { throw 'Missing embedded Control FG logo data parts.' }
$b64 = ($parts | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw }) -join ''
[IO.File]::WriteAllBytes($out,[Convert]::FromBase64String($b64))
Write-Host 'PASS: reconstructed assets\control_fg_logo.bgra'
