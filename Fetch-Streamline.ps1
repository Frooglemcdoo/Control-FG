#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Version = '2.14.1'
$Url = 'https://github.com/NVIDIA-RTX/Streamline/releases/download/v2.14.1/streamline-sdk-v2.14.1.zip'
$ExpectedArchiveSHA256 = '92C4D954631A1710DA86CA3FA8D5034F2B9503838C95FC4AE977AE149319781B'
$requiredBinaries = @('sl.interposer.dll','sl.common.dll','sl.pcl.dll','sl.reflex.dll','sl.dlss_g.dll','nvngx_dlssg.dll')
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $thirdParty = Join-Path $PSScriptRoot 'third_party\streamline'
    $includeStage = Join-Path $thirdParty 'include'
    $binStage = Join-Path $thirdParty 'bin'
    $infoPath = Join-Path $thirdParty 'sdk-info.json'
    if (Test-Path -LiteralPath $infoPath) {
        try {
            $existing = Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json
            $complete = ($existing.Version -eq $Version) -and (Test-Path -LiteralPath (Join-Path $includeStage 'sl.h'))
            foreach ($name in $requiredBinaries) { $complete = $complete -and (Test-Path -LiteralPath (Join-Path $binStage $name)) }
            if ($complete) {
                Write-Host 'Streamline SDK 2.14.1 is already staged; hashes will be rechecked.'
                foreach ($entry in $existing.Files) {
                    $p = Join-Path $thirdParty $entry.Path
                    if (-not (Test-Path -LiteralPath $p) -or (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne $entry.SHA256) { throw ('Staged Streamline file changed: ' + $entry.Path) }
                }
                Write-Host 'PASS: existing staged SDK files match sdk-info.json.'
                exit 0
            }
        } catch { Write-Host ('Existing staging is incomplete and will be replaced: ' + $_.Exception.Message) }
    }

    $work = Join-Path $PSScriptRoot 'third_party\_streamline_fetch'
    $archive = Join-Path $work 'streamline-sdk-v2.14.1.zip'
    $expanded = Join-Path $work 'expanded'
    if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force }
    New-Item -ItemType Directory -Path $expanded -Force | Out-Null
    Write-Host ('Downloading official NVIDIA Streamline SDK ' + $Version + ' (~276 MB)...')
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $archive
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    if ($archiveHash -ne $ExpectedArchiveSHA256) { throw ('Streamline release ZIP hash mismatch. Expected ' + $ExpectedArchiveSHA256 + ' got ' + $archiveHash) }
    Write-Host 'PASS: official release ZIP SHA-256 matches NVIDIA GitHub release metadata.'
    Expand-Archive -LiteralPath $archive -DestinationPath $expanded -Force

    $slHeader = Get-ChildItem -LiteralPath $expanded -Filter 'sl.h' -File -Recurse |
        Where-Object { $_.Directory.Name -eq 'include' } | Select-Object -First 1
    if (-not $slHeader) { throw 'Could not locate include\sl.h in the official SDK archive.' }
    $sdkRoot = $slHeader.Directory.Parent.FullName
    $sdkBin = Join-Path $sdkRoot 'bin\x64'
    if (-not (Test-Path -LiteralPath $sdkBin)) { throw ('Could not locate official x64 production bin folder: ' + $sdkBin) }

    if (Test-Path -LiteralPath $thirdParty) { Remove-Item -LiteralPath $thirdParty -Recurse -Force }
    New-Item -ItemType Directory -Path $includeStage -Force | Out-Null
    New-Item -ItemType Directory -Path $binStage -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $slHeader.Directory.FullName '*') -Destination $includeStage -Force -ErrorAction SilentlyContinue
    # LiteralPath does not expand '*'; copy headers explicitly for PowerShell 5.1.
    Get-ChildItem -LiteralPath $slHeader.Directory.FullName -File | Copy-Item -Destination $includeStage -Force

    $fileRecords = @()
    foreach ($name in $requiredBinaries) {
        $source = Join-Path $sdkBin $name
        if (-not (Test-Path -LiteralPath $source)) { throw ('Required production Streamline binary is missing from official SDK: ' + $name) }
        $dest = Join-Path $binStage $name
        Copy-Item -LiteralPath $source -Destination $dest -Force
        $fileRecords += [ordered]@{ Path = ('bin/' + $name); SHA256 = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size = (Get-Item -LiteralPath $dest).Length }
    }
    foreach ($header in Get-ChildItem -LiteralPath $includeStage -File) {
        $fileRecords += [ordered]@{ Path = ('include/' + $header.Name); SHA256 = (Get-FileHash -LiteralPath $header.FullName -Algorithm SHA256).Hash; Size = $header.Length }
    }
    [ordered]@{
        Version = $Version
        Source = 'NVIDIA-RTX/Streamline official GitHub release'
        SourceURL = $Url
        SourceArchiveSHA256 = $ExpectedArchiveSHA256
        Architecture = 'x64'
        Configuration = 'Production'
        StagedUtc = [DateTime]::UtcNow.ToString('o')
        Files = $fileRecords
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8
    Remove-Item -LiteralPath $work -Recurse -Force
    Write-Host ('PASS: staged ' + $requiredBinaries.Count + ' production runtime DLLs and official SDK headers under third_party\streamline.')
} catch {
    Write-Error $_
    exit 1
}
