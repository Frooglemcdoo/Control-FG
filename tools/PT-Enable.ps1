param([string]$GameDirectory=$PSScriptRoot)
$ErrorActionPreference='Stop'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if(-not (Test-Path -LiteralPath (Join-Path $game 'dxgi.dll') -PathType Leaf)){throw 'Extract the package beside dxgi.dll first.'}
if(Get-Process -Name 'Control','Control_DX12','Control_DX11' -ErrorAction SilentlyContinue){throw 'Close Control before changing the startup mode.'}
$root=Join-Path $game 'ControlFG-Settings-Backups'
$backup=Join-Path $root ((Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $backup | Out-Null
$files=@((Join-Path $game 'renderer.ini'))
if($env:LOCALAPPDATA){$files+=Join-Path $env:LOCALAPPDATA 'ControlFG\settings-mfg-test.ini';$files+=Join-Path $env:LOCALAPPDATA 'ControlFG\settings.ini'}
$manifest=@()
foreach($file in $files){
    if(Test-Path -LiteralPath $file -PathType Leaf){
        $hash=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
        $name=Split-Path -Leaf $file
        Copy-Item -LiteralPath $file -Destination (Join-Path $backup $name)
        if((Get-FileHash -LiteralPath (Join-Path $backup $name) -Algorithm SHA256).Hash -ne $hash){throw "Settings changed during backup: $file"}
        $manifest+=[pscustomobject]@{Path=$file;File=$name;BeforeSHA256=$hash;AfterSHA256=$null}
    }
}
$off=Join-Path $game 'ControlFG-PT-Capture-Off.txt'
if(Test-Path -LiteralPath $off){Remove-Item -LiteralPath $off}
[IO.File]::WriteAllText((Join-Path $game 'ControlFG-PT-Enabled.txt'),'PT enabled on next game launch')
foreach($entry in $manifest){$entry.AfterSHA256=(Get-FileHash -LiteralPath $entry.Path -Algorithm SHA256).Hash}
ConvertTo-Json -InputObject @($manifest) -Depth 3 | Set-Content -LiteralPath (Join-Path $backup 'Settings-Hashes.json') -Encoding UTF8
if(@($manifest | Where-Object {$_.BeforeSHA256 -ne $_.AfterSHA256}).Count){throw "Settings changed while enabling PT. Backup retained at $backup. No settings have been restored automatically."}
Write-Host 'PT enabled. Settings hashes are unchanged. Start Control normally.'
Write-Host "Recovery copies: $backup"
Write-Host 'PT-off pauses descriptor mirror copies; geometry capture remains active.'
