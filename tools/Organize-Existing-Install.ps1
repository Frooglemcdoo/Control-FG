param([string]$GameDirectory='')
$ErrorActionPreference='Stop'
if ([string]::IsNullOrWhiteSpace($GameDirectory)) { $GameDirectory=Join-Path $PSScriptRoot '..\..' }
$root=(Resolve-Path -LiteralPath $GameDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $root 'Control_DX12.exe') -PathType Leaf)) { throw 'Install the complete package beside Control_DX12.exe first.' }
if (Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue) { throw 'Close Control before organizing the installation.' }
$mod=Join-Path $root 'ControlTOH'
foreach ($required in @('Tools\ControlFG-Restart.exe','Shaders\rt_spot_full.dxil','Shaders\pt_glass_raygen.dxil')) {
    if (-not (Test-Path -LiteralPath (Join-Path $mod $required) -PathType Leaf)) { throw 'The new package is incomplete. Copy the full package first.' }
}
# Only names present in the new mod package are recognized. Never move root DLLs,
# generic readme files, game assets, Streamline, or unknown files/directories.
$relative=@()
foreach ($folder in @('Shaders','Tools','Diagnostics')) {
    $relative+=@(Get-ChildItem -LiteralPath (Join-Path $mod $folder) -File | Where-Object {
        $_.Name -notlike 'Organize-Existing-Install.*' -and $_.Name -notlike 'Start-R257-*'
    } | ForEach-Object { $_.Name })
}
$relative+=@('Start-R255-Storage-Diagnostics.cmd','Start-R256-Storage-Diagnostics.cmd')
$validators=Join-Path $mod 'Diagnostics\DirectStorage'
if (Test-Path -LiteralPath $validators -PathType Container) {
    $relative+=@(Get-ChildItem -LiteralPath $validators -File | ForEach-Object { 'DirectStorage\'+$_.Name })
}
$backup=Join-Path $mod ('LegacyBackup\'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
$count=0
foreach ($name in ($relative | Sort-Object -Unique)) {
    $source=Join-Path $root $name
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { continue }
    $item=Get-Item -LiteralPath $source
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
        ((Get-Item -LiteralPath $item.DirectoryName).Attributes -band [IO.FileAttributes]::ReparsePoint)) { continue }
    $destination=Join-Path $backup $name
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination)) | Out-Null
    Move-Item -LiteralPath $source -Destination $destination
    $count++
    Write-Output "Archived $name"
}
if ($count) { Write-Output "Archived $count legacy files to $backup" }
else { Write-Output 'No recognized legacy files remain beside the executable.' }
