param(
    [ValidateSet('Bypass','PT-Off','PT-On')][string]$Mode,
    [ValidateSet('DLAA','Quality')][string]$Resolution,
    [string]$OutputDirectory=(Join-Path ([Environment]::GetFolderPath('Desktop')) 'Control-PT-Profiles'),
    [ValidateRange(10,60)][int]$Seconds=20
)
$ErrorActionPreference='Stop'
$admin=New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $admin.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
    throw 'Right-click PT-Profile.cmd and choose Run as administrator. Windows requires elevation for CPU/GPU tracing.'
}
if(-not $Mode){
    Write-Host '1 = startup capture bypass; 2 = normal capture, PT OFF; 3 = normal capture, PT ON'
    $choice=Read-Host 'Choose the CURRENT game state (1/2/3)'
    $Mode=switch($choice){'1'{'Bypass'} '2'{'PT-Off'} '3'{'PT-On'} default{throw 'Choose 1, 2 or 3.'}}
}
if(-not $Resolution){
    $choice=Read-Host 'CURRENT DLSS setting: 1 = DLAA; 2 = Quality'
    $Resolution=switch($choice){'1'{'DLAA'} '2'{'Quality'} default{throw 'Choose 1 or 2.'}}
}
$wpr=(Get-Command wpr.exe -ErrorAction Stop).Source
function Invoke-Recorder {
    param([string[]]$Arguments)
    $saved=$ErrorActionPreference
    try {
        $ErrorActionPreference='Continue'
        $output=(& $wpr @Arguments 2>&1 | Out-String)
        $code=$LASTEXITCODE
    } finally {$ErrorActionPreference=$saved}
    [pscustomobject]@{Output=$output;Code=$code}
}
$result=Invoke-Recorder -Arguments @('-profiles')
$profiles=$result.Output
if($result.Code -ne 0 -or $profiles -notmatch '\bGeneralProfile\b' -or $profiles -notmatch '\bGPU\b'){
    throw 'This Windows recorder lacks GeneralProfile or GPU. Send the output of wpr -profiles; no trace was started.'
}
# Refuse existing OR unrecognized status. Never cancel someone else's capture.
$status=(Invoke-Recorder -Arguments @('-status')).Output
if($status -notmatch '(?i)not recording'){
    throw "Recorder is active or its status is unrecognized. Finish any existing recording first. Status: $status"
}
$game=@(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
if($game.Count -ne 1){throw 'Run one instance of Control_DX12.exe and load the test scene first.'}
$game=$game[0]
$buildPath=Join-Path $PSScriptRoot 'Build.json'
$build=Get-Content -LiteralPath $buildPath -Raw | ConvertFrom-Json
if($build.Build -ne 'M12R58'){throw 'This capture script requires the M12R58 profiling package.'}
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'SHA256SUMS.txt')
$expected=@($manifest | Where-Object {$_ -match '^[0-9a-fA-F]{64}\s+dxgi\.dll$'})
if($expected.Count -ne 1){throw 'Package must contain exactly one dxgi.dll hash.'}
$hash=($expected[0] -split '\s+')[0].ToLowerInvariant()
# Windows may report the same path in long or 8.3 form. Match the loaded
# module's bytes, not its spelling; the system dxgi.dll will not match.
$loaded=@($game.Modules | Where-Object {$_.ModuleName -ieq 'dxgi.dll'} | Where-Object {
    (Get-FileHash -LiteralPath $_.FileName -Algorithm SHA256).Hash.ToLowerInvariant() -eq $hash
})
if($loaded.Count -ne 1){throw 'The running game has no unique dxgi.dll matching this R41 package. Install together and restart Control.'}
$logDir=Join-Path $env:LOCALAPPDATA 'ControlFGProbe'
$record=Get-ChildItem -LiteralPath $logDir -Filter ('probe-*-'+$game.Id+'.log') -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if(-not $record){throw 'Cannot find a probe log for this game process. Restart Control after installing R41.'}
$startup=@(Get-Content -LiteralPath $record.FullName | Where-Object {$_ -match 'PT_PRIMARY_STARTUP build=M12R58 capture=[01]'})
if($startup.Count -ne 1){throw 'Loaded game does not have one R41 startup record. Restart after installation.'}
$actualBypass=$startup[0] -match 'capture=0'
if(($Mode -eq 'Bypass') -ne $actualBypass){throw 'Selected mode disagrees with the RUNNING game. Use the baseline/restore script and restart Control.'}
$dir=Join-Path $OutputDirectory ($Mode+'-'+$Resolution+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$profiles | Set-Content -LiteralPath (Join-Path $dir 'WPR-profiles.txt')
$meta=[ordered]@{Build='M12R58';Commit=$build.Commit;DllSHA256=$hash;LoadedDll=$loaded[0].FileName;ProcessId=$game.Id;ProcessStartUtc=$game.StartTime.ToUniversalTime().ToString('o');RequestedMode=$Mode;RequestedResolution=$Resolution;StartupCaptureDisabled=$actualBypass;Seconds=$Seconds;Profile='GeneralProfile+GPU';SettingsAreUserLabels=$true;StartupRecord=$startup[0]}
Write-Host 'Keep the same scene and camera, FG OFF, VSync/frame cap OFF, RR unchanged.'
Write-Host 'Set the selected PT/DLSS state and wait for it to settle before continuing.'
Read-Host 'Press Enter when ready. You then have 10 seconds to return to the game' | Out-Null
Start-Sleep -Seconds 10
$trace=Join-Path $dir 'CPU-GPU.etl'
$started=$false
try {
    $result=Invoke-Recorder -Arguments @('-start','GeneralProfile','-start','GPU','-filemode')
    $output=$result.Output;$code=$result.Code
    if($code -eq 0){$started=$true}
    $output | Set-Content -LiteralPath (Join-Path $dir 'WPR-start.txt')
    if($code -ne 0){throw "WPR could not start ($code). No existing recording will be cancelled. $output"}
    $started=$true
    $meta['StartUtc']=[DateTime]::UtcNow.ToString('o')
    Start-Sleep -Seconds $Seconds
} finally {
    if($started){
        $statusResult=Invoke-Recorder -Arguments @('-status','collectors','-details')
        # Save/stop before writing status: a status-file error must not leave recording active.
        $result=Invoke-Recorder -Arguments @('-stop',$trace,('Control R46 '+$Mode+' '+$Resolution),'-skipPdbGen','-compress')
        $output=$result.Output;$code=$result.Code
        $statusResult.Output | Set-Content -LiteralPath (Join-Path $dir 'WPR-status.txt')
        $output | Set-Content -LiteralPath (Join-Path $dir 'WPR-stop.txt')
        if($code -ne 0){throw "WPR stop failed ($code). Output is in $dir. In an elevated prompt run: wpr -stop `"$trace`""}
        $meta['EndUtc']=[DateTime]::UtcNow.ToString('o')
    }
}
$game.Refresh()
$meta['GameExitedDuringCapture']=$game.HasExited
$meta | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $dir 'Capture.json') -Encoding UTF8
Copy-Item -LiteralPath $record.FullName -Destination $dir
Copy-Item -LiteralPath $buildPath -Destination $dir
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'SHA256SUMS.txt') -Destination $dir
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=$dir+'.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($dir,$zip,[IO.Compression.CompressionLevel]::Fastest,$false)
Write-Host "Saved: $zip"
Write-Host 'The source capture folder is retained too. Upload the ZIP for this state.'
