$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP ('PT-Enable-Test-'+[guid]::NewGuid().ToString('N'))
$oldLocal=$env:LOCALAPPDATA
$script:fakeRunning=$false
function Get-Process {param($Name,$ErrorAction) if($script:fakeRunning){[pscustomobject]@{Id=123}}}
try {
    $game=Join-Path $root 'Game With Spaces'
    New-Item -ItemType Directory -Path $game | Out-Null
    $env:LOCALAPPDATA=Join-Path $root 'Local'
    New-Item -ItemType Directory -Path (Join-Path $env:LOCALAPPDATA 'ControlFG') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $game 'dxgi.dll'),'fixture')
    $renderer=Join-Path $game 'renderer.ini'
    $settings=Join-Path $env:LOCALAPPDATA 'ControlFG\settings-mfg-test.ini'
    [IO.File]::WriteAllText($renderer,'{"RT":true,"HDR":false,"Resolution":3840}')
    [IO.File]::WriteAllText($settings,"[FrameGeneration]`r`nMode=0`r`n")
    [IO.File]::WriteAllText((Join-Path $game 'ControlFG-PT-Capture-Off.txt'),'off')
    $before=@((Get-FileHash $renderer).Hash,(Get-FileHash $settings).Hash)
    & (Join-Path $PSScriptRoot 'PT-Enable.ps1') -GameDirectory $game
    if((Get-FileHash $renderer).Hash -ne $before[0] -or (Get-FileHash $settings).Hash -ne $before[1]){throw 'Settings modified'}
    if(Test-Path (Join-Path $game 'ControlFG-PT-Capture-Off.txt')){throw 'Override not removed'}
    if(-not (Test-Path (Join-Path $game 'ControlFG-PT-Enabled.txt'))){throw 'Enable marker missing'}
    $backups=@(Get-ChildItem (Join-Path $game 'ControlFG-Settings-Backups') -Directory)
    if($backups.Count -ne 1){throw 'Backup missing'}
    if((Get-FileHash (Join-Path $backups[0].FullName 'renderer.ini')).Hash -ne $before[0]){throw 'Recovery copy differs'}
    $manifest=Get-Content (Join-Path $backups[0].FullName 'Settings-Hashes.json') -Raw | ConvertFrom-Json
    if($manifest.Count -ne 2 -or @($manifest | Where-Object {$_.BeforeSHA256 -ne $_.AfterSHA256}).Count){throw 'Hash evidence invalid'}
    $script:fakeRunning=$true
    $rejected=$false
    try {& (Join-Path $PSScriptRoot 'PT-Enable.ps1') -GameDirectory $game} catch {$rejected=$true}
    if(-not $rejected){throw 'Running-game mode change was allowed'}
    Write-Host 'PASS: enable changes only markers, preserves exact settings bytes, creates recovery evidence and rejects running game.'
} finally {$env:LOCALAPPDATA=$oldLocal;Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue}
