$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build-pt-core-m1.ps1')
if ($LASTEXITCODE -ne 0) { throw 'M1 prerequisite failed' }
$out = Join-Path $PWD 'build\pt-dxr-m2'
New-Item -ItemType Directory -Force -Path $out | Out-Null
& cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /MT tools\test-pt-dxr-gpu.cpp "/Fo$out\gpu-test.obj" "/Fe$out\PT-DXR-GPU-Test.exe" /link d3d12.lib dxgi.lib
if ($LASTEXITCODE -ne 0) { throw 'DXR host build failed' }
Copy-Item build/pt-core-m1/pt-scene-query-test.dxil $out
Copy-Item tools/Run-PT-DXR-Test.cmd $out
Copy-Item research/pt-reflections/PT-DXR-M2-README.txt "$out\README.txt"
Push-Location $out
try {
    & .\PT-DXR-GPU-Test.exe --allow-no-dxr | Tee-Object -FilePath CI-GPU-Test-Results.txt
    if ($LASTEXITCODE -ne 0) { throw 'GPU test failed on available CI hardware' }
} finally { Pop-Location }
@{commit=$env:GITHUB_SHA;gpuValidation='See CI-GPU-Test-Results.txt; SKIP is not a GPU pass'} | ConvertTo-Json | Set-Content "$out\build-info.json"
Get-ChildItem $out -File | Where-Object { $_.Extension -in @('.exe','.dxil','.cmd') } |
    Get-FileHash -Algorithm SHA256 | Select-Object @{Name='File';Expression={Split-Path $_.Path -Leaf}},Hash |
    ConvertTo-Json | Set-Content "$out\hashes.json"
