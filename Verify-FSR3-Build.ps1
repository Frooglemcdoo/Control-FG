#requires -Version 5.1
$ErrorActionPreference='Stop'
try {
    $expected='?doAntiAliasing@DLSS@d3d@@SA_NPEAVNativeTexture@2@00000000_NNNMMM@Z'
    $symbols=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\abi-symbols.txt') -Raw
    if (-not $symbols.Contains($expected)) { throw 'MSVC ABI mismatch.' }
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-FidelityFX.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'FidelityFX staging validation failed.' }
    $exports=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\exports.txt') -Raw
    foreach($name in @('CreateDXGIFactory','CreateDXGIFactory1','CreateDXGIFactory2','DXGIGetDebugInterface1','DXGIDeclareAdapterRemovalSupport')) { if ($exports -notmatch ('(?m)\b'+[regex]::Escape($name)+'\b')) { throw ('Missing proxy export: '+$name) } }
    $imports=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\imports.txt') -Raw
    if ($imports -match '(?im)^\s+dxgi\.dll\s*$') { throw 'Proxy imports itself.' }
    if ($imports -match '(?im)^\s+amd_fidelityfx_.*\.dll\s*$') { throw 'FidelityFX must be loaded by absolute path.' }
    $dll=Join-Path $PSScriptRoot 'build\dxgi.dll'
    $bytes=[IO.File]::ReadAllBytes($dll); $text=[Text.Encoding]::ASCII.GetString($bytes)
    foreach($marker in @('FSR3_CORE_READY','FSR3_SWAPCHAIN_CREATE','FSR3_PROVIDER_SELECTED','FSR3_CONTEXT_CREATE','FSR3_PREPARE','FSR3_CONFIGURE','FSR3_GENERATION_CALLBACK','fixed_2x_sdr_bringup')) { if (-not $text.Contains($marker)) { throw ('FSR3 build marker missing: '+$marker) } }
    [ordered]@{ Version='1.1.0-alpha1'; Backend='AMD FSR 3 Frame Generation'; FidelityFXSDK='2.3.0'; FrameGenerationProvider='FSR 3.1.6 runtime-selected; FSR 4 rejected'; Swapchain='3.1.7 DX12'; Mode='Fixed 2x, synchronous, SDR-only bring-up'; SHA256=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'build\fsr3-build-validation.json') -Encoding UTF8
    Write-Host 'PASS: FSR3 alpha1 proxy compiled and static validation passed.'
} catch { Write-Error $_; exit 1 }
