#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Version = '2.3.0'
$SourceCommit = '60f4ea81909200d8542eca14dccb2628b763a9a3'
$HeaderManifest = @'
1b711807c597630f4a8d2dfa2572a845c069829a|api/include/dx12/ffx_api_dx12.h
5b518dc532a1cf7a89ee6cbac6ed10723ccb22c7|api/include/dx12/ffx_api_dx12.hpp
484ec23fa38a34c9061451cffd9f420ec2f7c1e8|api/include/ffx_api.h
bda66b2cee5eae0f0e24eec3d9fd80b6d3caa307|api/include/ffx_api.hpp
ac05ec4bbd5d338643825d81c0ffccd5fbbd1c3e|api/include/ffx_api_loader.h
07c0da20c8ae370caaa8bc674e7f22df83cbf163|api/include/ffx_api_types.h
8bc71ca2628552aa544abc8b205f267713916d1c|framegeneration/include/dx12/ffx_api_framegeneration_dx12.h
0fe46f0b08efbbc9481459c22370bb494bf56031|framegeneration/include/dx12/ffx_api_framegeneration_dx12.hpp
5ca5486460744e4357e30d3d273d7f12019fb5b1|framegeneration/include/ffx_framegeneration.h
baac01b2b5ec4ef220e62eb3fae26ef7784f25c1|framegeneration/include/ffx_framegeneration.hpp
e7e514ddb6666753de3b14f720d8e6774339b0b9|framegeneration/include/ffx_framegeneration_api_types.h
'@
$RuntimeManifest = @'
144916ba922c2a66149ad5cb1037a4f86d2b6491|amd_fidelityfx_loader_dx12.dll
1a06b72761086cb5561f8984ce0dc6a24a370e9e|amd_fidelityfx_framegeneration_dx12.dll
'@
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $thirdParty = Join-Path $PSScriptRoot 'third_party\fidelityfx'
    $infoPath = Join-Path $thirdParty 'sdk-info.json'
    if (Test-Path -LiteralPath $infoPath) {
        $existing = Get-Content -LiteralPath $infoPath -Raw | ConvertFrom-Json
        $complete = ($existing.Version -eq $Version) -and ($existing.SourceCommit -eq $SourceCommit)
        foreach ($entry in $existing.Files) {
            $path = Join-Path $thirdParty $entry.Path
            $complete = $complete -and (Test-Path -LiteralPath $path) -and ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -eq $entry.SHA256)
        }
        if ($complete) {
            Write-Host 'PASS: existing FidelityFX 2.3.0 staging matches its pinned manifest.'
            exit 0
        }
    }
    if (Test-Path -LiteralPath $thirdParty) { Remove-Item -LiteralPath $thirdParty -Recurse -Force }
    New-Item -ItemType Directory -Path $thirdParty -Force | Out-Null
    $records = @()
    Write-Host ('Staging FidelityFX headers from pinned commit ' + $SourceCommit + '...')
    foreach ($line in ($HeaderManifest -split "\r?\n")) {
        if (-not $line.Trim()) { continue }
        $parts = $line.Split('|', 2)
        $expectedBlob = $parts[0]
        $relative = $parts[1]
        $dest = Join-Path $thirdParty $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        $uri = 'https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/' + $SourceCommit + '/Kits/FidelityFX/' + $relative
        Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $dest
        $actualBlob = (& git hash-object --no-filters -- $dest).Trim()
        if ($LASTEXITCODE -ne 0 -or $actualBlob -ne $expectedBlob) { throw ('FidelityFX header blob mismatch: ' + $relative) }
        $records += [ordered]@{ Path=$relative; GitBlob=$actualBlob; SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size=(Get-Item -LiteralPath $dest).Length }
    }
    New-Item -ItemType Directory -Path (Join-Path $thirdParty 'bin') -Force | Out-Null
    Write-Host 'Staging signed FidelityFX DX12 runtimes from the same pinned commit...'
    foreach ($line in ($RuntimeManifest -split "\r?\n")) {
        if (-not $line.Trim()) { continue }
        $parts = $line.Split('|', 2)
        $expectedBlob = $parts[0]
        $name = $parts[1]
        $dest = Join-Path $thirdParty ('bin\' + $name)
        $uri = 'https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/' + $SourceCommit + '/Kits/FidelityFX/signedbin/' + $name
        Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $dest
        $actualBlob = (& git hash-object --no-filters -- $dest).Trim()
        if ($LASTEXITCODE -ne 0 -or $actualBlob -ne $expectedBlob) { throw ('FidelityFX runtime blob mismatch: ' + $name) }
        $signature = Get-AuthenticodeSignature -LiteralPath $dest
        if ($signature.Status -ne 'Valid') { throw ('FidelityFX runtime signature is not valid: ' + $name + ' (' + $signature.Status + ')') }
        $records += [ordered]@{ Path=('bin/' + $name); GitBlob=$actualBlob; SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size=(Get-Item -LiteralPath $dest).Length; Signer=$signature.SignerCertificate.Subject }
    }
    [ordered]@{
        Version=$Version
        ReleaseTag='v2.3.0'
        Source='GPUOpen-LibrariesAndSDKs/FidelityFX-SDK immutable GitHub commit'
        SourceCommit=$SourceCommit
        FrameGenerationProvider='FSR 3.1.6 selected at runtime'
        Swapchain='3.1.7'
        Files=$records
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8
    Write-Host ('PASS: staged ' + $records.Count + ' Git-blob-verified FidelityFX headers and signed runtimes.')
} catch { Write-Error $_; exit 1 }
