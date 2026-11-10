$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$log=Get-ChildItem -LiteralPath $root -Filter 'probe-*.log' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(!$log){throw 'No probe log found. Start Control with R410 first.'}
if(!(Select-String -LiteralPath $log.FullName -Pattern 'RT_ASSEMBLY_R410' -Quiet)){Write-Warning 'Latest log has no R410 marker; including it to diagnose installation.'}
$out=Join-Path $PSScriptRoot 'RT-Assembly-R410.zip'
Compress-Archive -LiteralPath $log.FullName -DestinationPath $out -Force
Write-Host "Send $out"
