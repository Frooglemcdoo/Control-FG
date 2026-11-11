$ErrorActionPreference='Stop'
$temp=Join-Path ([IO.Path]::GetTempPath()) ('Control scene check '+[guid]::NewGuid().ToString('N'))
$script=Join-Path $PSScriptRoot 'diagnostics/GI-Scene-Check.ps1'
try {
 $logs=Join-Path $temp 'logs';New-Item -ItemType Directory -Path $logs | Out-Null
 $rows=@('{"seq":1,"kind":25,"data":[427,1,0,4]}')
 foreach($point in @(1,2,3,8)){$rows+=('{"kind":22,"data":[1,100,'+$point+',2,2,0,0,0,63,0,1,1]}')}
 $rows+='{"status":true,"stopped":1,"written":5,"reserved":5,"dropped":0}'
 $file=Join-Path $logs 'GI-Phase-R425-1234.jsonl';$rows|Set-Content $file -Encoding ASCII
 foreach($case in @('complete','missing','stale','fault','overflow','malformed')){
  $out=Join-Path $temp $case;New-Item -ItemType Directory -Path $out|Out-Null
  $gameId=if($case -eq 'missing'){5678}else{1234}
  $start=if($case -eq 'stale'){[DateTime]::UtcNow.AddMinutes(1)}else{[DateTime]::UtcNow.AddMinutes(-1)}
  if($case -eq 'fault'){$rows=$rows -replace ',63,0,1,1',',63,5,1,1';$rows|Set-Content $file -Encoding ASCII}
  if($case -eq 'overflow'){$rows=$rows -replace '"dropped":0','"dropped":1';$rows|Set-Content $file -Encoding ASCII}
  if($case -eq 'malformed'){'{'|Set-Content $file -Encoding ASCII}
  & $script -CollectOnly -GameProcessId $gameId -RunStartedUtc $start.ToString('o') -LogDirectory $logs -OutputDirectory $out
  $folder=Get-ChildItem $out -Directory|Select-Object -First 1
  $r=[IO.File]::ReadAllText((Join-Path $folder.FullName 'result.json'))|ConvertFrom-Json
  if($r.SceneLeaseAuthorized -or $r.DeferredRenderingEnabled){throw 'False scene admission'}
  if($case -eq 'complete' -and (!$r.HooksInstalled -or $r.Warnings.Count -or $r.AfterSubmitSamples -ne 1)){throw 'Valid collection misclassified'}
  if($case -ne 'complete' -and !$r.Warnings.Count){throw 'Missing warning'}
  if(($case -eq 'missing' -or $case -eq 'stale') -and $r.LogFound){throw 'Wrong session accepted'}
  if(@(Get-ChildItem $out -Filter '*.zip').Count -ne 1){throw 'ZIP missing'}
 }
 Write-Host 'PASS scene collector: process/session identity, missing/faulted/overflowed/malformed evidence, no false ownership, ZIP'
}finally{if(Test-Path $temp){Remove-Item $temp -Recurse -Force}}
