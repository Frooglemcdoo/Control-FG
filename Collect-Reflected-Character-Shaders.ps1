param([string]$SourceRoot = $PSScriptRoot)
$ErrorActionPreference = 'Stop'
try {
    # Read-only collection. No DLL installation, settings changes or game hooks.
    function Find-ShaderFolder([string]$Root) {
        foreach ($relative in @('', 'data\shaders\build\pc_dxil', 'shaders\data\shaders\build\pc_dxil', 'shaders\build\pc_dxil', 'build\pc_dxil', 'pc_dxil')) {
            $candidate = if ($relative) { Join-Path $Root $relative } else { $Root }
            if (Test-Path -LiteralPath (Join-Path $candidate 'rt_transparent.obj') -PathType Leaf) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
        return $null
    }
    $folder = Find-ShaderFolder $SourceRoot
    if (-not $folder) {
        Write-Host 'Could not find loose DX12 glass shaders beside this collector.'
        Write-Host 'If you have extracted game shaders, enter the pc_dxil folder (or its parent).'
        Write-Host 'If you do not have that folder, press Enter. Nothing will be changed.'
        $inputPath = (Read-Host 'Shader folder').Trim().Trim('"')
        if ($inputPath) { $folder = Find-ShaderFolder $inputPath }
    }
    if (-not $folder) { throw 'rt_transparent.obj was not found. No shader archive was created. Report this message; do not change your installation.' }
    $required = @('rt_transparent.obj', 'rt_transparent_deferred.obj', 'standardmaterial.obj')
    foreach ($name in $required) {
        if (-not (Test-Path -LiteralPath (Join-Path $folder $name) -PathType Leaf)) {
            throw "Missing $name in the same shader folder. No archive was created."
        }
    }
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $output = Join-Path $PSScriptRoot ('Reflected-Character-Shaders-' + $stamp + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $output | Out-Null
    $manifest = @()
    foreach ($name in ($required + @('rt_skinning.obj', 'rt_reflection.obj', 'rt_deferred_reflection.obj', 'ssssblur.obj'))) {
        $source = Join-Path $folder $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            $destination = Join-Path $output $name
            Copy-Item -LiteralPath $source -Destination $destination
            $originalHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
            $copiedHash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
            if ($originalHash -ne $copiedHash) { throw "Copy verification failed: $name" }
            $manifest += [pscustomobject]@{ name=$name; bytes=(Get-Item -LiteralPath $destination).Length; sha256=$copiedHash }
        }
    }
    $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding UTF8
    Compress-Archive -Path (Join-Path $output '*') -DestinationPath ($output + '.zip')
    Write-Host ''
    Write-Host ('Ready to attach: ' + $output + '.zip') -ForegroundColor Green
    Write-Host 'R38 and all game settings are unchanged. The game does not need to run.'
} catch {
    Write-Host ('Collection failed: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
}
