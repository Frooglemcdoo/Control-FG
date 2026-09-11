#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Version = '2.3.0'
$Url = 'https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases/download/v2.3.0/FidelityFX-Samples-v2.3.0-prebuilt.zip'
$ExpectedArchiveSHA256 = 'F90890B9323BB2F4F2404AC4CDC9395E8495ECDAC6F7AA0BCDF1AD1848422273'
$SourceCommit = '60f4ea81909200d8542eca14dccb2628b763a9a3'
$SourceUrl = 'https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/archive/' + $SourceCommit + '.zip'
$RuntimeNames = @('amd_fidelityfx_loader_dx12.dll','amd_fidelityfx_framegeneration_dx12.dll')
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $thirdParty = Join-Path $PSScriptRoot 'third_party\fidelityfx'
    $infoPath = Join-Path $thirdParty 'sdk-info.json'
    if (Test-Path -LiteralPath $infoPath) {
        $existing = Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json
        $complete = $existing.Version -eq $Version
        foreach ($name in $RuntimeNames) { $complete = $complete -and (Test-Path -LiteralPath (Join-Path $thirdParty ('bin\' + $name))) }
        $complete = $complete -and (Test-Path -LiteralPath (Join-Path $thirdParty 'api\include\ffx_api.h'))
        if ($complete) {
            foreach ($entry in $existing.Files) {
                $path = Join-Path $thirdParty $entry.Path
                if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.SHA256) { throw ('Staged FidelityFX file changed: ' + $entry.Path) }
            }
            Write-Host 'PASS: existing FidelityFX SDK 2.3.0 staging matches sdk-info.json.'
            exit 0
        }
    }
    $work = Join-Path $PSScriptRoot 'third_party\_fidelityfx_fetch'
    $archive = Join-Path $work 'FidelityFX-Samples-v2.3.0-prebuilt.zip'
    $expanded = Join-Path $work 'expanded'
    if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force }
    New-Item -ItemType Directory -Path $expanded -Force | Out-Null
    Write-Host 'Downloading official AMD FSR SDK 2.3.0 prebuilt package...'
    Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $archive
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    if ($archiveHash -ne $ExpectedArchiveSHA256) { throw ('FidelityFX archive hash mismatch. Got ' + $archiveHash) }
    Expand-Archive -LiteralPath $archive -DestinationPath $expanded -Force
    # The minimal prebuilt package carries the signed runtimes and samples, but its
    # 2.3.0 layout does not expose the public headers at the former SDK path.
    # Fetch the immutable release commit for headers and keep the release ZIP for DLLs.
    $sourceArchive = Join-Path $work ('FidelityFX-SDK-' + $SourceCommit + '.zip')
    $sourceExpanded = Join-Path $work 'source'
    Write-Host ('Downloading FidelityFX SDK headers from pinned commit ' + $SourceCommit + '...')
    Invoke-WebRequest -UseBasicParsing -Uri $SourceUrl -OutFile $sourceArchive
    New-Item -ItemType Directory -Path $sourceExpanded -Force | Out-Null
    Expand-Archive -LiteralPath $sourceArchive -DestinationPath $sourceExpanded -Force
    $apiHeader = Get-ChildItem -LiteralPath $sourceExpanded -Filter 'ffx_api.h' -File -Recurse | Where-Object { $_.FullName -match '[\\/]Kits[\\/]FidelityFX[\\/]api[\\/]include[\\/]ffx_api\.h$' } | Select-Object -First 1
    if (-not $apiHeader) { throw 'Could not locate ffx_api.h in pinned FidelityFX source commit.' }
    $fidelityRoot = $apiHeader.Directory.Parent.Parent.FullName
    if (Test-Path -LiteralPath $thirdParty) { Remove-Item -LiteralPath $thirdParty -Recurse -Force }
    New-Item -ItemType Directory -Path $thirdParty -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $fidelityRoot 'api') -Destination $thirdParty -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $fidelityRoot 'framegeneration') -Destination $thirdParty -Recurse -Force
    New-Item -ItemType Directory -Path (Join-Path $thirdParty 'bin') -Force | Out-Null
    $records = @()
    foreach ($name in $RuntimeNames) {
        $source = Get-ChildItem -LiteralPath $expanded -Filter $name -File -Recurse | Where-Object { $_.FullName -match '[\\/]signedbin[\\/]' } | Select-Object -First 1
        if (-not $source) { throw ('Missing signed FidelityFX runtime in prebuilt release: ' + $name) }
        $dest = Join-Path $thirdParty ('bin\' + $name)
        Copy-Item -LiteralPath $source.FullName -Destination $dest -Force
        $records += [ordered]@{ Path=('bin/' + $name); SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size=(Get-Item $dest).Length }
    }
    foreach ($path in @('api\include\ffx_api.h','api\include\ffx_api_types.h','api\include\dx12\ffx_api_dx12.h','framegeneration\include\ffx_framegeneration.h','framegeneration\include\ffx_framegeneration_api_types.h','framegeneration\include\dx12\ffx_api_framegeneration_dx12.h')) {
        $full = Join-Path $thirdParty $path
        if (-not (Test-Path -LiteralPath $full)) { throw ('Missing FidelityFX header: ' + $path) }
        $records += [ordered]@{ Path=$path.Replace('\','/'); SHA256=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash; Size=(Get-Item $full).Length }
    }
    [ordered]@{ Version=$Version; Source='GPUOpen-LibrariesAndSDKs/FidelityFX-SDK official GitHub release'; SourceURL=$Url; SourceArchiveSHA256=$ExpectedArchiveSHA256; SourceCommit=$SourceCommit; FrameGenerationProvider='FSR 3.1.6 selected at runtime'; Swapchain='3.1.7'; Files=$records } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8
    Remove-Item -LiteralPath $work -Recurse -Force
    Write-Host 'PASS: staged AMD FidelityFX SDK 2.3.0 headers and signed FSR frame-generation runtimes.'
} catch { Write-Error $_; exit 1 }
 } | Select-Object -First 1
    if (-not $apiHeader) { throw 'Could not locate ffx_api.h in pinned FidelityFX source commit.' }
    $fidelityRoot = $apiHeader.Directory.Parent.Parent.FullName
    if (Test-Path -LiteralPath $thirdParty) { Remove-Item -LiteralPath $thirdParty -Recurse -Force }
    New-Item -ItemType Directory -Path $thirdParty -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $fidelityRoot 'api') -Destination $thirdParty -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $fidelityRoot 'framegeneration') -Destination $thirdParty -Recurse -Force
    New-Item -ItemType Directory -Path (Join-Path $thirdParty 'bin') -Force | Out-Null
    $records = @()
    foreach ($name in $RuntimeNames) {
        $source = Join-Path $signedBin $name
        if (-not (Test-Path -LiteralPath $source)) { throw ('Missing signed FidelityFX runtime: ' + $name) }
        $dest = Join-Path $thirdParty ('bin\' + $name)
        Copy-Item -LiteralPath $source -Destination $dest -Force
        $records += [ordered]@{ Path=('bin/' + $name); SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size=(Get-Item $dest).Length }
    }
    foreach ($path in @('api\include\ffx_api.h','api\include\ffx_api_types.h','api\include\dx12\ffx_api_dx12.h','framegeneration\include\ffx_framegeneration.h','framegeneration\include\ffx_framegeneration_api_types.h','framegeneration\include\dx12\ffx_api_framegeneration_dx12.h')) {
        $full = Join-Path $thirdParty $path
        if (-not (Test-Path -LiteralPath $full)) { throw ('Missing FidelityFX header: ' + $path) }
        $records += [ordered]@{ Path=$path.Replace('\','/'); SHA256=(Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash; Size=(Get-Item $full).Length }
    }
    [ordered]@{ Version=$Version; Source='GPUOpen-LibrariesAndSDKs/FidelityFX-SDK official GitHub release'; SourceURL=$Url; SourceArchiveSHA256=$ExpectedArchiveSHA256; FrameGenerationProvider='FSR 3.1.6 selected at runtime'; Swapchain='3.1.7'; Files=$records } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8
    Remove-Item -LiteralPath $work -Recurse -Force
    Write-Host 'PASS: staged AMD FidelityFX SDK 2.3.0 headers and signed FSR frame-generation runtimes.'
} catch { Write-Error $_; exit 1 }
