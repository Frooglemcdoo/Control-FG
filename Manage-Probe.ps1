#requires -Version 5.1
[CmdletBinding()]
param([ValidateSet('Install', 'Uninstall', 'Collect')][string]$Action = 'Install', [string]$GamePath)
$ErrorActionPreference = 'Stop'
$RuntimeNames = @('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll')
try {
    if ($Action -eq 'Collect') {
        $logDir = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
        $logs = @(Get-ChildItem -LiteralPath $logDir -Filter 'probe-*.log' -File -ErrorAction SilentlyContinue |
            Where-Object { (Get-Content -LiteralPath $_.FullName -TotalCount 1) -match ' PROBE v1\.0\.0 ' } |
            Sort-Object LastWriteTime -Descending | Select-Object -First 3)
        if (-not $logs.Count) { throw 'No v1.0.0 log exists. Check Build/Install first. Logs from every other probe version are excluded.' }
        $name = 'Control-FG-v1.0.0-Logs-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,6) + '.zip'
        $zip = Join-Path $PSScriptRoot $name
        $stage = Join-Path $env:TEMP ('ControlFG-v1.0.0-Collect-' + [guid]::NewGuid().ToString('N'))
        try {
            New-Item -ItemType Directory -Path $stage -Force | Out-Null
            $probeStage = Join-Path $stage 'probe'
            $packageStage = Join-Path $stage 'package'
            $streamlineStage = Join-Path $stage 'streamline'
            New-Item -ItemType Directory -Path $probeStage,$packageStage,$streamlineStage -Force | Out-Null
            foreach ($log in $logs) { Copy-Item -LiteralPath $log.FullName -Destination (Join-Path $probeStage $log.Name) -Force }
            foreach ($relative in @('Build.log', 'build\build-validation.json', 'third_party\streamline\sdk-info.json', 'installation.json')) {
                $path = Join-Path $PSScriptRoot $relative
                if (Test-Path -LiteralPath $path) {
                    $safeName = ($relative -replace '[\\/]', '__')
                    Copy-Item -LiteralPath $path -Destination (Join-Path $packageStage $safeName) -Force
                }
            }
            $settingsPath = Join-Path (Join-Path $env:LOCALAPPDATA 'ControlFG') 'settings.ini'
            if (Test-Path -LiteralPath $settingsPath) {
                Copy-Item -LiteralPath $settingsPath -Destination (Join-Path $packageStage 'ControlFG-settings.ini') -Force
            }
            $slRunDirs = @(Get-ChildItem -LiteralPath $logDir -Directory -Filter 'Streamline-v1.0.0-*' -ErrorAction SilentlyContinue |
                Sort-Object LastWriteTime -Descending | Select-Object -First 3)
            foreach ($slDir in $slRunDirs) {
                $runStage = Join-Path $streamlineStage $slDir.Name
                New-Item -ItemType Directory -Path $runStage -Force | Out-Null
                $slFiles = @(Get-ChildItem -LiteralPath $slDir.FullName -File -Recurse -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTime -Descending | Select-Object -First 30)
                foreach ($slFile in $slFiles) {
                    $relative = $slFile.FullName.Substring($slDir.FullName.Length).TrimStart('\')
                    $destination = Join-Path $runStage $relative
                    $parent = Split-Path -Parent $destination
                    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
                    Copy-Item -LiteralPath $slFile.FullName -Destination $destination -Force
                }
            }
            if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
            Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
        } finally {
            if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue }
        }
        Write-Host ('Upload this ZIP: ' + $zip)
        exit 0
    }

    if (Get-Process -Name 'Control_DX12', 'Control_DX11', 'Control' -ErrorAction SilentlyContinue) { throw 'Close Control before installing or removing the v1.0.0 bridge.' }
    $receiptPath = Join-Path $PSScriptRoot 'installation.json'
    $receipt = $null
    if (Test-Path -LiteralPath $receiptPath) { $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json }
    if (-not $GamePath -and $receipt) { $GamePath = $receipt.GamePath }
    if (-not $GamePath) {
        Write-Host 'Steam > Control > Properties > Installed Files > Browse.'
        $GamePath = Read-Host 'Paste the folder containing Control_DX12.exe'
    }
    $GamePath = (Resolve-Path -LiteralPath $GamePath.Trim().Trim('"')).ProviderPath
    if (-not (Test-Path -LiteralPath (Join-Path $GamePath 'Control_DX12.exe'))) { throw 'This folder does not contain Control_DX12.exe.' }

    if ($Action -eq 'Uninstall') {
        if (-not $receipt -or $receipt.GamePath -ine $GamePath -or $receipt.Version -ne '1.0.0') { throw 'No matching v1.0.0 installation receipt. Do not delete unrelated files.' }
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
        Write-Host 'Control FG v1.0.0 Control FG build removed. Original game files were not replaced.'
        exit 0
    }

    if ($receipt) { throw 'This package already has an installation receipt. Run Uninstall.cmd before reinstalling.' }
    $proxyDestination = Join-Path $GamePath 'dxgi.dll'
    $runtimeDestination = Join-Path $GamePath 'ControlFGStreamline'
    if (Test-Path -LiteralPath $proxyDestination) { throw 'A dxgi.dll already exists. Uninstall the previously installed proxy first.' }
    if (Test-Path -LiteralPath $runtimeDestination) { throw 'A ControlFGStreamline folder already exists. It was not modified. Remove/rename it only if you know it belongs to an earlier Control FG test.' }

    $proxySource = Join-Path $PSScriptRoot 'build\dxgi.dll'
    $validationPath = Join-Path $PSScriptRoot 'build\build-validation.json'
    if (-not (Test-Path -LiteralPath $proxySource) -or -not (Test-Path -LiteralPath $validationPath)) { throw 'Run Build.cmd successfully first.' }
    $validation = Get-Content -LiteralPath $validationPath -Raw | ConvertFrom-Json
    if ($validation.Version -ne '1.0.0' -or $validation.StreamlineSDKVersion -ne '2.14.1' -or $validation.DLSSGGenerationEnabled -ne $true -or $validation.DLSSGGeneratedFramesRequested -ne 3 -or $validation.TargetMultiplier -ne '4x') { throw 'Wrong/incomplete v1.0.0 build validation. Extract this package into a new folder, run Fetch-Streamline.cmd, then Build.cmd.' }
    $proxyHash = (Get-FileHash -LiteralPath $proxySource -Algorithm SHA256).Hash
    if ($proxyHash -ne $validation.SHA256 -or $validation.AbiCheck -ne 'Passed') { throw 'The proxy DLL does not match its validated build. Run Build.cmd again.' }

    $manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'target-manifest.json') -Raw | ConvertFrom-Json
    foreach ($file in $manifest.RequiredFiles) {
        $path = Join-Path $GamePath $file.Name
        if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.SHA256) { throw ('Unsupported or changed game file: ' + $file.Name) }
    }

    $sdkRoot = Join-Path $PSScriptRoot 'third_party\streamline'
    $sdkInfoPath = Join-Path $sdkRoot 'sdk-info.json'
    if (-not (Test-Path -LiteralPath $sdkInfoPath)) { throw 'Streamline SDK is not staged. Run Fetch-Streamline.cmd, then Build.cmd.' }
    $sdkInfo = Get-Content -LiteralPath $sdkInfoPath -Raw | ConvertFrom-Json
    if ($sdkInfo.Version -ne '2.14.1' -or $sdkInfo.SourceArchiveSHA256 -ne '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B') { throw 'Staged Streamline SDK does not match the pinned official 2.14.1 release.' }

    $planned = @([ordered]@{ RelativePath = 'dxgi.dll'; Source = $proxySource; SHA256 = $proxyHash })
    foreach ($name in $RuntimeNames) {
        $source = Join-Path $sdkRoot ('bin\' + $name)
        if (-not (Test-Path -LiteralPath $source)) { throw ('Missing staged Streamline runtime: ' + $name) }
        $record = @($sdkInfo.Files | Where-Object { $_.Path -eq ('bin/' + $name) })
        if ($record.Count -ne 1) { throw ('sdk-info.json is missing a unique hash record for ' + $name) }
        $hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
        if ($hash -ne $record[0].SHA256) { throw ('Staged Streamline runtime hash changed: ' + $name) }
        $planned += [ordered]@{ RelativePath = ('ControlFGStreamline/' + $name); Source = $source; SHA256 = $hash }
    }

    # Receipt is written before copies so any partial copy remains safely recoverable.
    [ordered]@{
        Version = '1.0.0'
        GamePath = $GamePath
        StreamlineSDKVersion = '2.14.1'
        DLSSGGenerationEnabled = $true
        DLSSGGeneratedFramesRequested = 3
        TargetMultiplier = '4x'
        InstalledUtc = [DateTime]::UtcNow.ToString('o')
        Files = @($planned | ForEach-Object { [ordered]@{ RelativePath = $_.RelativePath; SHA256 = $_.SHA256 } })
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receiptPath -Encoding UTF8

    New-Item -ItemType Directory -Path $runtimeDestination | Out-Null
    foreach ($file in $planned) {
        $destination = Join-Path $GamePath ($file.RelativePath -replace '/', '\')
        [IO.File]::Copy($file.Source, $destination, $false)
    }
    Write-Host 'Installed Control FG v1.0.0 with runtime-proven fixed 2x-6x plus native Dynamic/HDR and the Control-native top-right persistent F10 overlay with live FPS and embedded CONTROL FG title; latency telemetry removed.'
    Write-Host 'IMPORTANT: the v0.8.26 generation core is frozen. Change mode and Dynamic target in the F10 overlay, restart Control, and confirm the saved values are restored.'
    Write-Host 'After installation, run a short fixed/Dynamic/HDR smoke test before distribution.' 
} catch { Write-Error $_; exit 1 }
