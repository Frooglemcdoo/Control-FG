param([Parameter(Mandatory=$true)][ValidateSet('NativePrep','OwnedPrep')][string]$Mode)
$ErrorActionPreference='Stop'
if(Get-Process -Name 'Control_DX12','Control' -ErrorAction SilentlyContinue){throw 'Close Control before switching the R413 preparation.'}
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$source=Join-Path $root "ControlTOH\Diagnostics\R413-$Mode\dxgi.dll"
$hashFile="$source.sha256"
if(!(Test-Path -LiteralPath $source) -or !(Test-Path -LiteralPath $hashFile)){throw 'R413 preparation files are missing. Extract the complete R413 package first.'}
$expected=(Get-Content -LiteralPath $hashFile -Raw).Trim()
if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $expected){throw 'R413 preparation DLL failed its hash check. No files were changed.'}
$target=Join-Path $root 'dxgi.dll'
Copy-Item -LiteralPath $source -Destination $target -Force
if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $expected){throw 'Installed DLL verification failed.'}
Write-Host "R413 $Mode selected. Start Control normally."
