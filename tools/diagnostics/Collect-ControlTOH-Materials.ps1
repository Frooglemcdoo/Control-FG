param([string]$LogDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if ([string]::IsNullOrWhiteSpace($LogDirectory)) {
    if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA is unavailable; specify -LogDirectory.' }
    $LogDirectory=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) { $OutputDirectory=[Environment]::GetFolderPath('Desktop') }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) { throw 'Desktop is unavailable; specify -OutputDirectory.' }
$LogDirectory=(Resolve-Path -LiteralPath $LogDirectory).Path
$candidates=@(Get-ChildItem -LiteralPath $LogDirectory -Directory -Filter 'rr-material-shaders-*' | Where-Object {
    -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and
    @(Get-ChildItem -LiteralPath $_.FullName -File -Filter 'family-4-*-audit.json').Count -gt 0
} | Sort-Object LastWriteTimeUtc -Descending)
if (-not $candidates.Count) { throw 'No R258 foliage shader capture found. At the plant use F10 > CAPTURE MATERIALS, close F10, wait 10 seconds, then run this collector.' }
$source=$candidates[0]
if ($source.Name -notmatch '^rr-material-shaders-(\d+)-(\d+)$') { throw 'Unexpected material capture directory.' }
$capturePid=$Matches[1];$session=$Matches[2]
$files=[Collections.Generic.List[string]]::new()
$pairs=0
foreach ($metadata in @(Get-ChildItem -LiteralPath $source.FullName -File -Filter 'family-4-*-audit.json')) {
    if ($metadata.Name -notmatch '^family-4-batch-(\d+)-frame-(\d+)-audit\.json$') { throw 'Unexpected audit filename.' }
    $batch=$Matches[1];$frame=$Matches[2];$prefix=$metadata.FullName.Substring(0,$metadata.FullName.Length-11)
    $marker=$prefix+'.complete'
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) { continue }
    $m=Get-Content -LiteralPath $metadata.FullName -Raw | ConvertFrom-Json
    if ($m.schema -ne 'ControlTOH.MaterialAudit.R258' -or $m.family -ne 4 -or
        "$($m.pid)" -ne $capturePid -or "$($m.session)" -ne $session -or
        "$($m.frame)" -ne $frame -or "$($m.source_batch)" -ne $batch -or @($m.byte_sizes).Count -ne 4) { throw 'Audit metadata does not match the capture.' }
    $suffixes=@('-original-vs.dxbc','-original-ps.dxbc','-candidate-vs.dxbc','-candidate-ps.dxbc')
    for ($i=0;$i -lt 4;$i++) {
        $path=$prefix+$suffixes[$i];$f=Get-Item -LiteralPath $path
        if (($f.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $f.Length -ne $m.byte_sizes[$i] -or $f.Length -lt 32 -or $f.Length -gt 262144) { throw 'Incomplete or invalid shader file.' }
        $files.Add($path)
    }
    $files.Add($metadata.FullName);$files.Add($marker);$pairs++
}
if (-not $pairs) { throw 'Shader export has not completed. Wait 10 seconds and run this collector again.' }
if ($pairs -gt 24) { throw 'Unexpected shader count; capture exceeds its bounded contract.' }
$tag=(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
$staging=Join-Path ([IO.Path]::GetTempPath()) ('ControlTOH-Materials-'+$tag)
[IO.Directory]::CreateDirectory($staging) | Out-Null
try {
    $records=@()
    foreach ($path in $files) {
        $f=Get-Item -LiteralPath $path
        if ($f.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Capture files must be regular files.' }
        $dest=Join-Path $staging $f.Name;Copy-Item -LiteralPath $path -Destination $dest
        $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash -ne $hash) { throw 'Capture changed while collecting.' }
        $records+=@{name=$f.Name;bytes=$f.Length;sha256=$hash}
    }
    $logs=@(Get-ChildItem -LiteralPath $LogDirectory -File -Filter "probe-*-$capturePid.log" | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1)
    foreach ($log in $logs) {
        if ($log.Length -le 32MB -and -not ($log.Attributes -band [IO.FileAttributes]::ReparsePoint)) { Copy-Item -LiteralPath $log.FullName -Destination $staging }
    }
    @{schema='ControlTOH.MaterialPackage.R258';pid=$capturePid;session=$session;pairs=$pairs;files=$records;image_buffers_included=$false} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $staging 'manifest.json') -Encoding UTF8
    [IO.Directory]::CreateDirectory([IO.Path]::GetFullPath($OutputDirectory)) | Out-Null
    $output=Join-Path $OutputDirectory ('ControlTOH-Materials-R258-'+$tag+'.zip')
    Compress-Archive -LiteralPath @(Get-ChildItem -LiteralPath $staging -File | ForEach-Object { $_.FullName }) -DestinationPath $output
    Write-Host "Saved $pairs complete foliage shader pairs: $output"
    Write-Host 'Send this ZIP. Existing RR image buffers are not needed again.'
} finally {
    Remove-Item -LiteralPath $staging -Recurse -Force
}
