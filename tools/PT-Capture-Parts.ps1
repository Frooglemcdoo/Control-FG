function Export-PTCaptureParts {
    param(
        [Parameter(Mandatory=$true)][string]$Source,
        [Parameter(Mandatory=$true)][string]$OutputDirectory,
        [ValidateRange(1,48)][int]$PartMiB=48
    )
    $ErrorActionPreference='Stop'
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $file=Get-Item -LiteralPath $Source
    if($file.Length -eq 0){throw 'Cannot package an empty capture.'}
    if(Test-Path -LiteralPath $OutputDirectory){throw 'Upload directory already exists; choose a fresh destination.'}
    New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
    $limit=[int64]$PartMiB*1MB
    if($file.Length -le $limit){
        Copy-Item -LiteralPath $file.FullName -Destination $OutputDirectory
        return
    }
    $total=[int][Math]::Ceiling($file.Length/[double]$limit)
    $sourceHash=(Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash.ToLowerInvariant()
    $inputStream=[IO.File]::OpenRead($file.FullName)
    $buffer=New-Object byte[] (1MB)
    try {
        for($part=1;$part -le $total;$part++){
            $offset=$inputStream.Position
            $length=[Math]::Min($limit,$file.Length-$offset)
            $name=('Part-{0:D3}-of-{1:D3}.zip' -f $part,$total)
            $archive=[IO.Compression.ZipFile]::Open((Join-Path $OutputDirectory $name),[IO.Compression.ZipArchiveMode]::Create)
            try {
                $entry=$archive.CreateEntry('Payload.part',[IO.Compression.CompressionLevel]::NoCompression)
                $payload=$entry.Open()
                try {
                    $remaining=$length
                    while($remaining -gt 0){
                        $read=$inputStream.Read($buffer,0,[int][Math]::Min($buffer.Length,$remaining))
                        if($read -le 0){throw 'Unexpected end of capture ZIP.'}
                        $payload.Write($buffer,0,$read);$remaining-=$read
                    }
                } finally {$payload.Dispose()}
                $metadata=[ordered]@{Format='Control-PT-Frame-Split-v1';SourceName=$file.Name;SourceBytes=$file.Length;SourceSHA256=$sourceHash;Part=$part;Parts=$total;Offset=$offset;Bytes=$length}
                $entry=$archive.CreateEntry('Part.json')
                $writer=New-Object IO.StreamWriter($entry.Open())
                try {$writer.Write(($metadata | ConvertTo-Json))} finally {$writer.Dispose()}
            } finally {$archive.Dispose()}
        }
        if($inputStream.Position -ne $file.Length){throw 'Capture split did not consume the entire source.'}
    } finally {$inputStream.Dispose()}
}
