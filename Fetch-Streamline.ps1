#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'

$Version = '2.14.1'
$Url = 'https://github.com/NVIDIA-RTX/Streamline/releases/download/v2.14.1/streamline-sdk-v2.14.1.zip'
$ExpectedArchiveSHA256 = '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B'
$requiredBinaries = @(
    'sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll',
    'sl.dlss_g.dll','nvngx_dlssg.dll','sl.dlss_d.dll','nvngx_dlssd.dll'
)

function Test-StagedSdk([string]$Root) {
    try {
        $infoPath = Join-Path $Root 'sdk-info.json'
        $includeStage = Join-Path $Root 'include'
        $binStage = Join-Path $Root 'bin'
        if (-not (Test-Path -LiteralPath $infoPath)) { return $false }
        $info = Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json
        if ($info.Version -ne $Version) { return $false }
        if ($info.SourceArchiveSHA256 -ne $ExpectedArchiveSHA256) { return $false }
        if (-not (Test-Path -LiteralPath (Join-Path $includeStage 'sl.h'))) { return $false }
        if (-not (Test-Path -LiteralPath (Join-Path $includeStage 'sl_dlss_d.h'))) { return $false }
        foreach ($name in $requiredBinaries) {
            if (-not (Test-Path -LiteralPath (Join-Path $binStage $name))) { return $false }
        }
        foreach ($entry in $info.Files) {
            $p = Join-Path $Root ($entry.Path -replace '/', '\')
            if (-not (Test-Path -LiteralPath $p)) { return $false }
            if ((Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne $entry.SHA256) { return $false }
        }
        return $true
    } catch {
        return $false
    }
}

function Copy-StagedSdk([string]$Source, [string]$Destination) {
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | Copy-Item -Destination $Destination -Recurse -Force
}

function Download-Archive([string]$Destination) {
    $tmp = $Destination + '.download'
    if (Test-Path -LiteralPath $tmp) { Remove-Item -LiteralPath $tmp -Force }

    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        Write-Host 'Downloading pinned NVIDIA Streamline SDK with curl (first build only)...'
        & $curl.Source --location --fail --retry 3 --retry-delay 1 --output $tmp $Url
        if ($LASTEXITCODE -ne 0) { throw ('curl.exe failed with exit code ' + $LASTEXITCODE) }
    } else {
        Write-Host 'Downloading pinned NVIDIA Streamline SDK (first build only)...'
        Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $tmp
    }

    $hash = (Get-FileHash -LiteralPath $tmp -Algorithm SHA256).Hash
    if ($hash -ne $ExpectedArchiveSHA256) {
        Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
        throw ('Streamline release ZIP hash mismatch. Expected ' + $ExpectedArchiveSHA256 + ' got ' + $hash)
    }
    Move-Item -LiteralPath $tmp -Destination $Destination -Force
}

try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

    $projectStage = Join-Path $PSScriptRoot 'third_party\streamline'

    # Persistent cache survives extraction/deletion of individual test-build folders.
    $cacheBase = $env:CONTROL_FG_BUILD_CACHE
    if ([string]::IsNullOrWhiteSpace($cacheBase)) {
        if (-not [string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) {
            $cacheBase = Join-Path $env:LOCALAPPDATA 'ControlFG\BuildCache'
        } else {
            $cacheBase = Join-Path $env:TEMP 'ControlFG\BuildCache'
        }
    }
    $versionCache = Join-Path $cacheBase ('Streamline\' + $Version)
    $cacheStage = Join-Path $versionCache 'staged'
    $cacheArchive = Join-Path $versionCache ('streamline-sdk-v' + $Version + '.zip')
    New-Item -ItemType Directory -Path $versionCache -Force | Out-Null

    if (Test-StagedSdk $projectStage) {
        Write-Host 'CACHE HIT: project Streamline SDK 2.14.1 staging is valid.'
        exit 0
    }

    if (Test-StagedSdk $cacheStage) {
        Write-Host ('CACHE HIT: restoring Streamline SDK 2.14.1 from ' + $cacheStage)
        Copy-StagedSdk $cacheStage $projectStage
        if (-not (Test-StagedSdk $projectStage)) { throw 'Persistent Streamline cache restore failed validation.' }
        Write-Host 'PASS: restored validated Streamline SDK from persistent cache; no network download required.'
        exit 0
    }

    # Migration path for users who already built an older test ZIP before the persistent
    # cache existed. Look only at sibling Control-FG folders, validate their staged SDK,
    # then import it into the permanent cache. This avoids one last redundant download.
    $parentDir = Split-Path -Parent $PSScriptRoot
    try {
        foreach ($dir in Get-ChildItem -LiteralPath $parentDir -Directory -ErrorAction SilentlyContinue) {
            if ($dir.FullName -eq $PSScriptRoot) { continue }
            if ($dir.Name -notlike 'Control-FG*') { continue }
            $candidate = Join-Path $dir.FullName 'third_party\streamline'
            if (Test-StagedSdk $candidate) {
                Write-Host ('CACHE IMPORT: reusing validated Streamline SDK from prior build ' + $dir.FullName)
                Copy-StagedSdk $candidate $projectStage
                if (-not (Test-StagedSdk $projectStage)) { throw 'Prior-build Streamline import failed validation.' }
                Copy-StagedSdk $projectStage $cacheStage
                if (-not (Test-StagedSdk $cacheStage)) { throw 'Persistent cache seed failed validation.' }
                Write-Host 'PASS: imported prior-build SDK into persistent cache; no network download required.'
                exit 0
            }
        }
    } catch {
        Write-Host ('Prior-build cache import skipped: ' + $_.Exception.Message)
    }

    # Keep a verified copy of the official archive too, so a damaged staged cache can
    # be rebuilt without another network transfer.
    $archiveValid = $false
    if (Test-Path -LiteralPath $cacheArchive) {
        try {
            $archiveValid = ((Get-FileHash -LiteralPath $cacheArchive -Algorithm SHA256).Hash -eq $ExpectedArchiveSHA256)
        } catch { $archiveValid = $false }
        if (-not $archiveValid) {
            Write-Host 'Cached Streamline archive failed validation and will be replaced.'
            Remove-Item -LiteralPath $cacheArchive -Force
        }
    }

    if (-not $archiveValid) {
        Download-Archive $cacheArchive
        Write-Host 'PASS: official release ZIP SHA-256 matches the pinned NVIDIA release.'
    } else {
        Write-Host 'CACHE HIT: verified Streamline release archive is already present.'
    }

    $work = Join-Path $versionCache ('expand-' + $PID)
    $expanded = Join-Path $work 'expanded'
    if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force }
    New-Item -ItemType Directory -Path $expanded -Force | Out-Null

    Expand-Archive -LiteralPath $cacheArchive -DestinationPath $expanded -Force

    $slHeader = Get-ChildItem -LiteralPath $expanded -Filter 'sl.h' -File -Recurse |
        Where-Object { $_.Directory.Name -eq 'include' } | Select-Object -First 1
    if (-not $slHeader) { throw 'Could not locate include\sl.h in the official SDK archive.' }

    $sdkRoot = $slHeader.Directory.Parent.FullName
    $sdkBin = Join-Path $sdkRoot 'bin\x64'
    if (-not (Test-Path -LiteralPath $sdkBin)) { throw ('Could not locate official x64 production bin folder: ' + $sdkBin) }

    if (Test-Path -LiteralPath $projectStage) { Remove-Item -LiteralPath $projectStage -Recurse -Force }
    $includeStage = Join-Path $projectStage 'include'
    $binStage = Join-Path $projectStage 'bin'
    $infoPath = Join-Path $projectStage 'sdk-info.json'
    New-Item -ItemType Directory -Path $includeStage -Force | Out-Null
    New-Item -ItemType Directory -Path $binStage -Force | Out-Null

    Get-ChildItem -LiteralPath $slHeader.Directory.FullName -File | Copy-Item -Destination $includeStage -Force

    $fileRecords = @()
    foreach ($name in $requiredBinaries) {
        $source = Join-Path $sdkBin $name
        if (-not (Test-Path -LiteralPath $source)) { throw ('Required production Streamline binary is missing from official SDK: ' + $name) }
        $dest = Join-Path $binStage $name
        Copy-Item -LiteralPath $source -Destination $dest -Force
        $fileRecords += [ordered]@{
            Path = ('bin/' + $name)
            SHA256 = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
            Size = (Get-Item -LiteralPath $dest).Length
        }
    }

    foreach ($header in Get-ChildItem -LiteralPath $includeStage -File) {
        $fileRecords += [ordered]@{
            Path = ('include/' + $header.Name)
            SHA256 = (Get-FileHash -LiteralPath $header.FullName -Algorithm SHA256).Hash
            Size = $header.Length
        }
    }

    [ordered]@{
        Version = $Version
        Source = 'NVIDIA-RTX/Streamline official GitHub release'
        SourceURL = $Url
        SourceArchiveSHA256 = $ExpectedArchiveSHA256
        Architecture = 'x64'
        Configuration = 'Production'
        StagedUtc = [DateTime]::UtcNow.ToString('o')
        PersistentCacheRoot = $versionCache
        Files = $fileRecords
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8

    if (-not (Test-StagedSdk $projectStage)) { throw 'Fresh Streamline staging failed validation.' }

    # Refresh the persistent validated staged subset for all future test builds.
    Copy-StagedSdk $projectStage $cacheStage
    if (-not (Test-StagedSdk $cacheStage)) { throw 'Persistent Streamline staged cache failed validation.' }

    Remove-Item -LiteralPath $work -Recurse -Force
    Write-Host ('PASS: Streamline SDK 2.14.1 staged and cached persistently at ' + $versionCache)
    Write-Host 'Future Control FG test builds will reuse this cache and skip the network download.'
} catch {
    Write-Error $_
    exit 1
}
