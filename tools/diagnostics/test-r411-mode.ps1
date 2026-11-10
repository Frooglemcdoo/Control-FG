$ErrorActionPreference='Stop'
$folder=Join-Path ([IO.Path]::GetTempPath()) ("R411 test "+[guid]::NewGuid())
try {
 $tools=Join-Path $folder 'ControlTOH\Tools'
 New-Item -ItemType Directory -Path $tools -Force | Out-Null
 Copy-Item (Join-Path $PSScriptRoot 'Set-R411-Mode.ps1') $tools
 foreach($mode in @('Clean','Diagnostics')){
  $dir=Join-Path $folder "ControlTOH\Diagnostics\R411-$mode"
  New-Item -ItemType Directory -Path $dir -Force | Out-Null
  $file=Join-Path $dir 'dxgi.dll'
  [IO.File]::WriteAllText($file,$mode)
  (Get-FileHash $file -Algorithm SHA256).Hash | Set-Content "$file.sha256"
 }
 foreach($mode in @('Clean','Diagnostics','Clean')){
  & (Join-Path $tools 'Set-R411-Mode.ps1') -Mode $mode
  if([IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne $mode){throw 'Mode switch did not select requested DLL'}
 }
 $bad=Join-Path $folder 'ControlTOH\Diagnostics\R411-Diagnostics\dxgi.dll'
 [IO.File]::WriteAllText($bad,'bad')
 $rejected=$false
 try { & (Join-Path $tools 'Set-R411-Mode.ps1') -Mode Diagnostics } catch { $rejected=$true }
 if(!$rejected -or [IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne 'Clean'){throw 'Corrupt input was not rejected before replacement'}
 Write-Host 'PASS R411 mode selection: paths with spaces, clean/diagnostic/clean, corrupt-source rejection'
} finally { Remove-Item -LiteralPath $folder -Recurse -Force -ErrorAction SilentlyContinue }
