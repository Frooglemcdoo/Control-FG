$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$log=Get-ChildItem -LiteralPath $root -Filter 'probe-*.log' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(!$log){throw 'No probe log found. Start Control with R393 first.'}
$files=@($log.FullName)
if($log.Name -match '-([0-9]+)\.log$') {
 $gameProcess=$Matches[1]
 $capture=Get-ChildItem -LiteralPath $root -Filter "RT-Instances-R393-$gameProcess-*.bin" -File | Where-Object {$_.LastWriteTimeUtc -ge $log.CreationTimeUtc} | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if($capture){$files+=$capture.FullName}else{Write-Warning 'No completed capture for this run. The ZIP will contain its log.'}
}
$out=Join-Path $PSScriptRoot 'RT-Instances-R393.zip'
Compress-Archive -LiteralPath $files -DestinationPath $out -Force
Write-Host "Send $out"
