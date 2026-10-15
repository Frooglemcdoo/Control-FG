param([string]$GameDirectory=$PSScriptRoot,[ValidateSet("On","Off")][string]$Mode="On")
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
if(-not $env:LOCALAPPDATA){throw 'LOCALAPPDATA is unavailable; no mode changed.'}
$settingsDir=Join-Path $env:LOCALAPPDATA 'ControlFG'
New-Item -ItemType Directory -Path $settingsDir -Force | Out-Null
$settings=Join-Path $settingsDir 'settings-mfg-test.ini'
$legacy=Join-Path $settingsDir 'settings.ini'
if(-not (Test-Path -LiteralPath $settings) -and (Test-Path -LiteralPath $legacy -PathType Leaf)){Copy-Item -LiteralPath $legacy -Destination $settings}
if(-not ('ControlFGStartupIni' -as [type])){
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ControlFGStartupIni {
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool WritePrivateProfileString(string section,string key,string value,string path);
    [DllImport("kernel32.dll",CharSet=CharSet.Unicode)]
    public static extern uint GetPrivateProfileInt(string section,string key,int fallback,string path);
}
'@
}
$value=if($Mode -eq 'On'){'1'}else{'0'}
if(-not [ControlFGStartupIni]::WritePrivateProfileString('PTReflections','Enabled',$value,$settings)){throw 'Could not save PT selection.'}
if(-not [ControlFGStartupIni]::WritePrivateProfileString($null,$null,$null,$settings)){throw 'Could not flush PT selection.'}
if([ControlFGStartupIni]::GetPrivateProfileInt('PTReflections','Enabled',2,$settings) -ne [uint32]$value){throw 'PT selection verification failed.'}
foreach($entry in $manifest){$entry.AfterSHA256=(Get-FileHash -LiteralPath $entry.Path -Algorithm SHA256).Hash}
ConvertTo-Json -InputObject @($manifest) -Depth 3 | Set-Content -LiteralPath (Join-Path $backup 'Settings-Hashes.json') -Encoding UTF8
if(@($manifest | Where-Object {$_.Path -ne $settings -and $_.BeforeSHA256 -ne $_.AfterSHA256}).Count){throw "Unrelated settings changed while selecting PT. Backup retained at $backup. No settings have been restored automatically."}
Write-Host "PT $Mode selected for the next launch. Native game settings are unchanged."
Write-Host "Recovery copies: $backup"
Write-Host 'The overlay now saves this same selection. Updates preserve it; this command is optional.'
