#requires -Version 5.1
$ErrorActionPreference='Stop'
try {
    $version='1.1.0-alpha1'; $proxy=Join-Path $PSScriptRoot 'build\dxgi.dll'
    if (-not (Test-Path $proxy)) { throw 'Run Build-FSR3.cmd first.' }
    $out=Join-Path $PSScriptRoot ('release\Control-FG-FSR3-v'+$version); $zip=Join-Path $PSScriptRoot ('release\Control-FG-FSR3-v'+$version+'.zip')
    if (Test-Path $out) { Remove-Item $out -Recurse -Force }; if (Test-Path $zip) { Remove-Item $zip -Force }
    New-Item -ItemType Directory -Path (Join-Path $out 'ControlFGFidelityFX') -Force | Out-Null
    Copy-Item $proxy (Join-Path $out 'dxgi.dll')
    foreach($name in @('amd_fidelityfx_loader_dx12.dll','amd_fidelityfx_framegeneration_dx12.dll')) { Copy-Item (Join-Path $PSScriptRoot ('third_party\fidelityfx\bin\'+$name)) (Join-Path $out ('ControlFGFidelityFX\'+$name)) }
    foreach($name in @('TESTING-FSR3.md','Collect-ControlFG-Logs.cmd','Collect-ControlFG-Logs.ps1')) { Copy-Item (Join-Path $PSScriptRoot $name) (Join-Path $out $name) }
    @'
CONTROL FG - FSR3 FRAME GENERATION v1.1.0-alpha1
Experimental fixed 2x, synchronous, SDR-only test build.
Copy dxgi.dll and ControlFGFidelityFX beside Control_DX12.exe.
Remove the public DLSS-G dxgi.dll and ControlFGStreamline first.
This alpha enables FSR3 automatically. Run Collect-ControlFG-Logs.cmd after testing.
'@ | Set-Content -LiteralPath (Join-Path $out 'README.txt') -Encoding UTF8
    New-Item -ItemType Directory -Path (Split-Path $zip) -Force | Out-Null
    Compress-Archive -Path (Join-Path $out '*') -DestinationPath $zip -CompressionLevel Optimal
    Write-Host ('PASS: '+$zip)
} catch { Write-Error $_; exit 1 }
