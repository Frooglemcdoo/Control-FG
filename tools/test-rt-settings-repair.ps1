$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('control-repair-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null
try {
    $script = Join-Path $PSScriptRoot 'rt-settings-repair/Restore-R71-Shader.ps1'
    $path = Join-Path $root 'renderer.ini'
    $encoding = New-Object Text.UTF8Encoding($false)
    foreach ($bom in @($false,$true)) {
        $encoding = New-Object Text.UTF8Encoding($bom)
        $inputText = "{`r`n  `"m_iRTSamplesCount`": 8,`r`n  `"other`": `"keep me`"`r`n}`r`n"
        [IO.File]::WriteAllText($path,$inputText,$encoding)
        $original = [IO.File]::ReadAllBytes($path)
        & powershell.exe -NoProfile -File $script -RendererPath $path
        if ($LASTEXITCODE -ne 0) { throw 'Valid repair failed' }
        if ([IO.File]::ReadAllText($path) -cne $inputText.Replace(': 8,',': 1,')) { throw 'Unrelated text changed' }
        $backup = @(Get-ChildItem $root -Filter '*.bak')
        if ($backup.Count -ne 1 -or [Convert]::ToBase64String([IO.File]::ReadAllBytes($backup[0].FullName)) -cne [Convert]::ToBase64String($original)) { throw 'Backup differs' }
        $fixed = [IO.File]::ReadAllBytes($path)
        if ($bom -and $fixed[0] -ne 239) { throw 'BOM lost' }
        & powershell.exe -NoProfile -File $script -RendererPath $path
        if ($LASTEXITCODE -ne 0 -or @(Get-ChildItem $root -Filter '*.bak').Count -ne 1) { throw 'Not idempotent' }
        Remove-Item -LiteralPath $backup[0].FullName
    }
    foreach ($invalid in @('{"other":1}', '{"m_iRTSamplesCount":2,"m_iRTSamplesCount":4}', '{"m_iRTSamplesCount":"8"}', 'invalid')) {
        [IO.File]::WriteAllText($path,$invalid,$encoding)
        & powershell.exe -NoProfile -File $script -RendererPath $path
        if ($LASTEXITCODE -eq 0 -or [IO.File]::ReadAllText($path) -cne $invalid) { throw 'Invalid file accepted or changed' }
    }
    Write-Host 'PASS repair: one-field edit, exact backup, BOM preservation, repeated runs, rejected invalid inputs'
} finally { Remove-Item -LiteralPath $root -Recurse -Force }

exit 0
