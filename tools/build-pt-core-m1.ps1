$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$out = Join-Path $PWD 'build\pt-core-m1'
New-Item -ItemType Directory -Force -Path $out | Out-Null
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'Run from an x64 Visual Studio developer shell.' }
$dxc = $null
foreach ($root in @("${env:ProgramFiles(x86)}\Windows Kits\10\bin", "${env:ProgramFiles}\Microsoft Visual Studio")) {
    if (-not (Test-Path $root)) { continue }
    $candidate = Get-ChildItem -LiteralPath $root -Filter dxc.exe -File -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
    if ($candidate) { $dxc = $candidate.FullName; break }
}
if (-not $dxc) { throw 'DXC x64 was not found in the installed SDK.' }
foreach ($name in @('surface-transport','geometry-bounds','direct-light','scene-admission')) {
    & cl.exe /nologo /std:c++17 /EHsc /O2 /W4 /MT "tools\test-pt-$name.cpp" "/Fo$out\test-$name.obj" "/Fe$out\test-$name.exe"
    if ($LASTEXITCODE -ne 0) { throw "C++ build failed: $name" }
    & "$out\test-$name.exe" | Tee-Object -FilePath "$out\test-$name.txt"
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $name" }
}
foreach ($shader in @(
    @{Source='src/shaders/pt_scene_hits.hlsl';Profile='lib_6_3';Name='pt-scene-hits'},
    @{Source='tools/test-pt-path-query.hlsl';Profile='lib_6_3';Name='pt-path-query-test'},
    @{Source='tools/test-pt-scene-query.hlsl';Profile='lib_6_3';Name='pt-scene-query-test'},
    @{Source='tools/test-pt-surface-transport.hlsl';Profile='cs_6_0';Name='pt-transport-test'}
)) {
    $arguments = @('-T',$shader.Profile,'-HV','2021','-O3','-Fo',"$out\$($shader.Name).dxil",$shader.Source)
    if ($shader.Profile -eq 'cs_6_0') { $arguments += @('-E','main') }
    & $dxc @arguments
    if ($LASTEXITCODE -ne 0) { throw "DXC build failed: $($shader.Name)" }
    & $dxc -dumpbin "$out\$($shader.Name).dxil" | Set-Content "$out\$($shader.Name).dump.txt"
    if ($LASTEXITCODE -ne 0) { throw 'DXC reflection failed' }
}
Copy-Item research/pt-reflections/PT-Core-M1-README.txt "$out\README.txt"
Get-ChildItem $out -File | Where-Object { $_.Extension -in @('.exe','.dxil') } |
    Get-FileHash -Algorithm SHA256 | Select-Object @{Name='File';Expression={Split-Path $_.Path -Leaf}},Hash |
    ConvertTo-Json | Set-Content "$out\hashes.json"
Write-Host 'PASS: PT Core M1 component build. GPU execution and game integration are NOT tested.'
