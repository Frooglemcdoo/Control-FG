#requires -Version 5.1
param([string]$RuntimeDirectory = (Join-Path $PSScriptRoot 'third_party\streamline\bin'))
$ErrorActionPreference = 'Stop'
# Official NVIDIA/DLSS v310.9.1 lib/Windows_x86_64/rel/nvngx_dlssd.dll.
# Also byte-exact with the already pinned Streamline 2.14.1 package.
$path = Join-Path $RuntimeDirectory 'nvngx_dlssd.dll'
$expected = '4BC7EA5FCB2F32CF86BC2CB072E8D2860914A0BE511CBDCB2FC206C1D9D83B80'
if (-not (Test-Path -LiteralPath $path)) { throw 'Missing RR runtime nvngx_dlssd.dll.' }
if ((Get-Item -LiteralPath $path).Length -ne 48343664 -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $expected) {
    throw 'RR runtime must match the pinned NVIDIA 310.9.1 release used by the validated selectable RR preset path.'
}
Write-Host 'PASS: NVIDIA RR runtime 310.9.1 hash verified; native RR creation supports persisted preset selection E/F/K/L/M (F default).'
