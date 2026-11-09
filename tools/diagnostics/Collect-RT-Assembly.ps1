$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$log=Get-ChildItem -LiteralPath $root -Filter 'probe-*.log' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(!$log){throw 'No probe log found. Start Control with R403 first.'}
if(!(Select-String -LiteralPath $log.FullName -Pattern 'RT_ASSEMBLY_R403' -Quiet)){Write-Warning 'Latest log has no R403 marker; including it to diagnose installation.'}
$out=Join-Path $PSScriptRoot 'RT-Assembly-R403.zip'
Compress-Archive -LiteralPath $log.FullName -DestinationPath $out -Force
Write-Host "Send $out"
