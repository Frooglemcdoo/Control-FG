$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$logs=@(Get-ChildItem -LiteralPath $root -Filter 'probe-*.log' -File | Sort-Object LastWriteTime -Descending)
if(!$logs.Count){throw 'No probe log found. Start Control with R411 first.'}
$selected=@();$counts=@{ '0'=0; '1'=0 }
foreach($log in $logs){
 $match=Select-String -LiteralPath $log.FullName -Pattern 'RT_MODE_R411 diagnostics=([01])' | Select-Object -First 1
 if(!$match){continue}
 $mode=$match.Matches[0].Groups[1].Value
 if($counts[$mode] -lt 2){$selected+=$log.FullName;$counts[$mode]++}
 if($counts['0'] -eq 2 -and $counts['1'] -eq 2){break}
}
if(!$selected.Count){Write-Warning 'No R411 startup marker found. Including the latest log to diagnose installation.';$selected=@($logs[0].FullName)}
$out=Join-Path $PSScriptRoot 'RT-Assembly-R411.zip'
Compress-Archive -LiteralPath $selected -DestinationPath $out -Force
Write-Host "Collected clean runs: $($counts['0']); diagnostic runs: $($counts['1']). Send $out"
