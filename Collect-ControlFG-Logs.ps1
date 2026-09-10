#requires -Version 5.1
$ErrorActionPreference='Stop'
try {
    $logDir = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    if (-not (Test-Path -LiteralPath $logDir)) { throw 'No Control FG log directory exists yet. Run Control with the mod first.' }
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $stage = Join-Path $env:TEMP ('ControlFG-PublicLogs-' + [guid]::NewGuid().ToString('N'))
    $zip = Join-Path $PSScriptRoot ('Control-FG-Logs-' + $stamp + '.zip')
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    try {
        $probeDir = Join-Path $stage 'probe'
        $slDirOut = Join-Path $stage 'streamline'
        New-Item -ItemType Directory -Path $probeDir,$slDirOut -Force | Out-Null
        $probeLogs = @(Get-ChildItem -LiteralPath $logDir -File -Filter 'probe-*.log' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 5)
        foreach ($f in $probeLogs) { Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $probeDir $f.Name) -Force }
        $slRuns = @(Get-ChildItem -LiteralPath $logDir -Directory -Filter 'Streamline-v1.0.0-*' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 3)
        foreach ($run in $slRuns) {
            $dest = Join-Path $slDirOut $run.Name
            New-Item -ItemType Directory -Path $dest -Force | Out-Null
            Get-ChildItem -LiteralPath $run.FullName -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 30 | ForEach-Object {
                Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $dest $_.Name) -Force
            }
        }
        $settings = Join-Path (Join-Path $env:LOCALAPPDATA 'ControlFG') 'settings.ini'
        if (Test-Path -LiteralPath $settings) { Copy-Item -LiteralPath $settings -Destination (Join-Path $stage 'settings.ini') -Force }
        if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
        Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
    } finally {
        if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue }
    }
    Write-Host ('PASS: created ' + $zip)
} catch { Write-Error $_; exit 1 }
