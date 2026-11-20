[CmdletBinding()]
param([ValidateSet('Capture','Analyze')][string]$Mode='Capture',
 [string]$GameExe,[string]$CapturePath,[string]$OutputRoot,[string]$PixTool,
 [ValidateRange(1,32)][int]$Frames=16)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'PIX-Common.psm1') -Force
$steamState=$null
$started=Get-Date
try {
 $tool=Find-PixTool $PixTool
 if(!$OutputRoot){$OutputRoot=Read-Host 'Output folder on a drive with free space (all data stays here)'}
 if([string]::IsNullOrWhiteSpace($OutputRoot)){throw 'An output folder is required.'}
 $root=[IO.Path]::GetFullPath($OutputRoot)
 $folder=Join-Path $root ('Control-PIX-'+(Get-Date -Format yyyyMMdd-HHmmss)+'-'+[guid]::NewGuid().ToString('N').Substring(0,6))
 New-Item -ItemType Directory -Path $folder -Force | Out-Null
 Start-Transcript -Path (Join-Path $folder 'session.log') | Out-Null
 $meta=[ordered]@{schema='control-pix-local-1'; mode=$Mode; pix=$tool; pixVersion=(Get-Item -LiteralPath $tool).VersionInfo.FileVersion; framesRequested=$Frames; capture=$null; replayVerified=$false; artifactConfirmed=$false; computer=$env:COMPUTERNAME}
 if($Mode -eq 'Capture') {
  if(!$GameExe){$GameExe=(Read-Host 'Full path to Control_DX12.exe').Trim('"')}
  $GameExe=(Resolve-Path -LiteralPath $GameExe).Path
  if([IO.Path]::GetFileName($GameExe) -ne 'Control_DX12.exe'){throw 'Select Control_DX12.exe.'}
  if(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue){throw 'Close Control before launching this workflow. PIX must instrument startup.'}
  Assert-PixCommand $tool 'launch' @() $folder
  Assert-PixCommand $tool 'attach' @() $folder
  Assert-PixCommand $tool 'take-capture' @('--frames','--open') $folder
  Assert-PixCommand $tool 'save-capture' @() $folder
  # Clear diagnostic launch flags only in this helper process, inherited by the game.
  $clear=@('CONTROLTOH_GPU_FAULT','CONTROLTOH_GI_AUTO','CONTROLTOH_GI_AUTO_RAYS','CONTROLTOH_GI_TEXTURE_MIP1','CONTROLTOH_GI_SCENE_CAPTURE','CONTROLTOH_GI_CAPTURE','CONTROLTOH_GI_SPOT_TRACE','CONTROLTOH_GI_JOINT_TRACE','CONTROLTOH_GI_FROZEN_RENDER','CONTROLTOH_GI_FROZEN_PREPARE','CONTROLTOH_GI_MOTION_CAPTURE')
  foreach($name in $clear){[Environment]::SetEnvironmentVariable($name,$null,'Process')}
  $meta['clearedDiagnosticVariables']=$clear
  $meta['game']=$GameExe
  $dll=Join-Path (Split-Path $GameExe) 'dxgi.dll'
  if(Test-Path -LiteralPath $dll){$meta['modSha256']=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash}
  $steamState=Initialize-SteamDebugLaunch $GameExe
  if($steamState){$meta['steamAppId']=$steamState.appId;Write-Host "Steam debugger launch: App ID $($steamState.appId) from installed manifest."}
  Write-Host 'Launching through PIX. Use RR ON and FG OFF; keep your normal GI settings.'
  Write-Host 'Do NOT start the F10 custom capture or the old motion launcher.'
  Invoke-Pix $tool (New-LaunchArgs $GameExe) (Join-Path $folder 'launch.log') | Out-Host
  Write-Host 'Checking game startup and PIX instrumentation (up to 45 seconds)...'
  $launched=Wait-PixGame $GameExe $tool (Join-Path $folder 'startup.log')
  $meta['launchedPid']=$launched.Id
  $meta | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $folder 'manifest.json') -Encoding UTF8
  Restore-SteamDebugLaunch $steamState
  $steamState=$null
  Read-Host 'Load the scene. When ready, return here and press Enter to arm a 7-second countdown' | Out-Null
  $game=@(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
  if($game.Count -ne 1){throw 'Expected exactly one running Control_DX12 process.'}
  if($game[0].Id -ne $launched.Id -or $game[0].StartTime -ne $launched.StartTime){throw 'Control restarted after PIX startup. Capture cancelled to avoid using an uninstrumented process.'}
  if([IO.Path]::GetFullPath($game[0].Path) -ne $GameExe){throw 'Running Control path differs from the selected game.'}
  Write-Host 'Return to the game and keep Jesse moving through the affected light. No keys are injected.'
  for($n=7;$n -ge 1;$n--){Write-Host "Capture in $n...";Start-Sleep -Seconds 1}
  $CapturePath=Join-Path $folder 'motion.wpix'
  $arguments=New-CaptureArgs $game[0].Id $Frames $CapturePath
  Invoke-Pix $tool $arguments (Join-Path $folder 'capture.log') | Out-Host
  if(!(Test-Path -LiteralPath $CapturePath) -or (Get-Item -LiteralPath $CapturePath).Length -eq 0){throw 'PIX returned without a nonempty capture. See capture.log.'}
  $meta['capture']=$CapturePath
  $meta | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $folder 'manifest.json') -Encoding UTF8
  Write-Host "Saved: $CapturePath"
  Write-Host 'Close Control before local replay to free GPU memory. Run Analyze-PIX.cmd and select this file.'
 } else {
  if(!$CapturePath){$CapturePath=(Read-Host 'Full path to the .wpix capture').Trim('"')}
  $CapturePath=(Resolve-Path -LiteralPath $CapturePath).Path
  if(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue){throw 'Close Control before replay analysis.'}
  $meta['capture']=$CapturePath
  $meta['captureBytes']=(Get-Item -LiteralPath $CapturePath).Length
  $results=@()
  foreach($step in (Get-AnalysisSteps $CapturePath $folder)){
   Write-Host "Running PIX $($step.name)..."
   $result=[ordered]@{step=$step.name; success=$false; output=$step.output; error=$null}
   try{
    Invoke-Pix $tool $step.args (Join-Path $folder ($step.name+'.log')) | Out-Host
    if($step.output -and (!(Test-Path -LiteralPath $step.output) -or (Get-Item -LiteralPath $step.output).Length -eq 0)){throw 'Expected export was not created.'}
    $result.success=$true
   } catch {$result.error=$_.Exception.Message;Write-Warning $result.error}
   $results+=,$result
  }
  $meta['results']=$results
  $meta | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $folder 'manifest.json') -Encoding UTF8
  Write-Host "Analysis outputs: $folder"
  Write-Host 'Replay success is NOT visual equivalence. rtv0 is not guaranteed to be the final screen.'
  Write-Host 'Open the capture in PIX and verify the speckle is present. Give local Codex this folder and the original .wpix path.'
  if(@($results | Where-Object {!$_.success}).Count){throw 'One or more analysis steps failed. Logs and partial exports were preserved.'}
 }
 Stop-Transcript | Out-Null
 exit 0
} catch {
 Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
 if($folder){
  try {
   Get-WinEvent -FilterHashtable @{LogName='Application';StartTime=$started;Id=1000,1001,1002} -ErrorAction Stop |
    Where-Object {$_.Message -match 'Control_DX12|WinPix|pixtool'} |
    Format-List TimeCreated,ProviderName,Id,Message | Out-File (Join-Path $folder 'startup-events.txt') -Encoding UTF8
  }catch{"No matching Application events available: $($_.Exception.Message)" | Set-Content (Join-Path $folder 'startup-events.txt')}
  Write-Host "Startup diagnostics: $folder"
 }
 try{Stop-Transcript | Out-Null}catch{}
 exit 1
} finally {
 try{Restore-SteamDebugLaunch $steamState}catch{Write-Warning "Could not remove temporary Steam App ID file: $($_.Exception.Message)"}
}
