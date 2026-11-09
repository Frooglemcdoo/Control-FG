#requires -Version 5.1
$ErrorActionPreference='Stop'
try {
    $root = Join-Path $PSScriptRoot 'third_party\streamline'
    $infoPath = Join-Path $root 'sdk-info.json'
    if (-not (Test-Path -LiteralPath $infoPath)) { throw 'Streamline SDK is not staged. Run Fetch-Streamline.cmd first.' }
    $info = Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json
    if ($info.Version -ne '2.14.1' -or $info.SourceArchiveSHA256 -ne '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B') { throw 'Wrong Streamline SDK release. Run Fetch-Streamline.cmd again.' }
    foreach ($entry in $info.Files) {
        $path = Join-Path $root ($entry.Path -replace '/', '\')
        if (-not (Test-Path -LiteralPath $path)) { throw ('Missing staged SDK file: ' + $entry.Path) }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.SHA256) { throw ('Staged SDK file hash changed: ' + $entry.Path) }
    }
    foreach ($name in @('sl.h','sl_consts.h','sl_core_api.h','sl_core_types.h','sl_reflex.h','sl_dlss_g.h','sl_dlss_d.h','sl_pcl.h')) {
        if (-not (Test-Path -LiteralPath (Join-Path $root ('include\' + $name)))) { throw ('Missing SDK header: ' + $name) }
    }
    $constsHeader = Get-Content -LiteralPath (Join-Path $root 'include\sl_consts.h') -Raw
    if ($constsHeader -match '\brenderingGameFrames\b') { throw 'Unexpected Streamline sl::Constants ABI for the pinned 2.14.1 package.' }
    foreach ($name in @('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll','sl.dlss_d.dll','nvngx_dlssd.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $root ('bin\' + $name)))) { throw ('Missing production SDK runtime: ' + $name) }
    }
    Write-Host 'PASS: Streamline SDK 2.14.1 staging and recorded hashes are intact.'
} catch { Write-Error $_; exit 1 }
