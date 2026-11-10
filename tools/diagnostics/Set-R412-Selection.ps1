param([Parameter(Mandatory=$true)][ValidateSet('Native','Owned')][string]$Mode)
$ErrorActionPreference='Stop'
if(Get-Process -Name 'Control_DX12','Control' -ErrorAction SilentlyContinue){throw 'Close Control before switching the R412 selection.'}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source=Join-Path $root "ControlTOH\Diagnostics\R412-$Mode\dxgi.dll"
$hashFile="$source.sha256"
if(!(Test-Path -LiteralPath $source) -or !(Test-Path -LiteralPath $hashFile)){throw 'R412 selection files are missing. Extract the complete R412 package first.'}
$expected=(Get-Content -LiteralPath $hashFile -Raw).Trim()
if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $expected){throw 'R412 selection DLL failed its hash check. No files were changed.'}
$target=Join-Path $root 'dxgi.dll'
Copy-Item -LiteralPath $source -Destination $target -Force
if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected){throw 'Installed DLL verification failed.'}
Write-Host "R412 $Mode selected. Start Control normally."
