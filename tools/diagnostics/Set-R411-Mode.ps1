param([Parameter(Mandatory=$true)][ValidateSet('Clean','Diagnostics')][string]$Mode)
$ErrorActionPreference='Stop'
if(Get-Process -Name 'Control_DX12','Control' -ErrorAction SilentlyContinue){throw 'Close Control before switching the R411 mode.'}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source=Join-Path $root "ControlTOH\Diagnostics\R411-$Mode\dxgi.dll"
$hashFile="$source.sha256"
if(!(Test-Path -LiteralPath $source) -or !(Test-Path -LiteralPath $hashFile)){throw 'R411 mode files are missing. Extract the complete R411 package first.'}
$expected=(Get-Content -LiteralPath $hashFile -Raw).Trim()
if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $expected){throw 'R411 mode DLL failed its hash check. No files were changed.'}
$target=Join-Path $root 'dxgi.dll'
Copy-Item -LiteralPath $source -Destination $target -Force
if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected){throw 'Installed DLL verification failed.'}
Write-Host "R411 $Mode selected. Start Control normally."
