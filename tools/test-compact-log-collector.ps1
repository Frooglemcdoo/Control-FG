#requires -Version 5.1
$ErrorActionPreference='Stop'
$root=Join-Path ([IO.Path]::GetTempPath()) ('Control collector fixture '+[guid]::NewGuid().ToString('N'))
$collector=Join-Path $PSScriptRoot '../Collect-ControlFG-Compact-Logs.ps1'
try {
    foreach ($case in @('legacy','character','startup','invalid')) {
        $dir=Join-Path $root $case
        $logs=Join-Path $dir 'logs'
        $out=Join-Path $dir 'out'
        [void][IO.Directory]::CreateDirectory($logs)
        $header=switch ($case) {
            'legacy' { '00 PROBE v2.1.2 internal_build=test ' }
            'character' { '00 RR_CHARACTER_SUPPORT_R418 build=R425 lifecycle_logging=1 ' }
            'startup' { '00 GI_FROZEN_R426 startup=1 requested=1 ' }
            'invalid' { '00 GI_FROZEN_R426_EXTRA startup=1 unrelated=1 ' }
        }
        $name='probe-20261112-160755-123-1234.log'
        Set-Content -LiteralPath (Join-Path $logs $name) -Value $header -Encoding UTF8
        # A matching header with an invalid filename must never be selected.
        Set-Content -LiteralPath (Join-Path $logs 'probe-unrelated.log') -Value '00 PROBE v9.9 unrelated=1 ' -Encoding UTF8
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $collector -LogDirectory $logs -ProjectDirectory $dir -OutputDirectory $out
        $code=$LASTEXITCODE
        if ($case -eq 'invalid') {
            if ($code -eq 0) { throw 'Unrelated log accepted' }
            continue
        }
        if ($code -ne 0) { throw "Collection failed: $case" }
        $zips=@(Get-ChildItem -LiteralPath $out -Filter '*.zip')
        if ($zips.Count -ne 1) { throw "Expected one ZIP: $case" }
        $extracted=Join-Path $dir 'extracted'
        Expand-Archive -LiteralPath $zips[0].FullName -DestinationPath $extracted
        $report=Get-Content -LiteralPath (Join-Path $extracted 'collection-report.json') -Raw | ConvertFrom-Json
        if (@($report.ProbeLogs).Count -ne 1 -or $report.ProbeLogs[0] -ne $name) { throw 'Incorrect selected logs' }
        if ($report.MatchingProcessIds[0] -ne '1234') { throw 'Incorrect PID' }
        if ($case -ne 'legacy' -and $report.Version -notlike 'lifecycle-*') { throw 'Fallback version mislabeled' }
        if ((Get-Content -LiteralPath (Join-Path $extracted ('probe/'+$name)) -Raw).Trim() -ne $header.Trim()) { throw 'Log contents changed' }
        Write-Host "PASS compact collector $case"
    }
    Write-Host 'PASS compact collector rejects unrelated logs'
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
