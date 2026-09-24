function Copy-ControlFGLiveGuides($LogDir, $Stage, $RunPids, $Notes) {
    $records = @()
    foreach ($capture in @(Get-ChildItem -LiteralPath $LogDir -Directory | Where-Object { $_.Name -like 'rr-live-g15-*' -or $_.Name -like 'rr-distance-r20-*' } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 4)) {
        $destination = Join-Path (Join-Path $Stage 'rr-guides') $capture.Name
        try {
            if ($capture.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'live capture is a reparse point' }
            $metaFile = Get-Item -LiteralPath (Join-Path $capture.FullName 'metadata.json')
            if ($metaFile.Length -gt 65536 -or ($metaFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'invalid live metadata' }
            $meta = Get-Content -LiteralPath $metaFile.FullName -Raw | ConvertFrom-Json
            if ($meta.schema -cne 'control-rr-live-g15' -and $meta.schema -cne 'control-rr-distance-r20') { continue }
            if (-not $RunPids.ContainsKey([string]$meta.pid)) { continue }
            if (-not (Test-ControlFGInteger $meta.width) -or -not (Test-ControlFGInteger $meta.height) -or $meta.width -lt 64 -or $meta.height -lt 64 -or $meta.width -gt 8192 -or $meta.height -gt 8192) { throw 'invalid live extent' }
            [long]$pixels = [long]$meta.width * [long]$meta.height
            if ($pixels -gt 8388608) { throw 'invalid capture extent' }
            if ($meta.schema -ceq 'control-rr-distance-r20') {
                if ($meta.representative_ray -ne 0 -or $meta.rr_lighting -ne $true) { throw 'invalid distance contract' }
                $sizes = [ordered]@{'distance.r32f'=($pixels*4);'status.r32u'=($pixels*4);'metadata.json'=$metaFile.Length}
            } else {
                if ($meta.source -cne 'live_owned_outputs_same_dispatch' -or ($meta.rr_evaluation -ne $false -and -not ($null -eq $meta.rr_evaluation -and $meta.rr_lighting -eq $true)) -or $meta.coverage_proven -ne $false) { throw 'invalid live contract' }
                $sizes = [ordered]@{'normal-roughness.rgba32f'=($pixels*16);'specular.rgba32f'=($pixels*16);'diffuse.rgba16f'=($pixels*8);'metadata.json'=$metaFile.Length}
            }
            New-Item -ItemType Directory -Path $destination | Out-Null
            $hashes = @(); [long]$total = 0
            foreach ($name in $sizes.Keys) {
                $file = Get-Item -LiteralPath (Join-Path $capture.FullName $name)
                if ($file.PSIsContainer -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $file.Length -ne $sizes[$name]) { throw ('invalid live member: ' + $name) }
                $inputStream = $null; $outputStream = $null; $copied = Join-Path $destination $name
                try {
                    $inputStream = [IO.File]::Open($file.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
                    if ($inputStream.Length -ne $sizes[$name]) { throw 'live member length changed' }
                    $outputStream = [IO.File]::Open($copied,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
                    $inputStream.CopyTo($outputStream)
                } finally {
                    if ($null -ne $outputStream) { $outputStream.Dispose() }
                    if ($null -ne $inputStream) { $inputStream.Dispose() }
                }
                if ((Get-Item -LiteralPath $copied).Length -ne $sizes[$name]) { throw 'incomplete live member' }
                $total += $sizes[$name]
                $hashes += [ordered]@{Name=$name;Bytes=$sizes[$name];SHA256=(Get-FileHash -LiteralPath $copied -Algorithm SHA256).Hash}
            }
            $records += [ordered]@{Directory=$capture.Name;Bytes=$total;LiveOutputs=$true;Frame=$meta.frame;Files=$hashes}
        } catch {
            $Notes.Add(('Skipped live capture ' + $capture.Name + ': ' + $_.Exception.Message))
            if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Recurse -Force }
        }
    }
    return $records
}
