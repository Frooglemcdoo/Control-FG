$ErrorActionPreference='Stop'
$folder=Join-Path ([IO.Path]::GetTempPath()) ("R413 test "+[guid]::NewGuid())
try {
 $tools=Join-Path $folder 'ControlTOH\Tools'
 New-Item -ItemType Directory -Path $tools -Force | Out-Null
 Copy-Item (Join-Path $PSScriptRoot 'Set-R413-Preparation.ps1') $tools
 foreach($mode in @('NativePrep','OwnedPrep')){
  $dir=Join-Path $folder "ControlTOH\Diagnostics\R413-$mode"
  New-Item -ItemType Directory -Path $dir -Force | Out-Null
  $file=Join-Path $dir 'dxgi.dll'
  [IO.File]::WriteAllText($file,$mode)
  (Get-FileHash $file -Algorithm SHA256).Hash | Set-Content "$file.sha256"
 }
 foreach($mode in @('NativePrep','OwnedPrep','NativePrep')){
  & (Join-Path $tools 'Set-R413-Preparation.ps1') -Mode $mode
  if([IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne $mode){throw 'Mode switch did not select requested DLL'}
 }
 $bad=Join-Path $folder 'ControlTOH\Diagnostics\R413-OwnedPrep\dxgi.dll'
 [IO.File]::WriteAllText($bad,'bad')
 $rejected=$false
 try { & (Join-Path $tools 'Set-R413-Preparation.ps1') -Mode OwnedPrep } catch { $rejected=$true }
 if(!$rejected -or [IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne 'NativePrep'){throw 'Corrupt input was not rejected before replacement'}
 Write-Host 'PASS R413 mode selection: paths with spaces, native-prep/owned-prep/native-prep, corrupt-source rejection'
} finally { Remove-Item -LiteralPath $folder -Recurse -Force -ErrorAction SilentlyContinue }
