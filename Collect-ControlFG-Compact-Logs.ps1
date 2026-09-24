#requires -Version 5.1
[CmdletBinding()]
param([string]$LogDirectory='', [string]$ProjectDirectory='', [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$script:remaining=10MB
$script:records=New-Object 'System.Collections.Generic.List[object]'
$script:notes=New-Object 'System.Collections.Generic.List[string]'
function Resolve-CollectorDirectory([string]$Path,[string]$Default,[string]$Label) {
    if ([string]::IsNullOrWhiteSpace($Path)) { $Path=$Default }
    if ([string]::IsNullOrWhiteSpace($Path)) { throw ($Label+' directory could not be determined.') }
    $provider=$null; $drive=$null
    $resolved=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path,[ref]$provider,[ref]$drive)
    if ($provider.Name -ne 'FileSystem') { throw ($Label+' directory must be a filesystem path.') }
    return $resolved
}
function Test-RegularFile($Item) {
    return ($null -ne $Item -and -not $Item.PSIsContainer -and -not ($Item.Attributes -band [IO.FileAttributes]::ReparsePoint))
}
function Read-LogHeader($Path) {
    $stream=$null
    try {
        $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        $buffer=New-Object byte[] 4096
        $count=$stream.Read($buffer,0,$buffer.Length)
        return [Text.Encoding]::UTF8.GetString($buffer,0,$count)
    } finally { if ($null -ne $stream) { $stream.Dispose() } }
}
function Copy-Range($InputStream,$OutputStream,[long]$Offset,[long]$Count) {
    [void]$InputStream.Seek($Offset,[IO.SeekOrigin]::Begin)
    $buffer=New-Object byte[] 65536
    while ($Count -gt 0) {
        $read=$InputStream.Read($buffer,0,[int][Math]::Min($Count,$buffer.Length))
        if ($read -le 0) { throw 'Source changed or ended during collection.' }
        $OutputStream.Write($buffer,0,$read)
        $Count-=$read
    }
}
function Copy-CompactFile($File,[string]$Relative,[long]$Limit,[bool]$MayTrim=$false) {
    if (-not (Test-RegularFile $File)) { return }
    $limitNow=[long][Math]::Min($Limit,$script:remaining)
    if ($limitNow -lt 1024) { $script:notes.Add(('Size budget reached: '+$Relative)); return }
    $inputStream=$null; $outputStream=$null
    $destination=Join-Path $script:stage $Relative
    try {
        $inputStream=[IO.File]::Open($File.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        [long]$length=$inputStream.Length
        if ($length -gt $limitNow -and -not $MayTrim) {
            $script:notes.Add(('Omitted oversized metadata: '+$Relative)); return
        }
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
        $outputStream=[IO.File]::Open($destination,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
        [long]$head=$length; [long]$tail=0; [long]$omitted=0
        if ($length -gt $limitNow) {
            $marker=[Text.Encoding]::UTF8.GetBytes("`r`n[COMPACT COLLECTOR: middle omitted; startup and latest log bytes follow. See collection-report.json.]`r`n")
            $head=[long][Math]::Min(16384,$limitNow-$marker.Length)
            $tail=$limitNow-$head-$marker.Length
            Copy-Range $inputStream $outputStream 0 $head
            $outputStream.Write($marker,0,$marker.Length)
            Copy-Range $inputStream $outputStream ($length-$tail) $tail
            $omitted=$length-$head-$tail
        } else { Copy-Range $inputStream $outputStream 0 $length }
        [long]$written=$outputStream.Length
        $outputStream.Dispose(); $outputStream=$null
        $script:remaining-=$written
        $script:records.Add([ordered]@{File=$Relative;SourceBytes=$length;IncludedBytes=$written;OmittedBytes=$omitted;HeadBytes=$head;TailBytes=$tail;SHA256=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash})
    } catch {
        if ($null -ne $outputStream) { $outputStream.Dispose(); $outputStream=$null }
        if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Force }
        $script:notes.Add(('Could not copy '+$Relative+': '+$_.Exception.Message))
    } finally {
        if ($null -ne $inputStream) { $inputStream.Dispose() }
        if ($null -ne $outputStream) { $outputStream.Dispose() }
    }
}
try {
    # Resolve defaults in the script body, before any .NET filesystem calls.
    $collectorDirectory=$PSScriptRoot
    if ([string]::IsNullOrWhiteSpace($collectorDirectory)) {
        $collectorDirectory=Split-Path -Parent $MyInvocation.MyCommand.Path
    }
    $OutputDirectory=Resolve-CollectorDirectory $OutputDirectory $collectorDirectory 'Output'
    $ProjectDirectory=Resolve-CollectorDirectory $ProjectDirectory $collectorDirectory 'Project'
    if ([string]::IsNullOrWhiteSpace($LogDirectory)) {
        if ([string]::IsNullOrWhiteSpace($env:LOCALAPPDATA)) { throw 'LOCALAPPDATA is unavailable; specify -LogDirectory.' }
        $LogDirectory=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
    }
    $LogDirectory=Resolve-CollectorDirectory $LogDirectory '' 'Log'
    if (-not (Test-Path -LiteralPath $LogDirectory -PathType Container)) { throw 'No Control FG logs found. Run the installed mod first.' }
    if ((Get-Item -LiteralPath $LogDirectory).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Log directory must be a regular directory.' }
    $candidates=@(Get-ChildItem -LiteralPath $LogDirectory -File -Filter 'probe-*.log' | Where-Object { Test-RegularFile $_ } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 30)
    $logs=@(); $runPids=@{}; $version=''
    foreach ($file in $candidates) {
        if ($logs.Count -ge 3) { break }
        try { $header=Read-LogHeader $file.FullName } catch { continue }
        if ($header -notmatch ' PROBE v([^\s]+) ') { continue }
        $foundVersion=$Matches[1]
        if (-not $version) { $version=$foundVersion }
        if ($foundVersion -cne $version -or $file.Name -notmatch '^probe-\d{8}-\d{6}-\d{3}-(\d+)\.log$') { continue }
        $runPids[$Matches[1]]=$true; $logs+= $file
    }
    if (-not $logs.Count) { throw 'No recognizable Control FG probe log was found.' }
    $suffix=[guid]::NewGuid().ToString('N').Substring(0,8)
    $script:stage=Join-Path ([IO.Path]::GetTempPath()) ('ControlFG-Compact-'+$suffix)
    [void][IO.Directory]::CreateDirectory($OutputDirectory)
    $zip=Join-Path $OutputDirectory ('Control-FG-Compact-Logs-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+$suffix+'.zip')
    try {
        [void][IO.Directory]::CreateDirectory($script:stage)
        foreach ($file in $logs) { Copy-CompactFile $file ('probe/'+$file.Name) 3MB $true }
        $summaryScript=Join-Path $PSScriptRoot 'Summarize-ControlFG-Performance.ps1'
        if (Test-Path -LiteralPath $summaryScript -PathType Leaf) {
            . $summaryScript
            foreach ($file in $logs) {
                $copied=Join-Path $script:stage ('probe/'+$file.Name)
                if (-not (Test-Path -LiteralPath $copied)) { continue }
                $summary=Join-Path ([IO.Path]::GetTempPath()) ('ControlFG-Performance-'+$suffix+'.json')
                try {
                    if (Write-ControlFGPerformanceSummary $copied $summary) {
                        Copy-CompactFile (Get-Item -LiteralPath $summary) ('performance/'+$file.BaseName+'.json') 128KB
                    }
                } catch { $script:notes.Add(('Performance summary unavailable: '+$_.Exception.Message)) }
                finally { if (Test-Path -LiteralPath $summary) { Remove-Item -LiteralPath $summary -Force } }
            }
        }
        # Always include the experimental sidecar log, even if other logs are newer.
        foreach ($runPid in $runPids.Keys) {
            $testLog=Get-Item -LiteralPath (Join-Path $LogDirectory ('Streamline-v1.0.0-RR-Native-P9-'+$runPid+'/RTX40MFG.log')) -ErrorAction SilentlyContinue
            if (Test-RegularFile $testLog) { Copy-CompactFile $testLog ('mfg/'+$runPid+'/RTX40MFG.log') 256KB $true }
        }
        # Release support collection intentionally excludes retired RR capture metadata.
        foreach ($runPid in $runPids.Keys) {
            $folder=Join-Path $LogDirectory ('Streamline-v1.0.0-RR-Native-P9-'+$runPid)
            if (-not (Test-Path -LiteralPath $folder -PathType Container)) { continue }
            if ((Get-Item -LiteralPath $folder).Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
            $files=@(Get-ChildItem -LiteralPath $folder -File | Where-Object { (Test-RegularFile $_) -and $_.Extension -in @('.log','.txt','.json') } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 6)
            foreach ($file in $files) { Copy-CompactFile $file ('streamline/'+$runPid+'/'+$file.Name) 256KB ($file.Extension -ne '.json') }
        }
        foreach ($relative in @('build/build-validation.json','installation.json')) {
            $file=Get-Item -LiteralPath (Join-Path $ProjectDirectory $relative) -ErrorAction SilentlyContinue
            if (Test-RegularFile $file) { Copy-CompactFile $file ('package/'+($relative -replace '[\\/]','__')) 256KB ($file.Extension -eq '.log') }
        }
        $receiptFile=Join-Path $ProjectDirectory 'installation.json'
        if (Test-Path -LiteralPath $receiptFile -PathType Leaf) {
            try {
                $receipt=Get-Content -LiteralPath $receiptFile -Raw | ConvertFrom-Json
                if ($receipt.GamePath -and (Test-Path -LiteralPath $receipt.GamePath -PathType Container)) {
                    foreach ($name in @('ReShade.log','d3d12.log')) {
                        $file=Get-Item -LiteralPath (Join-Path $receipt.GamePath $name) -ErrorAction SilentlyContinue
                        if (Test-RegularFile $file) { Copy-CompactFile $file ('reshade/'+$name) 512KB $true }
                    }
                }
            } catch { $script:notes.Add(('ReShade log collection unavailable: '+$_.Exception.Message)) }
        }
        if ($env:LOCALAPPDATA) {
            $settings=Get-Item -LiteralPath (Join-Path $env:LOCALAPPDATA 'ControlFG/settings-mfg-test.ini') -ErrorAction SilentlyContinue
            if (Test-RegularFile $settings) { Copy-CompactFile $settings 'package/settings.ini' 64KB }
        }
        if (-not @($script:records | Where-Object { $_.File -like 'probe/*' }).Count) { throw 'No probe log could be copied.' }
        $script:notes.Add('Release support bundle: probe logs, performance summary, Streamline logs, settings, installation/build validation, and ReShade logs when present. the build transcript, SDK staging metadata, retired RR capture metadata, raw images, shader bytecode, videos and DLLs are excluded.')
        [ordered]@{Schema='ControlFG.CompactLogs.v2';Version=$version;CollectedUtc=[DateTime]::UtcNow.ToString('o');ProbeLogs=@($logs.Name);MatchingProcessIds=@($runPids.Keys);DataLimitBytes=10MB;IncludedDataBytes=(10MB-$script:remaining);RawImagesIncluded=$false;Files=@($script:records.ToArray());Notes=@($script:notes.ToArray())} |
            ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $script:stage 'collection-report.json') -Encoding UTF8
        Compress-Archive -Path (Join-Path $script:stage '*') -DestinationPath $zip -CompressionLevel Optimal
        if ((Get-Item -LiteralPath $zip).Length -gt 12MB) { Remove-Item -LiteralPath $zip -Force; throw 'Compact ZIP exceeded the 12 MiB sharing limit.' }
    } finally { if (Test-Path -LiteralPath $script:stage) { Remove-Item -LiteralPath $script:stage -Recurse -Force } }
    Write-Host ('Collected '+$version+' logs: '+[Math]::Round((Get-Item -LiteralPath $zip).Length/1MB,2)+' MiB')
    Write-Host ('Upload this ZIP: '+$zip)
    exit 0
} catch { Write-Error $_; exit 1 }
