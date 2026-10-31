#requires -Version 5.1
[CmdletBinding()]
param([string]$LogDirectory=(Join-Path $env:LOCALAPPDATA 'ControlFGProbe'),[string]$OutputDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression
$source=Get-ChildItem -LiteralPath $LogDirectory -Filter 'probe-*.log' -File | Where-Object { -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $source) { throw 'No probe log found.' }
[void][IO.Directory]::CreateDirectory($OutputDirectory)
$output=Join-Path $OutputDirectory ('Control-FG-Texture-Trace-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8)+'.zip')
$inputStream=$null; $file=$null; $archive=$null; $entryStream=$null
try {
    $inputStream=[IO.File]::Open($source.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    # Snapshot length: concurrent appends are excluded, never silently trim the middle.
    [long]$length=$inputStream.Length; [long]$remaining=$length
    $file=[IO.File]::Open($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    $archive=New-Object IO.Compression.ZipArchive($file,[IO.Compression.ZipArchiveMode]::Create,$true)
    $entry=$archive.CreateEntry($source.Name,[IO.Compression.CompressionLevel]::Optimal)
    $entryStream=$entry.Open(); $buffer=New-Object byte[] 65536
    while ($remaining -gt 0) {
        $read=$inputStream.Read($buffer,0,[int][Math]::Min($remaining,$buffer.Length))
        if ($read -le 0) { throw 'Source ended before the snapshot was copied.' }
        $entryStream.Write($buffer,0,$read); $remaining-=$read
    }
    $entryStream.Dispose();$entryStream=$null
    $archive.Dispose();$archive=$null
    $file.Dispose();$file=$null
    Write-Host ('Saved complete latest-session log ('+$length+' bytes): '+$output)
    Write-Output $output
} catch {
    if ($null -ne $entryStream) {$entryStream.Dispose();$entryStream=$null}
    if ($null -ne $archive) {$archive.Dispose();$archive=$null}
    if ($null -ne $file) {$file.Dispose();$file=$null}
    if (Test-Path -LiteralPath $output) {Remove-Item -LiteralPath $output -Force}
    throw
} finally {
    if ($null -ne $inputStream) {$inputStream.Dispose()}
}
