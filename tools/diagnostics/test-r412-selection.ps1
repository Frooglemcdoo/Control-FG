$ErrorActionPreference='Stop'
$folder=Join-Path ([IO.Path]::GetTempPath()) ("R412 test "+[guid]::NewGuid())
try {
 $tools=Join-Path $folder 'ControlTOH\Tools'
 New-Item -ItemType Directory -Path $tools -Force | Out-Null
 Copy-Item (Join-Path $PSScriptRoot 'Set-R412-Selection.ps1') $tools
 foreach($mode in @('Native','Owned')){
  $dir=Join-Path $folder "ControlTOH\Diagnostics\R412-$mode"
  New-Item -ItemType Directory -Path $dir -Force | Out-Null
  $file=Join-Path $dir 'dxgi.dll'
  [IO.File]::WriteAllText($file,$mode)
  (Get-FileHash $file -Algorithm SHA256).Hash | Set-Content "$file.sha256"
 }
 foreach($mode in @('Native','Owned','Native')){
  & (Join-Path $tools 'Set-R412-Selection.ps1') -Mode $mode
  if([IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne $mode){throw 'Mode switch did not select requested DLL'}
 }
 $bad=Join-Path $folder 'ControlTOH\Diagnostics\R412-Owned\dxgi.dll'
 [IO.File]::WriteAllText($bad,'bad')
 $rejected=$false
 try { & (Join-Path $tools 'Set-R412-Selection.ps1') -Mode Owned } catch { $rejected=$true }
 if(!$rejected -or [IO.File]::ReadAllText((Join-Path $folder 'dxgi.dll')) -ne 'Native'){throw 'Corrupt input was not rejected before replacement'}
 Write-Host 'PASS R412 mode selection: paths with spaces, native/owned/native, corrupt-source rejection'
} finally { Remove-Item -LiteralPath $folder -Recurse -Force -ErrorAction SilentlyContinue }
