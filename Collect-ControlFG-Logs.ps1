#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
function Test-ControlFGInteger($Value) {
    return ($Value -is [int] -or $Value -is [long])
}
function Initialize-ControlFGShaderHash {
    if ($null -eq ('ControlFG.CollectionShaderHash' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
namespace ControlFG {
    public static class CollectionShaderHash {
        public static string Fnv1a64(byte[] bytes) {
            ulong value = 14695981039346656037UL;
            unchecked {
                foreach (byte item in bytes) { value ^= item; value *= 1099511628211UL; }
            }
            return value.ToString("x16");
        }
    }
}
'@
    }
}
try {
    . (Join-Path $PSScriptRoot 'Build-Metadata.ps1')
    $logDir = Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    if (-not (Test-Path -LiteralPath $logDir -PathType Container)) { throw 'No Control FG log directory exists yet. Run Control with G11 first.' }
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $suffix = [guid]::NewGuid().ToString('N').Substring(0,8)
    $stage = Join-Path $env:TEMP ('ControlFG-G11-Collect-' + $suffix)
    $zip = Join-Path $PSScriptRoot ('Control-FG-v' + $ControlFGBuild.Version + '-Logs-' + $stamp + '-' + $suffix + '.zip')
    $identity = ' PROBE v' + $ControlFGBuild.Version + ' '
    $probeLogs = @(Get-ChildItem -LiteralPath $logDir -File -Filter 'probe-*.log' |
        Where-Object { $_.Length -le 32MB -and -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and ([string](Get-Content -LiteralPath $_.FullName -TotalCount 1)).Contains($identity) } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 5)
    if (-not $probeLogs.Count) { throw 'No matching Control FG probe log was found. Run Control DX12 with this build first.' }
    $runPids = @{}
    foreach ($log in $probeLogs) {
        if ($log.Name -match '^probe-\d{8}-\d{6}-\d{3}-(\d+)\.log$') { $runPids[$Matches[1]] = $true }
    }
    $notes = New-Object 'System.Collections.Generic.List[string]'
    $captureRecords = @()
    $allowNames = @($ControlFGCapture.Images | ForEach-Object { $_.Name }) + @($ControlFGCapture.MetadataName)
    $capturePattern = '^' + [regex]::Escape($ControlFGCapture.DirectoryPrefix) + '\d{8}-\d{6}-\d{3}-(\d+)(?:-\d+)?$'
    New-Item -ItemType Directory -Path $stage | Out-Null
    try {
        foreach ($name in @('probe','streamline','rr-guides','package')) { New-Item -ItemType Directory -Path (Join-Path $stage $name) | Out-Null }
        foreach ($log in $probeLogs) { Copy-Item -LiteralPath $log.FullName -Destination (Join-Path (Join-Path $stage 'probe') $log.Name) }
        $completed = @(Get-ChildItem -LiteralPath $logDir -Directory -Filter ($ControlFGCapture.DirectoryPrefix + '*') |
            Where-Object { -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and (Test-Path -LiteralPath (Join-Path $_.FullName 'metadata.json') -PathType Leaf) } |
            Sort-Object LastWriteTimeUtc -Descending)
        foreach ($capture in $completed) {
            if ($captureRecords.Count -ge $ControlFGCapture.MaximumCaptures) { break }
            if ($capture.Name -notmatch $capturePattern -or -not $runPids.ContainsKey($Matches[1])) { continue }
            try {
                $metadataPath = Join-Path $capture.FullName 'metadata.json'
                $metadataFile = Get-Item -LiteralPath $metadataPath
                if ($metadataFile.Length -gt 1MB -or ($metadataFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'invalid metadata file' }
                $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
                if ($metadata.schema -isnot [string] -or $metadata.status -isnot [string] -or
                    $metadata.schema -cne $ControlFGCapture.Schema -or $metadata.status -cne $ControlFGCapture.CompletedStatus) { throw 'metadata is not a completed G11 export' }
                if (-not (Test-ControlFGInteger $metadata.engine_frame) -or $metadata.engine_frame -lt 0 -or
                    $metadata.semantics_unverified -isnot [bool] -or -not $metadata.semantics_unverified) { throw 'capture frame or diagnostic semantics are invalid' }
                if (-not (Test-ControlFGInteger $metadata.width) -or -not (Test-ControlFGInteger $metadata.height)) { throw 'capture dimensions must be integers' }
                [long]$width = $metadata.width; [long]$height = $metadata.height
                if ($width -le 0 -or $height -le 0 -or $width -gt $ControlFGCapture.MaximumDimension -or $height -gt $ControlFGCapture.MaximumDimension) { throw 'invalid capture dimensions' }
                [long]$pixels = $width * $height
                if ($pixels -gt $ControlFGCapture.MaximumPixels) { throw 'capture pixel count exceeds the runtime limit' }
                [long]$bmpLength = 54 + ((($width*3 + 3) -band -4) * $height)
                if ($metadata.has_native_albedo_outputs -isnot [bool] -or
                    -not (Test-ControlFGInteger $metadata.native_albedo_output_count) -or
                    $metadata.native_albedo_images -isnot [array]) { throw 'native image manifest fields are missing or have incorrect types' }
                $nativeCount = $metadata.native_albedo_output_count
                if ($nativeCount -lt 0 -or $nativeCount -gt $ControlFGCapture.NativeTargetLimit -or
                    $metadata.native_albedo_images.Count -ne $nativeCount -or
                    $metadata.has_native_albedo_outputs -ne ($nativeCount -gt 0)) { throw 'native image count does not match the manifest' }
                $captureCounts = $metadata.native_albedo_capture_counts
                if ($null -eq $captureCounts) { throw 'native capture counts are missing' }
                foreach ($countName in @('source_batches','accepted_batches','replayed_batches','draw_ranges','skipped_ranges')) {
                    $countProperty = $captureCounts.PSObject.Properties[$countName]
                    if ($null -eq $countProperty -or -not (Test-ControlFGInteger $countProperty.Value) -or
                        $countProperty.Value -lt 0 -or $countProperty.Value -gt 4294967295) { throw ('invalid native capture count: ' + $countName) }
                }
                $requiredNames = @($allowNames)
                $expectedBytes = @{}
                if ($metadata.gpu_reflectance_candidates) {
                    if ($metadata.gpu_reflectance_candidates -isnot [bool] -or
                        -not (Test-ControlFGInteger $metadata.reflectance_source_frame) -or
                        $metadata.reflectance_source_frame -ne $metadata.engine_frame -or
                        $metadata.reflectance_coverage_validated -isnot [bool] -or $metadata.reflectance_coverage_validated -or
                        $metadata.reflectance_rr_validated -isnot [bool] -or $metadata.reflectance_rr_validated -or
                        -not (Test-ControlFGInteger $metadata.reflectance_diffuse_bytes) -or
                        -not (Test-ControlFGInteger $metadata.reflectance_specular_bytes) -or
                        $metadata.reflectance_diffuse_bytes -ne ($pixels*8) -or
                        $metadata.reflectance_specular_bytes -ne ($pixels*16) -or
                        $metadata.reflectance_diffuse_fnv1a64 -isnot [string] -or
                        $metadata.reflectance_specular_fnv1a64 -isnot [string] -or
                        $metadata.reflectance_diffuse_fnv1a64 -cnotmatch '^[0-9a-f]{16}$' -or
                        $metadata.reflectance_specular_fnv1a64 -cnotmatch '^[0-9a-f]{16}$') {
                        throw 'invalid GPU reflectance candidate manifest'
                    }
                    $requiredNames += 'diffuse-reflectance-candidate.rgba16f','specular-reflectance-candidate.rgba32f'
                    $expectedBytes['diffuse-reflectance-candidate.rgba16f']=$pixels*8
                    $expectedBytes['specular-reflectance-candidate.rgba32f']=$pixels*16
                }
                if ($null -ne $metadata.part1_bytes -and $metadata.part1_bytes -ne 0) {
                    if (-not (Test-ControlFGInteger $metadata.part1_bytes) -or
                        -not (Test-ControlFGInteger $metadata.part1_stride) -or
                        -not (Test-ControlFGInteger $metadata.part1_records) -or
                        -not (Test-ControlFGInteger $metadata.part1_source_frame) -or
                        $metadata.part1_fnv1a64 -isnot [string] -or
                        $metadata.part1_fnv1a64 -cnotmatch '^[0-9a-f]{16}$' -or
                        $metadata.part1_bytes -lt 8 -or $metadata.part1_bytes -gt 524288 -or
                        ($metadata.part1_bytes % 8) -ne 0 -or $metadata.part1_stride -ne 8 -or
                        $metadata.part1_records -ne ($metadata.part1_bytes / 8) -or
                        $metadata.part1_file -cne 'material-part1.bin' -or
                        $metadata.part1_source_frame -ne $metadata.engine_frame -or
                        $metadata.part1_rr_validated -isnot [bool] -or $metadata.part1_rr_validated) {
                        throw 'invalid Part1 readback manifest'
                    }
                    $requiredNames += 'material-part1.bin'
                    $expectedBytes['material-part1.bin'] = $metadata.part1_bytes
                }
                if ($metadata.material_rejection_audit_file -isnot [string] -or
                    $metadata.material_rejection_audit_file -cne $ControlFGCapture.MaterialRejectionAuditName -or
                    -not (Test-ControlFGInteger $metadata.material_rejection_audit_bytes) -or
                    $metadata.material_rejection_audit_bytes -le 0 -or
                    $metadata.material_rejection_audit_bytes -gt $ControlFGCapture.MaximumMaterialRejectionAuditBytes) { throw 'material rejection audit manifest is missing or invalid' }
                $auditName = $ControlFGCapture.MaterialRejectionAuditName
                $requiredNames += $auditName; $expectedBytes[$auditName] = $metadata.material_rejection_audit_bytes
                $auditPath = Join-Path $capture.FullName $auditName
                $audit = Get-Content -LiteralPath $auditPath -Raw | ConvertFrom-Json
                if ($audit.schema -isnot [string] -or $audit.schema -cne 'ControlFG.RRMaterialRejectionAudit.v4' -or
                    -not (Test-ControlFGInteger $audit.engine_frame) -or $audit.engine_frame -ne $metadata.engine_frame -or
                    -not (Test-ControlFGInteger $audit.source_batches) -or -not (Test-ControlFGInteger $audit.accepted_batches) -or
                    -not (Test-ControlFGInteger $audit.rejected_batches) -or $audit.source_batches -ne $captureCounts.source_batches -or
                    $audit.accepted_batches -ne $captureCounts.accepted_batches -or $audit.rejected_batches -ne ($audit.source_batches - $audit.accepted_batches) -or
                    $audit.rejection_total_reconciled -isnot [bool] -or -not $audit.rejection_total_reconciled) { throw 'material rejection audit counts do not reconcile' }
                $acceptedFamilies = $audit.accepted_families
                $acceptedEye = 0
                if ($null -ne $acceptedFamilies -and $null -ne $acceptedFamilies.PSObject.Properties['eye']) {
                    if (-not (Test-ControlFGInteger $acceptedFamilies.eye)) { throw 'invalid eye-family count' }
                    $acceptedEye = $acceptedFamilies.eye
                }
                if ($null -eq $acceptedFamilies -or
                    -not (Test-ControlFGInteger $acceptedFamilies.standardmaterial) -or
                    -not (Test-ControlFGInteger $acceptedFamilies.character) -or
                    -not (Test-ControlFGInteger $acceptedFamilies.cloth) -or
                    -not (Test-ControlFGInteger $acceptedFamilies.foliage) -or
                    -not (Test-ControlFGInteger $acceptedFamilies.hair) -or
                    ($acceptedFamilies.standardmaterial + $acceptedFamilies.character + $acceptedFamilies.cloth + $acceptedFamilies.foliage + $acceptedFamilies.hair + $acceptedEye) -ne $audit.accepted_batches) {
                    throw 'material rejection audit accepted-family counts do not reconcile'
                }
                for ($index=0; $index -lt $ControlFGCapture.Images.Count; $index++) {
                    $image = $ControlFGCapture.Images[$index]
                    $expectedBytes[$image.Name] = if ($image.Format -eq 'bmp24') { $bmpLength } else { $pixels * $image.BytesPerPixel }
                }
                $targetIndexes = @{}
                foreach ($image in $metadata.native_albedo_images) {
                    if (-not (Test-ControlFGInteger $image.target_index) -or $image.target_index -lt 0 -or
                        $image.target_index -ge $ControlFGCapture.NativeTargetLimit -or $targetIndexes.ContainsKey([int]$image.target_index)) { throw 'invalid or duplicated native target index' }
                    $targetIndexes[[int]$image.target_index] = $true
                    $nativeBase = $ControlFGCapture.NativeTargetPrefix + $image.target_index
                    $rawName = $nativeBase + $ControlFGCapture.NativeTargetRawSuffix
                    $colorName = $nativeBase + $ControlFGCapture.NativeTargetPreviewSuffix
                    $alphaName = $nativeBase + $ControlFGCapture.NativeTargetAlphaSuffix
                    if ($image.raw_file -isnot [string] -or $image.raw_file -cne $rawName -or
                        $image.color_preview_file -isnot [string] -or $image.color_preview_file -cne $colorName -or
                        $image.alpha_preview_file -isnot [string] -or $image.alpha_preview_file -cne $alphaName) { throw 'native target filenames do not match the exact allowlist' }
                    if (-not (Test-ControlFGInteger $image.width) -or -not (Test-ControlFGInteger $image.height) -or
                        $image.width -ne $width -or $image.height -ne $height -or
                        -not (Test-ControlFGInteger $image.raw_bytes) -or $image.raw_bytes -ne $pixels*8 -or
                        -not (Test-ControlFGInteger $image.source_frame) -or $image.source_frame -ne $metadata.engine_frame) { throw 'invalid native target dimensions, byte count or source frame' }
                    if ($image.format -isnot [string] -or $image.format -cne 'R16G16B16A16_FLOAT' -or
                        $image.semantic -isnot [string] -or $image.semantic -cne ('NativeAlbedoTarget' + $image.target_index) -or
                        $image.semantics_unverified -isnot [bool] -or -not $image.semantics_unverified -or
                        $image.alpha_interpretation -isnot [string] -or $image.alpha_interpretation -cne 'unspecified') { throw 'native target format or diagnostic semantics are invalid' }
                    $requiredNames += @($rawName,$colorName,$alphaName)
                    $expectedBytes[$rawName]=$pixels*8; $expectedBytes[$colorName]=$bmpLength; $expectedBytes[$alphaName]=$bmpLength
                }
                if (-not (Test-ControlFGInteger $metadata.native_albedo_shader_count) -or
                    $metadata.native_albedo_shaders -isnot [array]) { throw 'native shader manifest fields are missing or have incorrect types' }
                $shaderCount = $metadata.native_albedo_shader_count
                if ($shaderCount -lt 0 -or $shaderCount -gt $ControlFGCapture.ShaderLimit -or
                    $metadata.native_albedo_shaders.Count -ne $shaderCount) { throw 'native shader count does not match the manifest' }
                $shaderIndexes = @{}; $shaderFnv = @{}; [long]$shaderTotal = 0
                if ($metadata.gpu_reflectance_candidates) {
                    $shaderFnv['diffuse-reflectance-candidate.rgba16f']=$metadata.reflectance_diffuse_fnv1a64
                    $shaderFnv['specular-reflectance-candidate.rgba32f']=$metadata.reflectance_specular_fnv1a64
                }
                if ($null -ne $metadata.part1_bytes -and $metadata.part1_bytes -gt 0) {
                    $shaderFnv['material-part1.bin'] = $metadata.part1_fnv1a64
                }
                foreach ($shader in $metadata.native_albedo_shaders) {
                    if (-not (Test-ControlFGInteger $shader.index) -or $shader.index -lt 0 -or $shader.index -ge $ControlFGCapture.ShaderLimit -or
                        $shaderIndexes.ContainsKey([int]$shader.index)) { throw 'invalid or duplicated native shader index' }
                    $shaderIndexes[[int]$shader.index] = $true
                    $shaderName = $ControlFGCapture.ShaderPrefix + ('{0:D3}' -f [int]$shader.index) + $ControlFGCapture.ShaderSuffix
                    if ($shader.file -isnot [string] -or $shader.file -cne $shaderName -or
                        $shader.fnv1a64 -isnot [string] -or $shader.fnv1a64 -cnotmatch '^[0-9a-f]{16}$' -or
                        -not (Test-ControlFGInteger $shader.bytes) -or $shader.bytes -le 0 -or $shader.bytes -gt $ControlFGCapture.MaximumShaderBytes -or
                        -not (Test-ControlFGInteger $shader.source_frame) -or $shader.source_frame -le 0 -or
                        -not (Test-ControlFGInteger $shader.declared_render_target_count) -or $shader.declared_render_target_count -lt 0 -or $shader.declared_render_target_count -gt 8) { throw 'native shader filename, size, hash or metadata is invalid' }
                    $shaderTotal += $shader.bytes
                    if ($shaderTotal -gt $ControlFGCapture.MaximumShaderAggregateBytes) { throw 'native shader aggregate exceeds the collection limit' }
                    $requiredNames += $shaderName; $expectedBytes[$shaderName]=$shader.bytes; $shaderFnv[$shaderName]=$shader.fnv1a64
                }
                $files = @()
                [long]$total = 0
                foreach ($name in $requiredNames) {
                    $file = Get-Item -LiteralPath (Join-Path $capture.FullName $name)
                    if ($file.PSIsContainer -or ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw ('invalid export member ' + $name) }
                    $total += $file.Length
                    if ($total -gt $ControlFGCapture.MaximumCaptureBytes) { throw ('capture exceeds the ' + $ControlFGCapture.MaximumCaptureBytes + '-byte collection limit') }
                    if ($expectedBytes.ContainsKey($name) -and $file.Length -ne $expectedBytes[$name]) { throw ('image or shader length does not match metadata: ' + $name) }
                    $files += $file
                }
                $destination = Join-Path (Join-Path $stage 'rr-guides') $capture.Name
                New-Item -ItemType Directory -Path $destination | Out-Null
                $hashes = @()
                foreach ($file in $files) {
                    $copied = Join-Path $destination $file.Name
                    $inputStream = $null; $outputStream = $null
                    try {
                        # Windows share mode prevents a writer changing a member while it is copied.
                        $inputStream = [IO.File]::Open($file.FullName, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
                        if ($inputStream.Length -ne $file.Length) { throw ('export member changed during collection: ' + $file.Name) }
                        $outputStream = [IO.File]::Open($copied, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
                        $inputStream.CopyTo($outputStream)
                    } finally {
                        if ($null -ne $outputStream) { $outputStream.Dispose() }
                        if ($null -ne $inputStream) { $inputStream.Dispose() }
                    }
                    $copiedFile = Get-Item -LiteralPath $copied
                    if ($copiedFile.Length -ne $file.Length) { throw ('incomplete export member copy: ' + $file.Name) }
                    if ($shaderFnv.ContainsKey($file.Name)) {
                        Initialize-ControlFGShaderHash
                        $copiedFnv = [ControlFG.CollectionShaderHash]::Fnv1a64([IO.File]::ReadAllBytes($copied))
                        if ($copiedFnv -cne $shaderFnv[$file.Name]) { throw ('capture member content hash does not match metadata: ' + $file.Name) }
                    }
                    $hashes += [ordered]@{ Name=$file.Name; Bytes=$copiedFile.Length; SHA256=(Get-FileHash -LiteralPath $copied -Algorithm SHA256).Hash }
                }
                $captureRecords += [ordered]@{ Directory=$capture.Name; Bytes=$total; HasNativeAlbedoOutputs=$metadata.has_native_albedo_outputs; NativeAlbedoOutputCount=$nativeCount; NativeAlbedoShaderCount=$shaderCount; Files=$hashes }
            } catch {
                $notes.Add(('Skipped ' + $capture.Name + ': ' + $_.Exception.Message))
                $partial = Join-Path (Join-Path $stage 'rr-guides') $capture.Name
                if (Test-Path -LiteralPath $partial) { Remove-Item -LiteralPath $partial -Recurse -Force }
            }
        }
        . (Join-Path $PSScriptRoot 'Collect-LiveGuides.ps1')
        $captureRecords += @(Copy-ControlFGLiveGuides $logDir $stage $runPids $notes)
        # The frozen Streamline bridge retains its P9 directory label. Match its PID to selected G11 logs.
        foreach ($runPid in $runPids.Keys) {
            $source = Join-Path $logDir ('Streamline-v1.0.0-RR-Native-P9-' + $runPid)
            if (-not (Test-Path -LiteralPath $source -PathType Container)) { continue }
            $sourceItem = Get-Item -LiteralPath $source
            if ($sourceItem.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
            $destination = Join-Path (Join-Path $stage 'streamline') $sourceItem.Name
            New-Item -ItemType Directory -Path $destination | Out-Null
            # Streamline writes its logs directly into the run directory; do not follow arbitrary subdirectories.
            foreach ($file in @(Get-ChildItem -LiteralPath $source -File |
                Where-Object { $_.Length -le 16MB -and $_.Extension -in @('.log','.txt','.json') -and -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) } |
                Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 30)) {
                Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $destination $file.Name)
            }
        }
        foreach ($relative in @('Build.log','build/build-validation.json','third_party/streamline/sdk-info.json','installation.json')) {
            $source = Join-Path $PSScriptRoot $relative
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                $file = Get-Item -LiteralPath $source
                if ($file.Length -le 16MB -and -not ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                    Copy-Item -LiteralPath $source -Destination (Join-Path (Join-Path $stage 'package') ($relative -replace '[\\/]','__'))
                }
            }
        }
        $settings = Join-Path (Join-Path $env:LOCALAPPDATA 'ControlFG') 'settings.ini'
        if (Test-Path -LiteralPath $settings -PathType Leaf) { Copy-Item -LiteralPath $settings -Destination (Join-Path (Join-Path $stage 'package') 'settings.ini') }
        if (-not $captureRecords.Count) { $notes.Add('No completed, matching G11 image export was available. Probe logs are still included for diagnosis.') }
        [ordered]@{
            Schema=$ControlFGCapture.CollectionSchema; Version=$ControlFGBuild.Version
            CollectedUtc=[DateTime]::UtcNow.ToString('o'); ProbeLogs=@($probeLogs.Name)
            MatchingProcessIds=@($runPids.Keys); CaptureLimit=$ControlFGCapture.MaximumCaptures; PerCaptureLimitBytes=$ControlFGCapture.MaximumCaptureBytes
            Captures=$captureRecords; Notes=@($notes.ToArray())
            StreamlineDirectoryPolicy='Frozen bridge retains P9 folder label; only directories matching selected G11 probe-log PIDs are included.'
        } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $stage 'collection-report.json') -Encoding UTF8
        Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
    } finally {
        if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue }
    }
    Write-Host ('PASS: collected ' + $captureRecords.Count + ' completed G11 capture(s).')
    foreach ($note in $notes) { Write-Host $note }
    Write-Host ('Upload this ZIP: ' + $zip)
    exit 0
} catch { Write-Error $_; exit 1 }
