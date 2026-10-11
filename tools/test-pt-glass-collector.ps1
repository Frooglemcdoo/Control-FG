$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$temp=Join-Path ([IO.Path]::GetTempPath()) ('ControlFG-Glass-Test-'+[guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path "$temp/logs","$temp/output","$temp/project" -Force | Out-Null
    $log=Join-Path $temp 'logs/probe-20261011-220000-000-12345.log'
    $writer=New-Object IO.StreamWriter($log,$false,[Text.Encoding]::UTF8)
    try {
        $writer.WriteLine('qpc=1 tid=1 PROBE v2.1.2 internal_build=test ')
        $padding='x'*1024
        for($i=0;$i -lt 4096;$i++) { $writer.WriteLine($padding) }
        $writer.WriteLine('qpc=2 tid=1 PT_GLASS_DXIL_BEGIN index=0 bytes=4')
        $writer.WriteLine('qpc=3 tid=1 PT_GLASS_DXIL_CHUNK index=0 offset=0 bytes=4 hex=44584243')
        $writer.WriteLine('qpc=4 tid=1 PT_GLASS_DXIL_END index=0 bytes=4')
        for($i=0;$i -lt 4096;$i++) { $writer.WriteLine($padding) }
    } finally { $writer.Dispose() }
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$root/Collect-ControlFG-Compact-Logs.ps1" -LogDirectory "$temp/logs" -ProjectDirectory "$temp/project" -OutputDirectory "$temp/output"
    if($LASTEXITCODE -ne 0) { throw 'Collector failed.' }
    $zip=Get-ChildItem "$temp/output" -Filter '*.zip' | Select-Object -First 1
    if(-not $zip) { throw 'Collector did not produce a ZIP.' }
    Expand-Archive $zip.FullName "$temp/extracted"
    $glass=Get-Content "$temp/extracted/glass/probe-20261011-220000-000-12345.log" -Raw
    foreach($marker in @('PT_GLASS_DXIL_BEGIN','hex=44584243','PT_GLASS_DXIL_END')) {
        if(-not $glass.Contains($marker)) { throw "Missing capture marker: $marker" }
    }
    $report=Get-Content "$temp/extracted/collection-report.json" -Raw | ConvertFrom-Json
    if($report.IncludedDataBytes -le 0) { throw 'Invalid included-byte accounting.' }
    Write-Host 'PASS: glass capture retained outside compact log tail; accounting valid.'
} finally {
    if(Test-Path $temp) { Remove-Item $temp -Recurse -Force }
}
