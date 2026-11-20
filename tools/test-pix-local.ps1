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
 Assert ($a.Count -eq 7 -and $a[0] -eq 'attach' -and $a[3] -eq '--frames=16' -and $a[6] -eq 'D:\captures with spaces\motion.wpix') 'Capture argv split or changed'
 Assert (($a -join ' ') -notmatch 'F7|Control_L|SendKeys') 'Unexpected input injection'
 $steps=@(Get-AnalysisSteps 'D:\captures with spaces\motion.wpix' $tmp)
 Assert ($steps.Count -eq 4) 'Missing independent analysis phases'
 foreach($s in $steps){Assert ($s.args[1] -eq 'D:\captures with spaces\motion.wpix') 'Lost source path'}
 $fake=Join-Path $tmp 'fake pix.ps1'
 @'
param([Parameter(ValueFromRemainingArguments=$true)][string[]]$Values)
if($Values -contains 'fail'){ $global:LASTEXITCODE=7; 'simulated PIX failure'; return }
$global:LASTEXITCODE=0
'launch --working-directory --command-line take-capture --frames --open save-capture attach'
'@ | Set-Content $fake
 Assert-PixCommand $fake 'take-capture' @('--frames','--open') $tmp
 $rejected=$false
 try{Assert-PixCommand $fake 'take-capture' @('--invented') $tmp}catch{$rejected=$true}
 Assert $rejected 'Unsupported option was accepted'
 $failed=$false
 try{Invoke-Pix $fake @('fail') (Join-Path $tmp 'failure.log')}catch{$failed=$true}
 Assert $failed 'Nonzero PIX exit ignored'
 Assert ((Get-Content (Join-Path $tmp 'failure.log') -Raw) -match 'simulated PIX failure') 'Failure log lost'
 Write-Host 'PASS Windows parsing, argument paths, capture frames, supported-option checks, independent analysis stages, nonzero exit preservation'
} finally {Remove-Item -LiteralPath $tmp -Recurse -Force}
