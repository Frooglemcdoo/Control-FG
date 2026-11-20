Set-StrictMode -Version Latest
function Find-PixTool([string]$Explicit) {
 if($Explicit){return (Resolve-Path -LiteralPath $Explicit -ErrorAction Stop).Path}
 $root=Join-Path $env:ProgramFiles 'Microsoft PIX'
 $found=@(Get-ChildItem -LiteralPath $root -Filter pixtool.exe -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending)
 if(!$found.Count){throw 'PIX was not found. Install PIX or pass -PixTool with the full path to pixtool.exe.'}
 return $found[0].FullName
}
function Invoke-Pix([string]$Tool,[string[]]$Arguments,[string]$Log,[switch]$HelpQuery) {
 if($HelpQuery -and ($Arguments.Count -ne 2 -or $Arguments[0] -ne '--help')){throw 'Help-only exit handling cannot be used for PIX operations.'}
 # Some PIX versions return -1 after printing help. Only a help query may
 # tolerate that status; Assert-PixCommand must still validate its contents.
 $priorPreference=$ErrorActionPreference
 try {
  $ErrorActionPreference='Continue'
  $text=@(& $Tool @Arguments 2>&1 | ForEach-Object {"$_"})
  $code=$LASTEXITCODE
 } finally {$ErrorActionPreference=$priorPreference}
 @("PIX executable: $Tool", "Exit code: $code")+$text | Set-Content -LiteralPath $Log -Encoding UTF8
 if($code -ne 0 -and !($HelpQuery -and $code -eq -1)){
  throw "PIX failed (exit $code). See $Log`n$($text -join "`n")"
 }
 return ($text -join "`n")
}
function Assert-PixCommand([string]$Tool,[string]$Command,[string[]]$Options,[string]$Folder) {
 $log=Join-Path $Folder ('help-'+$Command+'.txt')
 $help=Invoke-Pix $Tool @('--help',$Command) $log -HelpQuery
 if($help -notmatch [regex]::Escape($Command)){throw "PIX help did not confirm $Command. See $log`n$help"}
 foreach($option in $Options){if($help -notmatch [regex]::Escape($option)){throw "PIX help did not confirm $option for $Command. See $log`n$help"}}
}
function New-LaunchArgs([string]$GameExe) {
 # Control_DX12.exe selects DX12 directly. PIX defaults the working directory
 # to the executable directory; no version-specific option syntax is needed.
 return @('launch',$GameExe)
}
function New-CaptureArgs([int]$ProcessId,[int]$Frames,[string]$Path) {
 if($ProcessId -le 0 -or $Frames -lt 1 -or $Frames -gt 32){throw 'Invalid capture process/frame count.'}
 return @('attach',"$ProcessId",'take-capture','--frames',"$Frames",'--open','save-capture',$Path)
}
function Get-AnalysisSteps([string]$Capture,[string]$Folder) {
 return @(
  @{name='screenshot'; output=(Join-Path $Folder 'capture-screenshot.png'); args=@('open-capture',$Capture,'save-screenshot',(Join-Path $Folder 'capture-screenshot.png'))},
  @{name='events'; output=(Join-Path $Folder 'events.csv'); args=@('open-capture',$Capture,'save-event-list',(Join-Path $Folder 'events.csv'))},
  @{name='playback'; output=$null; args=@('open-capture',$Capture,'perform-single-playback')},
  @{name='rtv0'; output=(Join-Path $Folder 'replayed-rtv0.png'); args=@('open-capture',$Capture,'save-resource',(Join-Path $Folder 'replayed-rtv0.png'))}
 )
}

function Initialize-SteamDebugLaunch([string]$GameExe) {
 # Valve's documented debugger launch path. Derive the ID from this install,
 # never guess an app ID or overwrite a user's existing file.
 $gameDir=Split-Path $GameExe
 $common=Split-Path $gameDir
 if((Split-Path $common -Leaf) -ne 'common'){return $null}
 $steamapps=Split-Path $common
 if((Split-Path $steamapps -Leaf) -ne 'steamapps'){return $null}
 $install=Split-Path $gameDir -Leaf
 $ids=@(foreach($manifest in Get-ChildItem -LiteralPath $steamapps -Filter 'appmanifest_*.acf'){
  $body=Get-Content -LiteralPath $manifest.FullName -Raw
  $dirMatch=[regex]::Match($body,'"installdir"\s+"([^"]+)"')
  $idMatch=[regex]::Match($body,'"appid"\s+"(\d+)"')
  if($dirMatch.Success -and $dirMatch.Groups[1].Value -eq $install -and $idMatch.Success){$idMatch.Groups[1].Value}
 })
 if($ids.Count -ne 1){throw 'Could not uniquely identify this Steam install from appmanifest files. No App ID file was changed.'}
 if(!(Get-Process -Name steam -ErrorAction SilentlyContinue)){throw 'Open Steam and sign in first, then run Capture-PIX.cmd again. Leave Control closed.'}
 $path=Join-Path $gameDir 'steam_appid.txt'
 $state=@{path=$path; appId=$ids[0]; created=$false; hash=$null}
 if(Test-Path -LiteralPath $path){
  if((Get-Content -LiteralPath $path -Raw).Trim() -ne $ids[0]){throw 'Existing steam_appid.txt does not match the installed game. It was left unchanged.'}
  return $state
 }
 $bytes=[Text.Encoding]::ASCII.GetBytes($ids[0])
 $file=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 try{$file.Write($bytes,0,$bytes.Length)}finally{$file.Dispose()}
 $state.created=$true
 $state.hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
 return $state
}
function Restore-SteamDebugLaunch($State) {
 if($null -eq $State -or !$State.created){return}
 if(Test-Path -LiteralPath $State.path){
  if((Get-FileHash -LiteralPath $State.path -Algorithm SHA256).Hash -eq $State.hash){Remove-Item -LiteralPath $State.path}
  else{Write-Warning 'steam_appid.txt changed during the session and was left untouched.'}
 }
}
function Wait-PixGame([string]$GameExe,[string]$Tool,[string]$Log,[int]$TimeoutSeconds=45) {
 $timer=[Diagnostics.Stopwatch]::StartNew()
 $expected=Join-Path (Split-Path $Tool) 'WinPixGpuCapturer.dll'
 $lastKey='';$stable=0
 while($timer.Elapsed.TotalSeconds -lt $TimeoutSeconds){
  $processes=@(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
  $ready=$null
  foreach($process in $processes){
   try {
    $path=$process.Path
    $modules=@($process.Modules | Where-Object {$_.ModuleName -ieq 'WinPixGpuCapturer.dll'})
    $line="PID=$($process.Id) path=$path window=$($process.MainWindowHandle) capturer=$(@($modules | ForEach-Object {$_.FileName}) -join ',')"
    Add-Content -LiteralPath $Log -Value ("$(Get-Date -Format o) $line")
    if($path -ieq $GameExe -and $process.MainWindowHandle -ne 0 -and @($modules | Where-Object {$_.FileName -ieq $expected}).Count -eq 1){$ready=$process}
   } catch {Add-Content -LiteralPath $Log -Value ("$(Get-Date -Format o) PID=$($process.Id) inspection: $($_.Exception.Message)")}
  }
  if(!$processes.Count){Add-Content -LiteralPath $Log -Value ("$(Get-Date -Format o) No Control_DX12 process")}
  if($ready){
   $key="$($ready.Id):$($ready.StartTime.Ticks)"
   if($key -eq $lastKey){$stable++}else{$lastKey=$key;$stable=1}
   if($stable -ge 3){return $ready}
  }else{$lastKey='';$stable=0}
  Start-Sleep -Milliseconds 500
 }
 throw "Control did not reach a visible window with this PIX capturer loaded within $TimeoutSeconds seconds. See startup.log and startup-events.txt; no capture was attempted."
}
Export-ModuleMember -Function *
