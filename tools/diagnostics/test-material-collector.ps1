$ErrorActionPreference='Stop'
$root=Join-Path ([IO.Path]::GetTempPath()) ('TOH collector test '+[Guid]::NewGuid().ToString('N'))
$logs=Join-Path $root 'logs';$output=Join-Path $root 'output with spaces'
$source=Join-Path $logs 'rr-material-shaders-1234-500'
[IO.Directory]::CreateDirectory($source) | Out-Null
try {
    $prefix=Join-Path $source 'family-4-batch-9-frame-501'
    $suffixes=@('-original-vs.dxbc','-original-ps.dxbc','-candidate-vs.dxbc','-candidate-ps.dxbc')
    foreach ($suffix in $suffixes) { [IO.File]::WriteAllBytes(($prefix+$suffix),[byte[]](0..63)) }
    @{schema='ControlTOH.MaterialAudit.R258';pid=1234;session=500;frame=501;family=4;source_batch=9;byte_sizes=@(64,64,64,64)} | ConvertTo-Json | Set-Content -LiteralPath ($prefix+'-audit.json')
    # A second incomplete pair must not be exported.
    Copy-Item -LiteralPath ($prefix+'-audit.json') -Destination (Join-Path $source 'family-4-batch-10-frame-502-audit.json')
    'complete' | Set-Content -LiteralPath ($prefix+'.complete')
    'probe' | Set-Content -LiteralPath (Join-Path $logs 'probe-test-1234.log')
    $images=Join-Path $logs 'rr-input-c1-1234-501';[IO.Directory]::CreateDirectory($images)|Out-Null
    'must not ship' | Set-Content -LiteralPath (Join-Path $images 'input-color.rgba16f')
    & "$PSScriptRoot/Collect-ControlTOH-Materials.ps1" -LogDirectory $logs -OutputDirectory $output
    $archive=@(Get-ChildItem -LiteralPath $output -Filter '*.zip');if($archive.Count -ne 1){throw 'Expected one ZIP'}
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive[0].FullName)
    try {
        $names=@($zip.Entries|ForEach-Object{$_.Name})
        if($names.Count -ne 8 -or $names -contains 'input-color.rgba16f' -or $names -contains 'family-4-batch-10-frame-502-audit.json'){throw 'Unexpected packaged files'}
        $entry=$zip.GetEntry('manifest.json');$reader=[IO.StreamReader]::new($entry.Open())
        try {$manifest=$reader.ReadToEnd()|ConvertFrom-Json}finally{$reader.Dispose()}
        if($manifest.pairs -ne 1 -or $manifest.files.Count -ne 6 -or $manifest.image_buffers_included){throw 'Wrong package manifest'}
    }finally{$zip.Dispose()}
    # Truncation must be rejected instead of producing a misleading second ZIP.
    [IO.File]::WriteAllBytes(($prefix+$suffixes[0]),[byte[]](0..10))
    $rejected=$false
    try { & "$PSScriptRoot/Collect-ControlTOH-Materials.ps1" -LogDirectory $logs -OutputDirectory $output }catch{$rejected=$true}
    if(-not $rejected -or @(Get-ChildItem -LiteralPath $output -Filter '*.zip').Count -ne 1){throw 'Truncated shader not rejected'}
    Write-Host 'PASS material collector: complete pairs, hash manifest, paths with spaces, incomplete/truncated rejection, no image buffers'
}finally{Remove-Item -LiteralPath $root -Recurse -Force}
