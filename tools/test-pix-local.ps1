$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$kit=Join-Path $PSScriptRoot 'pix-local'
foreach($p in Get-ChildItem $kit -Include *.ps1,*.psm1 -Recurse){
 $tokens=$null;$errors=$null
 [System.Management.Automation.Language.Parser]::ParseFile($p.FullName,[ref]$tokens,[ref]$errors) | Out-Null
 if($errors.Count){throw ($errors | Out-String)}
}
Import-Module (Join-Path $kit 'PIX-Common.psm1') -Force
function Assert($condition,$message){if(!$condition){throw $message}}
$tmp=Join-Path $env:TEMP ('PIX tests '+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $tmp | Out-Null
try {
 $a=@(New-CaptureArgs 123 16 'D:\captures with spaces\motion.wpix')
 Assert ($a.Count -eq 8 -and $a[0] -eq 'attach' -and $a[3] -eq '--frames' -and $a[4] -eq '16' -and $a[7] -eq 'D:\captures with spaces\motion.wpix') 'Capture argv split or changed'
 Assert (($a -join ' ') -notmatch 'F7|Control_L|SendKeys') 'Unexpected input injection'
 # Exercise Windows PowerShell -> native executable argument serialization,
 # including the user's exact Program Files (x86) path. No mocked PS binding.
 $native=Join-Path $tmp 'argv probe.exe'
 Add-Type -TypeDefinition @'
using System;
public class PixArgumentProbe {
 public static int Main(string[] args) {
  foreach(string arg in args) Console.WriteLine(arg);
  if(args.Length==2 && args[0]=="launch" && args[1]==@"C:\Program Files (x86)\Steam\steamapps\common\Control\Control_DX12.exe") return 0;
  if(args.Length==8 && args[0]=="attach" && args[1]=="123" && args[2]=="take-capture" && args[3]=="--frames" && args[4]=="16" && args[5]=="--open" && args[6]=="save-capture" && args[7]==@"D:\captures with spaces\motion.wpix") return 0;
  return 23;
 }
}
'@ -OutputAssembly $native -OutputType ConsoleApplication
 $launchArgs=New-LaunchArgs 'C:\Program Files (x86)\Steam\steamapps\common\Control\Control_DX12.exe'
 Invoke-Pix $native $launchArgs (Join-Path $tmp 'native-launch.log') | Out-Null
 Invoke-Pix $native $a (Join-Path $tmp 'native-capture.log') | Out-Null
 $steps=@(Get-AnalysisSteps 'D:\captures with spaces\motion.wpix' $tmp)
 Assert ($steps.Count -eq 4) 'Missing independent analysis phases'
 foreach($s in $steps){Assert ($s.args[1] -eq 'D:\captures with spaces\motion.wpix') 'Lost source path'}
 $fake=Join-Path $tmp 'fake pix.ps1'
 @'
param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Values)
if($Values -contains 'fail'){ $global:LASTEXITCODE=7; 'simulated PIX failure'; return }
$global:LASTEXITCODE=-1
'launch --working-directory --command-line take-capture --frames --open save-capture attach'
'@ | Set-Content $fake
 Assert-PixCommand $fake 'take-capture' @('--frames','--open') $tmp
 $rejected=$false
 try{Assert-PixCommand $fake 'take-capture' @('--invented') $tmp}catch{$rejected=$true}
 Assert $rejected 'Unsupported option was accepted'
 $guarded=$false
 try{Invoke-Pix $fake @('launch','game.exe') (Join-Path $tmp 'unsafe-help.log') -HelpQuery}catch{$guarded=$true}
 Assert $guarded 'Help exception leaked into launch'
 $negativeFailed=$false
 try{Invoke-Pix $fake @('launch') (Join-Path $tmp 'negative.log')}catch{$negativeFailed=$_.Exception.Message -match 'launch --working-directory'}
 Assert $negativeFailed 'Operational -1 ignored or underlying output hidden'
 $failed=$false
 try{Invoke-Pix $fake @('fail') (Join-Path $tmp 'failure.log')}catch{$failed=$true}
 Assert $failed 'Nonzero PIX exit ignored'
 Assert ((Get-Content (Join-Path $tmp 'failure.log') -Raw) -match 'simulated PIX failure') 'Failure log lost'
 Write-Host 'PASS Windows parsing, argument paths, capture frames, supported-option checks, independent analysis stages, nonzero exit preservation'
} finally {Remove-Item -LiteralPath $tmp -Recurse -Force}

exit 0
