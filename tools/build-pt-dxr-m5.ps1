$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'build-pt-dxr-m4.ps1')
if ($LASTEXITCODE -ne 0) { throw 'Adapter prerequisite failed' }
$out=Join-Path $PWD 'build\pt-dxr-m5'
New-Item -ItemType Directory -Force -Path $out | Out-Null
Copy-Item build/pt-dxr-m4/*.exe,build/pt-dxr-m4/*.dxil,build/pt-dxr-m4/*.cmd,build/pt-dxr-m4/*.json,build/pt-dxr-m4/*.txt $out
Copy-Item research/pt-reflections/PT-Scene-Adapter-M5-README.txt "$out\README.txt"
Copy-Item research/pt-reflections/entrance-native-sbt-summary.json $out
