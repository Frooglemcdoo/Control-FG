$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build-pt-dxr-m3.ps1')
if ($LASTEXITCODE -ne 0) { throw 'M1 prerequisite failed' }
$out = Join-Path $PWD 'build\pt-dxr-m4'
New-Item -ItemType Directory -Force -Path $out | Out-Null
& cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /MT /DPT_PATH_TEST /DPT_GEOMETRY_SPAWN tools\test-pt-dxr-gpu.cpp "/Fo$out\gpu-test.obj" "/Fe$out\PT-Geometry-GPU-Test.exe" /link d3d12.lib dxgi.lib
if ($LASTEXITCODE -ne 0) { throw 'DXR host build failed' }
$dxc = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Filter dxc.exe -File -Recurse |
    Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
& $dxc.FullName -T lib_6_3 -HV 2021 -O3 -DPT_GEOMETRY_SPAWN -Fo "$out\pt-geometry-query-test.dxil" tools/test-pt-path-query.hlsl
if ($LASTEXITCODE -ne 0) { throw 'Geometry shader compile failed' }
Copy-Item research/pt-reflections/NVIDIA-SIA-LICENSE.txt $out
Copy-Item build/pt-dxr-m2/PT-DXR-GPU-Test.exe $out
Copy-Item build/pt-dxr-m2/pt-scene-query-test.dxil $out
Copy-Item tools/Run-PT-Geometry-Test.cmd $out
Copy-Item research/pt-reflections/PT-Geometry-M4-README.txt "$out\README.txt"
Push-Location $out
try {
    foreach ($scenario in @('', '--translated', '--scaled', '--worker-reject')) {
        & .\PT-Geometry-GPU-Test.exe --allow-no-dxr $scenario | Tee-Object -FilePath CI-GPU-Test-Results.txt -Append
        if ($LASTEXITCODE -ne 0) { throw "Geometry test failed: $scenario" }
    }
} finally { Pop-Location }
@{commit=$env:GITHUB_SHA;gpuValidation='See CI-GPU-Test-Results.txt; SKIP is not a GPU pass'} | ConvertTo-Json | Set-Content "$out\build-info.json"
Get-ChildItem $out -File | Where-Object { $_.Extension -in @('.exe','.dxil','.cmd') } |
    Get-FileHash -Algorithm SHA256 | Select-Object @{Name='File';Expression={Split-Path $_.Path -Leaf}},Hash |
    ConvertTo-Json | Set-Content "$out\hashes.json"
