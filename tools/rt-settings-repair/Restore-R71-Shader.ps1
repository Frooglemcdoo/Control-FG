param([string]$RendererPath = (Join-Path $PSScriptRoot 'renderer.ini'))
$ErrorActionPreference = 'Stop'
try {
    if (Get-Process -Name Control_DX12,Control_DX11 -ErrorAction SilentlyContinue) {
        throw 'Close Control before restoring its saved settings.'
    }
    $path = (Resolve-Path -LiteralPath $RendererPath).Path
    $bytes = [IO.File]::ReadAllBytes($path)
    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191
    $encoding = New-Object System.Text.UTF8Encoding($hasBom, $true)
    $offset = 0
    if ($hasBom) { $offset = 3 }
    $text = $encoding.GetString($bytes, $offset, $bytes.Length - $offset)
    $settings = ConvertFrom-Json -InputObject $text
    $property = $settings.PSObject.Properties['m_iRTSamplesCount']
    if ($null -eq $property) { throw 'm_iRTSamplesCount is absent. No settings were changed; send renderer.ini for inspection.' }
    $pattern = '("m_iRTSamplesCount"\s*:\s*)(-?\d+)(?=\s*[,}])'
    $matches = [regex]::Matches($text, $pattern)
    if ($matches.Count -ne 1 -or $property.Value -isnot [ValueType] -or $property.Value -ne [long]$matches[0].Groups[2].Value) {
        throw 'The sample-count entry is ambiguous or unsupported. No settings were changed.'
    }
    $old = $property.Value
    Write-Host "File: $path"
    Write-Host "m_iRTSamplesCount: $old -> 1"
    if ($old -eq 1) {
        Write-Host 'Already 1. No file changed. If R71 is still blocked, send this file and the new R71 log.'
        exit 0
    }
    $updated = [regex]::Replace($text, $pattern, '${1}1')
    $verified = ConvertFrom-Json -InputObject $updated
    if ($verified.m_iRTSamplesCount -ne 1) { throw 'Updated settings failed validation.' }
    $backup = $path + '.before-R71-' + [Guid]::NewGuid().ToString('N') + '.bak'
    $temporary = $path + '.' + [Guid]::NewGuid().ToString('N') + '.tmp'
    try {
        [IO.File]::WriteAllText($temporary, $updated, $encoding)
        # Atomic replacement retains the original as a backup; no partial write.
        [IO.File]::Replace($temporary, $path, $backup)
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
    }
    Write-Host "Backup: $backup"
    Write-Host 'Restored the single-sample native RT setting. Launch Control with R71 and enable RT SPOTLIGHT.'
    Write-Host 'This also restores native reflection/diffuse sampling to 1. Other settings retain their original text.'
    exit 0
} catch {
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
}
