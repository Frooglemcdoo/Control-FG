#requires -Version 5.1
$ErrorActionPreference = 'Stop'

$Version = '2.14.1'
$ArchiveSha256 = '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B'
$ReleaseUrl = 'https://github.com/NVIDIA-RTX/Streamline/releases/download/v2.14.1/streamline-sdk-v2.14.1.zip'
$SdkRoot = Join-Path $PSScriptRoot 'third_party\streamline'
$CacheRoot = Join-Path $env:LOCALAPPDATA 'ControlFG\Cache'
$Archive = Join-Path $CacheRoot 'streamline-sdk-v2.14.1.zip'
$ExtractRoot = Join-Path $CacheRoot 'streamline-sdk-v2.14.1'

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

try {
    New-Item -ItemType Directory -Path $CacheRoot -Force | Out-Null

    $needDownload = $true
    if (Test-Path -LiteralPath $Archive) {
        if ((Get-Sha256 $Archive) -eq $ArchiveSha256) { $needDownload = $false }
        else { Remove-Item -LiteralPath $Archive -Force }
    }

    if ($needDownload) {
        Write-Host ('Downloading NVIDIA Streamline SDK ' + $Version + ' ...')
        Invoke-WebRequest -Uri $ReleaseUrl -OutFile $Archive -UseBasicParsing
    }

    $actualArchiveHash = Get-Sha256 $Archive
    if ($actualArchiveHash -ne $ArchiveSha256) {
        throw ('Streamline archive SHA-256 mismatch. Expected ' + $ArchiveSha256 + ', got ' + $actualArchiveHash)
    }

    if (Test-Path -LiteralPath $ExtractRoot) { Remove-Item -LiteralPath $ExtractRoot -Recurse -Force }
    New-Item -ItemType Directory -Path $ExtractRoot -Force | Out-Null
    Expand-Archive -LiteralPath $Archive -DestinationPath $ExtractRoot -Force

    $sourceRoot = Get-ChildItem -LiteralPath $ExtractRoot -Directory | Select-Object -First 1
    if (-not $sourceRoot) { throw 'The Streamline archive did not contain an SDK directory.' }

    if (Test-Path -LiteralPath $SdkRoot) { Remove-Item -LiteralPath $SdkRoot -Recurse -Force }
    New-Item -ItemType Directory -Path (Join-Path $SdkRoot 'include'), (Join-Path $SdkRoot 'bin') -Force | Out-Null

    $includeNames = @('sl.h','sl_consts.h','sl_core_api.h','sl_core_types.h','sl_dlss_g.h','sl_helpers.h','sl_hooks.h','sl_pcl.h','sl_reflex.h','sl_result.h','sl_struct.h','sl_version.h')
    foreach ($name in $includeNames) {
        $source = Join-Path $sourceRoot.FullName ('include\' + $name)
        if (-not (Test-Path -LiteralPath $source)) { throw ('Missing Streamline include: ' + $name) }
        Copy-Item -LiteralPath $source -Destination (Join-Path $SdkRoot ('include\' + $name)) -Force
    }

    $runtimeNames = @('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll')
    $binCandidates = @(
        (Join-Path $sourceRoot.FullName 'bin\x64'),
        (Join-Path $sourceRoot.FullName 'bin\x64\Release'),
        (Join-Path $sourceRoot.FullName 'bin')
    )
    foreach ($name in $runtimeNames) {
        $found = $null
        foreach ($dir in $binCandidates) {
            $candidate = Join-Path $dir $name
            if (Test-Path -LiteralPath $candidate) { $found = $candidate; break }
        }
        if (-not $found) {
            $match = Get-ChildItem -LiteralPath $sourceRoot.FullName -Recurse -File -Filter $name -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch '\\debug\\' } | Select-Object -First 1
            if ($match) { $found = $match.FullName }
        }
        if (-not $found) { throw ('Missing Streamline runtime: ' + $name) }
        Copy-Item -LiteralPath $found -Destination (Join-Path $SdkRoot ('bin\' + $name)) -Force
    }

    $records = @()
    foreach ($name in $includeNames) {
        $p = Join-Path $SdkRoot ('include\' + $name)
        $records += [ordered]@{ Path=('include/' + $name); SHA256=(Get-Sha256 $p) }
    }
    foreach ($name in $runtimeNames) {
        $p = Join-Path $SdkRoot ('bin\' + $name)
        $records += [ordered]@{ Path=('bin/' + $name); SHA256=(Get-Sha256 $p) }
    }
    [ordered]@{
        Version=$Version
        SourceUrl=$ReleaseUrl
        SourceArchiveSHA256=$ArchiveSha256
        Files=$records
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $SdkRoot 'sdk-info.json') -Encoding UTF8

    Write-Host ('PASS: staged and hashed NVIDIA Streamline SDK ' + $Version + ' in ' + $SdkRoot)
} catch {
    Write-Error $_
    exit 1
}
