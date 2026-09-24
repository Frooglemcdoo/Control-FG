#requires -Version 5.1
[CmdletBinding()]
param([ValidateSet('Install', 'Uninstall', 'Collect')][string]$Action = 'Install', [string]$GamePath)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Build-Metadata.ps1')
$ProbeVersion = $ControlFGBuild.Version
$RuntimeNames = @('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll','sl.dlss_d.dll','nvngx_dlssd.dll')
try {
    if ($Action -eq 'Collect') {
        & (Join-Path $PSScriptRoot 'Collect-ControlFG-Logs.ps1')
        exit $LASTEXITCODE
    }

    if (Get-Process -Name 'Control_DX12', 'Control_DX11', 'Control' -ErrorAction SilentlyContinue) { throw 'Close Control before installing or removing this Control FG build.' }
    $receiptPath = Join-Path $PSScriptRoot 'installation.json'
    $receipt = $null
    if (Test-Path -LiteralPath $receiptPath) { $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json }
    if (-not $GamePath -and $receipt) { $GamePath = $receipt.GamePath }
    if (-not $GamePath) {
        Write-Host 'Browse to the Control install folder from Steam or Epic Games Store.'
        $GamePath = Read-Host 'Paste the folder containing Control_DX12.exe'
    }
    $GamePath = (Resolve-Path -LiteralPath $GamePath.Trim().Trim('"')).ProviderPath
    if (-not (Test-Path -LiteralPath (Join-Path $GamePath 'Control_DX12.exe'))) { throw 'This folder does not contain Control_DX12.exe.' }

    if ($Action -eq 'Uninstall') {
        if (-not $receipt -or $receipt.GamePath -ine $GamePath -or $receipt.Version -ne $ProbeVersion) { throw 'No matching Control FG installation receipt for this build. Do not delete unrelated files.' }
        # Validate every existing installed file before removing the first one.
        foreach ($file in $receipt.Files) {
            $path = Join-Path $GamePath ($file.RelativePath -replace '/', '\')
            if (Test-Path -LiteralPath $path) {
                $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
                if ($actual -ne $file.SHA256) { throw ('Installed file changed since installation and nothing was removed: ' + $file.RelativePath) }
            }
        }
        foreach ($file in $receipt.Files) {
            $path = Join-Path $GamePath ($file.RelativePath -replace '/', '\')
            if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
        }
        $runtimeDir = Join-Path $GamePath 'ControlFGStreamline'
        if (Test-Path -LiteralPath $runtimeDir) {
            if (@(Get-ChildItem -LiteralPath $runtimeDir -Force).Count -eq 0) { Remove-Item -LiteralPath $runtimeDir -Force }
            else { Write-Warning 'ControlFGStreamline was not removed because it contains unexpected files.' }
        }
        Remove-Item -LiteralPath $receiptPath -Force
        Write-Host ('Control FG v' + $ProbeVersion + ' removed. Original game files and user-supplied ReShade were not replaced or removed.')
        exit 0
    }

    if ($receipt) { throw 'This package already has an installation receipt. Run Uninstall.cmd before reinstalling.' }
    $proxyDestination = Join-Path $GamePath 'dxgi.dll'
    $runtimeDestination = Join-Path $GamePath 'ControlFGStreamline'
    if (Test-Path -LiteralPath $proxyDestination) { throw 'A dxgi.dll already exists. Uninstall the previously installed proxy first.' }
    if (Test-Path -LiteralPath $runtimeDestination) { throw 'A ControlFGStreamline folder already exists. It was not modified. Remove/rename it only if you know it belongs to an earlier Control FG test.' }
    if (Test-Path -LiteralPath (Join-Path $GamePath 'ControlFG-RenoDXClampRef.addon64')) { Write-Warning 'An archived Control FG reference add-on is still beside the game. the public release does not use it; remove it unless you are deliberately reproducing the old r21w diagnostic.' }
    if (Test-Path -LiteralPath (Join-Path $GamePath 'renodx-control-rr.addon64')) { Write-Warning 'The full RenoDX Control RR add-on is present. It can alter the same denoiser shader/NGX path; remove it when validating the public release.' }
    if (Test-Path -LiteralPath (Join-Path $GamePath 'd3d12.dll')) { Write-Warning 'A d3d12.dll is present (commonly ReShade). The public release does not require it; remove/rename it for an uncontaminated validation run.' }

    $proxySource = Join-Path $PSScriptRoot 'build\dxgi.dll'
    $validationPath = Join-Path $PSScriptRoot 'build\build-validation.json'
    if (-not (Test-Path -LiteralPath $proxySource) -or -not (Test-Path -LiteralPath $validationPath)) { throw 'Run Build.cmd successfully first.' }
    $validation = Get-Content -LiteralPath $validationPath -Raw | ConvertFrom-Json
    Assert-ControlFGBuildValidation $validation
    $proxyHash = (Get-FileHash -LiteralPath $proxySource -Algorithm SHA256).Hash
    if ($proxyHash -ne $validation.SHA256 -or $validation.AbiCheck -ne 'Passed') { throw 'The proxy DLL does not match its validated build. Run Build.cmd again.' }

    $manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'target-manifest.json') -Raw | ConvertFrom-Json
    $matchedTarget = $null
    if ($manifest.PSObject.Properties.Name -contains 'SupportedTargets') {
        foreach ($target in @($manifest.SupportedTargets)) {
            $targetMatches = $true
            foreach ($file in @($target.RequiredFiles)) {
                $path = Join-Path $GamePath $file.Name
                if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.SHA256) {
                    $targetMatches = $false
                    break
                }
            }
            if ($targetMatches) {
                $matchedTarget = $target
                break
            }
        }
        if ($null -eq $matchedTarget) { throw 'Unsupported or changed Control build. Steam, Epic and GOG targets are exact-triple locked.' }
        Write-Host ('Detected supported Control target: ' + $matchedTarget.Name + ' (' + $matchedTarget.Build + ')')
    } else {
        foreach ($file in $manifest.RequiredFiles) {
            $path = Join-Path $GamePath $file.Name
            if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.SHA256) { throw ('Unsupported or changed game file: ' + $file.Name) }
        }
    }

    $sdkRoot = Join-Path $PSScriptRoot 'third_party\streamline'
    $sdkInfoPath = Join-Path $sdkRoot 'sdk-info.json'
    if (-not (Test-Path -LiteralPath $sdkInfoPath)) { throw 'Streamline SDK is not staged. Run Fetch-Streamline.cmd, then Build.cmd.' }
    $sdkInfo = Get-Content -LiteralPath $sdkInfoPath -Raw | ConvertFrom-Json
    if ($sdkInfo.Version -ne '2.14.1' -or $sdkInfo.SourceArchiveSHA256 -ne '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B') { throw 'Staged Streamline SDK does not match the pinned official 2.14.1 release.' }

    $planned = @(
        [ordered]@{ RelativePath = 'dxgi.dll'; Source = $proxySource; SHA256 = $proxyHash }
    )
    foreach ($name in $RuntimeNames) {
        $source = Join-Path $sdkRoot ('bin\' + $name)
        if (-not (Test-Path -LiteralPath $source)) { throw ('Missing staged Streamline runtime: ' + $name) }
        $record = @($sdkInfo.Files | Where-Object { $_.Path -eq ('bin/' + $name) })
        if ($record.Count -ne 1) { throw ('sdk-info.json is missing a unique hash record for ' + $name) }
        $hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
        if ($hash -ne $record[0].SHA256) { throw ('Staged Streamline runtime hash changed: ' + $name) }
        $planned += [ordered]@{ RelativePath = ('ControlFGStreamline/' + $name); Source = $source; SHA256 = $hash }
    }

    $testSidecar=Join-Path $PSScriptRoot 'build/ControlFG.RTX40MFG.dll'
    $planned += [ordered]@{ RelativePath='ControlFGStreamline/ControlFG.RTX40MFG.dll'; Source=$testSidecar; SHA256=(Get-FileHash -LiteralPath $testSidecar -Algorithm SHA256).Hash }
    # Receipt is written before copies so any partial copy remains safely recoverable.
    [ordered]@{
        Version = $ProbeVersion
        GamePath = $GamePath
        StreamlineSDKVersion = '2.14.1'
        DLSSGGenerationEnabled = $true
        DLSSGGeneratedFramesRequested = 3
        TargetMultiplier = '4x'
        GameTarget = if ($matchedTarget) { [string]$matchedTarget.Name } else { 'Steam legacy manifest' }
        InstalledUtc = [DateTime]::UtcNow.ToString('o')
        Files = @($planned | ForEach-Object { [ordered]@{ RelativePath = $_.RelativePath; SHA256 = $_.SHA256 } })
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receiptPath -Encoding UTF8

    New-Item -ItemType Directory -Path $runtimeDestination | Out-Null
    foreach ($file in $planned) {
        $destination = Join-Path $GamePath ($file.RelativePath -replace '/', '\')
        [IO.File]::Copy($file.Source, $destination, $false)
    }
    Write-Host ('Installed Control FG v' + $ProbeVersion + ' production package. No ReShade/RenoDX reference add-on is part of the install.')
    Write-Host 'For a clean public-release validation run, remove/rename ReShade d3d12.dll and any archived ControlFG-RenoDXClampRef.addon64.'
    Write-Host 'Do not install the full RenoDX Control add-on and do not place loose DLSS/DLSSD runtimes beside Control_DX12.exe. Control FG continues to use ControlFGStreamline.'
    Write-Host 'Use 4K DLAA, FG OFF, RR ON. Preset F is the default; E remains available in the overlay. Collect compact logs only if support data is needed. Set CONTROLFG_VERBOSE_LOG=1 before launch only for deep diagnostic logging.'
} catch { Write-Error $_; exit 1 }
