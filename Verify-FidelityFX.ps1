#requires -Version 5.1
$ErrorActionPreference='Stop'
try {
    $root=Join-Path $PSScriptRoot 'third_party\fidelityfx'
    $info=Get-Content -LiteralPath (Join-Path $root 'sdk-info.json') -Raw | ConvertFrom-Json
    if ($info.Version -ne '2.3.0' -or $info.SourceCommit -ne '60f4ea81909200d8542eca14dccb2628b763a9a3') { throw 'Unexpected FidelityFX SDK identity.' }
    foreach($entry in $info.Files) {
        $path=Join-Path $root $entry.Path
        if (-not (Test-Path -LiteralPath $path)) { throw ('Missing FidelityFX file: '+$entry.Path) }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.SHA256) { throw ('FidelityFX file hash mismatch: '+$entry.Path) }
        if ($entry.Path -like 'bin/*' -and (Get-AuthenticodeSignature -LiteralPath $path).Status -ne 'Valid') { throw ('Invalid FidelityFX runtime signature: '+$entry.Path) }
    }
    $fg=Get-Content -LiteralPath (Join-Path $root 'framegeneration\include\ffx_framegeneration.h') -Raw
    foreach($marker in @('FFX_FRAMEGENERATION_VERSION_MAJOR 4','ffxDispatchDescFrameGenerationPrepareV2','ffxCreateContextDescFrameGenerationVersion')) { if (-not $fg.Contains($marker)) { throw ('Frame-generation header missing marker: '+$marker) } }
    $swap=Get-Content -LiteralPath (Join-Path $root 'framegeneration\include\dx12\ffx_api_framegeneration_dx12.h') -Raw
    foreach($marker in @('FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MAJOR 3','FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_MINOR 1','FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION_PATCH 7')) { if (-not $swap.Contains($marker)) { throw ('Swapchain header missing marker: '+$marker) } }
    Write-Host 'PASS: FidelityFX SDK 2.3.0 commit, signed runtimes, and FSR 3.1.7 swapchain headers verified.'
} catch { Write-Error $_; exit 1 }
