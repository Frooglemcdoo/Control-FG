$ErrorActionPreference='Stop'
$root=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$logs=@(Get-ChildItem -LiteralPath $root -Filter 'probe-*.log' -File | Sort-Object LastWriteTime -Descending)
if(!$logs.Count){throw 'No probe log found. Start Control with R413 first.'}
$selected=@();$counts=@{ 'native'=0; 'owned'=0 }
foreach($log in $logs){
 $match=Select-String -LiteralPath $log.FullName -Pattern 'RT_MODE_R411 preparation_comparison=R413 preparation=(native|owned)' | Select-Object -First 1
 if(!$match){continue}
 $mode=$match.Matches[0].Groups[1].Value
 if($counts[$mode] -lt 2){$selected+=$log.FullName;$counts[$mode]++}
 if($counts['native'] -eq 2 -and $counts['owned'] -eq 2){break}
}
if(!$selected.Count){Write-Warning 'No R413 startup marker found. Including the latest log to diagnose installation.';$selected=@($logs[0].FullName)}
$out=Join-Path $PSScriptRoot 'RT-Preparation-R413.zip'
Compress-Archive -LiteralPath $selected -DestinationPath $out -Force
Write-Host "Collected native runs: $($counts['native']); owned runs: $($counts['owned']). Send $out"
