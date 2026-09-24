#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Build-Metadata.ps1')
$Version=$ControlFGBuild.Version
$PublicVersion='2.1.0'
$RuntimeNames=@('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll','sl.dlss_d.dll','nvngx_dlssd.dll')
try {
    $validationPath=Join-Path $PSScriptRoot 'build\build-validation.json'
    $proxy=Join-Path $PSScriptRoot 'build\dxgi.dll'
    if (-not (Test-Path -LiteralPath $validationPath) -or -not (Test-Path -LiteralPath $proxy)) { throw 'Run Build.cmd successfully first.' }
    $v=Get-Content -LiteralPath $validationPath -Raw | ConvertFrom-Json
    Assert-ControlFGBuildValidation $v
    if ((Get-FileHash -LiteralPath $proxy -Algorithm SHA256).Hash -ne $v.SHA256) { throw 'dxgi.dll no longer matches build-validation.json.' }

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-Streamline.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Pinned Streamline validation failed before packaging.' }
    if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build/ControlFG.RTX40MFG.dll') -Algorithm SHA256).Hash -ne $v.MFGSidecarSHA256) { throw 'MFG sidecar changed after build validation.' }
    $sdk=Join-Path $PSScriptRoot 'third_party\streamline\bin'
    $releaseRoot=Join-Path $PSScriptRoot 'release'
    $out=Join-Path $releaseRoot ('Control-FG-v'+$PublicVersion)
    $publicZip=Join-Path $releaseRoot ('Control-FG-v'+$PublicVersion+'.zip')
    $legacyZip=Join-Path $PSScriptRoot ('Control-FG-v'+$PublicVersion+'-DropIn.zip')
    if (Test-Path -LiteralPath $out) { Remove-Item -LiteralPath $out -Recurse -Force }
    if (Test-Path -LiteralPath $publicZip) { Remove-Item -LiteralPath $publicZip -Force }
    if (Test-Path -LiteralPath $legacyZip) { Remove-Item -LiteralPath $legacyZip -Force }
    New-Item -ItemType Directory -Path (Join-Path $out 'ControlFGStreamline') -Force | Out-Null

    Copy-Item -LiteralPath $proxy -Destination (Join-Path $out 'dxgi.dll')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build/ControlFG.RTX40MFG.dll') -Destination (Join-Path $out 'ControlFGStreamline/ControlFG.RTX40MFG.dll')
    foreach($name in $RuntimeNames) {
        $src=Join-Path $sdk $name
        if (-not (Test-Path -LiteralPath $src)) { throw ('Missing Streamline runtime: '+$name) }
        Copy-Item -LiteralPath $src -Destination (Join-Path (Join-Path $out 'ControlFGStreamline') $name)
    }

    foreach ($doc in @('Collect-Alignment-Logs.cmd','Collect-Alignment-Logs.ps1','KNOWN_ISSUES.md','README.md','INSTALL.md','TROUBLESHOOTING.md','RELEASE_NOTES.md','Collect-ControlFG-Compact-Logs.cmd','Collect-ControlFG-Compact-Logs.ps1')) {
        $src=Join-Path $PSScriptRoot $doc
        if (-not (Test-Path -LiteralPath $src)) { throw ('Missing public release document: '+$doc) }
        Copy-Item -LiteralPath $src -Destination (Join-Path $out $doc)
    }
    New-Item -ItemType Directory -Path (Join-Path $out 'legal') -Force | Out-Null
    foreach ($doc in @('STREAMLINE-LICENSE.txt','RENODX-LICENSE.txt','RESHade-LICENSE.txt','THIRD-PARTY-NOTICES.md','RTX40MFG-MINIMAL-LICENSE.txt','RTX40MFG-UPSTREAM-LICENSE.txt')) {
        $src=Join-Path (Join-Path $PSScriptRoot 'legal') $doc
        if (-not (Test-Path -LiteralPath $src)) { throw ('Missing legal notice: '+$doc) }
        Copy-Item -LiteralPath $src -Destination (Join-Path (Join-Path $out 'legal') $doc)
    }

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-RRRuntime.ps1') -RuntimeDirectory (Join-Path $out 'ControlFGStreamline')
    if ($LASTEXITCODE -ne 0) { throw 'Packaged RR runtime verification failed.' }
    New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
    Compress-Archive -Path (Join-Path $out '*') -DestinationPath $publicZip -CompressionLevel Optimal
    Copy-Item -LiteralPath $publicZip -Destination $legacyZip

    $sumPath=Join-Path $releaseRoot 'SHA256SUMS.txt'
    $records=@()
    $records += ('{0}  {1}' -f (Get-FileHash -LiteralPath $publicZip -Algorithm SHA256).Hash.ToLowerInvariant(), (Split-Path -Leaf $publicZip))
    $records += ('{0}  ControlFGStreamline/ControlFG.RTX40MFG.dll' -f $v.MFGSidecarSHA256.ToLowerInvariant())
    $records += ('{0}  {1}' -f (Get-FileHash -LiteralPath $proxy -Algorithm SHA256).Hash.ToLowerInvariant(), 'dxgi.dll')
    foreach($name in $RuntimeNames) {
        $path=Join-Path $sdk $name
        $records += ('{0}  ControlFGStreamline/{1}' -f (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant(), $name)
    }
    $records | Set-Content -LiteralPath $sumPath -Encoding ASCII

    Write-Host ('PASS: public binary release: '+$publicZip)
    Write-Host ('PASS: compatibility drop-in ZIP: '+$legacyZip)
    Write-Host ('PASS: checksums: '+$sumPath)
} catch { Write-Error $_; exit 1 }
