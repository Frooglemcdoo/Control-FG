$ErrorActionPreference='Stop'
$root=Join-Path $env:TEMP ('PT-Enable-Test-'+[guid]::NewGuid().ToString('N'))
$oldLocal=$env:LOCALAPPDATA
function Get-Process {param($Name,$ErrorAction)}
try {
    $game=Join-Path $root 'Game With Spaces'
    New-Item -ItemType Directory -Path $game | Out-Null
    $env:LOCALAPPDATA=Join-Path $root 'Local'
    New-Item -ItemType Directory -Path (Join-Path $env:LOCALAPPDATA 'ControlFG') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $game 'dxgi.dll'),'fixture')
    $renderer=Join-Path $game 'renderer.ini'
    $settings=Join-Path $env:LOCALAPPDATA 'ControlFG\settings-mfg-test.ini'
    $legacy=Join-Path $env:LOCALAPPDATA 'ControlFG\settings.ini'
    [IO.File]::WriteAllText($renderer,'{"RT":true,"HDR":false,"Resolution":3840}')
    [IO.File]::WriteAllText($legacy,"[FrameGeneration]`r`nMode=3`r`n[RayReconstruction]`r`nEnabled=1`r`n[PTReflections]`r`nEnabled=0`r`n")
    [IO.File]::WriteAllText((Join-Path $game 'ControlFG-PT-Capture-Off.txt'),'legacy marker')
    $before=@((Get-FileHash $renderer).Hash,(Get-FileHash $legacy).Hash)
    foreach($mode in @('On','Off','On')){
        & (Join-Path $PSScriptRoot 'PT-Enable.ps1') -GameDirectory $game -Mode $mode
        if((Get-FileHash $renderer).Hash -ne $before[0] -or (Get-FileHash $legacy).Hash -ne $before[1]){throw 'Unrelated settings modified'}
        $expected=if($mode -eq 'On'){1}else{0}
        if([ControlFGStartupIni]::GetPrivateProfileInt('PTReflections','Enabled',2,$settings) -ne $expected){throw 'Selection not persisted'}
        if([ControlFGStartupIni]::GetPrivateProfileInt('FrameGeneration','Mode',0,$settings) -ne 3){throw 'FG preference lost'}
        if([ControlFGStartupIni]::GetPrivateProfileInt('RayReconstruction','Enabled',0,$settings) -ne 1){throw 'RR preference lost'}
    }
    $backups=@(Get-ChildItem (Join-Path $game 'ControlFG-Settings-Backups') -Directory)
    if($backups.Count -ne 3){throw 'Backup missing'}
    foreach($backup in $backups){
        if((Get-FileHash (Join-Path $backup.FullName 'renderer.ini')).Hash -ne $before[0]){throw 'Recovery copy differs'}
        $manifest=Get-Content (Join-Path $backup.FullName 'Settings-Hashes.json') -Raw | ConvertFrom-Json
        if(@($manifest | Where-Object {$_.Path -ne $settings -and $_.BeforeSHA256 -ne $_.AfterSHA256}).Count){throw 'Hash evidence invalid'}
    }
    function Get-Process {param($Name,$ErrorAction) [pscustomobject]@{Id=123}}
    $beforeRunning=(Get-FileHash $settings).Hash
    $rejected=$false
    try {& (Join-Path $PSScriptRoot 'PT-Enable.ps1') -GameDirectory $game -Mode Off} catch {$rejected=$true}
    if(-not $rejected -or (Get-FileHash $settings).Hash -ne $beforeRunning){throw 'Running-game mode change was allowed'}
    Write-Host 'PASS: on/off share persisted selection, preserve unrelated settings and recovery evidence, migrate fallback and reject running game.'
} finally {$env:LOCALAPPDATA=$oldLocal;Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue}
