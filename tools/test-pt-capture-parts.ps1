$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'PT-Capture-Parts.ps1'),[ref]$tokens,[ref]$errors) | Out-Null
if($errors.Count){throw ($errors | Out-String)}
. (Join-Path $PSScriptRoot 'PT-Capture-Parts.ps1')
$root=Join-Path $env:TEMP ('PT-Split-Test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
try {
    $source=Join-Path $root 'capture.zip'
    $bytes=New-Object byte[] (2MB+12345)
    $rng=New-Object Random(1234);$rng.NextBytes($bytes)
    [IO.File]::WriteAllBytes($source,$bytes)
    $expected=(Get-FileHash $source -Algorithm SHA256).Hash.ToLowerInvariant()
    $output=Join-Path $root 'parts'
    Export-PTCaptureParts -Source $source -OutputDirectory $output -PartMiB 1
    $parts=@(Get-ChildItem $output -Filter '*.zip' | Sort-Object Name)
    if($parts.Count -ne 3){throw 'Expected three bounded ZIP parts'}
    $joined=Join-Path $root 'joined.zip';$stream=[IO.File]::Create($joined)
    try {
        $ordinal=0
        foreach($part in $parts){
            if($part.Length -gt (1MB+4096)){throw 'Part exceeded bounded payload plus ZIP overhead'}
            $archive=[IO.Compression.ZipFile]::OpenRead($part.FullName)
            try {
                $reader=New-Object IO.StreamReader($archive.GetEntry('Part.json').Open())
                try {$meta=$reader.ReadToEnd() | ConvertFrom-Json} finally {$reader.Dispose()}
                $ordinal++
                if($meta.Part -ne $ordinal -or $meta.Parts -ne 3 -or $meta.Offset -ne $stream.Position -or $meta.SourceSHA256 -ne $expected){throw 'Invalid part identity/order/offset'}
                $payload=$archive.GetEntry('Payload.part').Open()
                try {$payload.CopyTo($stream)} finally {$payload.Dispose()}
            } finally {$archive.Dispose()}
        }
    } finally {$stream.Dispose()}
    if((Get-FileHash $joined -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected){throw 'Reassembled capture differs'}
    $small=Join-Path $root 'small.zip';[IO.File]::WriteAllBytes($small,[byte[]](1,2,3))
    Export-PTCaptureParts -Source $small -OutputDirectory (Join-Path $root 'small') -PartMiB 1
    if((Get-FileHash $small).Hash -ne (Get-FileHash (Join-Path $root 'small/small.zip')).Hash){throw 'Small capture changed'}
    $rejected=$false
    try {Export-PTCaptureParts -Source $source -OutputDirectory $output -PartMiB 1} catch {$rejected=$_.Exception.Message -match 'already exists'}
    if(-not $rejected){throw 'Existing upload directory was overwritten'}
    Write-Host 'PASS: bounded capture parts reassemble exactly; small capture and existing output protected.'
} finally {Remove-Item -LiteralPath $root -Recurse -Force}
