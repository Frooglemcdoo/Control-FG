$ErrorActionPreference='Stop'
$tokens=$null;$errors=$null
$path=Join-Path $PSScriptRoot 'Capture-GI-Button.ps1'
$ast=[System.Management.Automation.Language.Parser]::ParseFile($path,[ref]$tokens,[ref]$errors)
if($errors.Count){throw ($errors | Out-String)}
$function=$ast.Find({param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-GIButtonMeasurement'},$true)
Invoke-Expression $function.Extent.Text
$script:events=New-Object System.Collections.Generic.List[string]
function Start-Sleep {param([int]$Seconds) $script:events.Add("delay:$Seconds")}
function Mark {param([string]$Name) $script:events.Add("mark:$Name")}
function Tone {param([int]$Count) $script:events.Add("beep:$Count")}
function Sample {param([int]$Seconds,[string]$Phase) $script:events.Add("sample:$Phase`:$Seconds")}
Invoke-GIButtonMeasurement
$expected='delay:5|mark:STILL_BEGIN|beep:1|sample:STILL:10|mark:STILL_END|mark:PAN_BEGIN|beep:1|sample:PAN:10|mark:PAN_END|beep:2'
if(($script:events -join '|') -ne $expected){throw ($script:events -join '|')}
# Failure before the measured end must not emit the success double-beep.
$script:events.Clear()
function Sample {param([int]$Seconds,[string]$Phase) throw 'simulated game exit'}
$failed=$false
try{Invoke-GIButtonMeasurement}catch{$failed=$true}
if(-not $failed -or $script:events.Contains('beep:2')){throw 'False completion signal after early exit'}
$text=Get-Content -Raw $path
if($text -match 'Read-Host|KeyAvailable|GetAsyncKeyState|recorder-check|Start-Process.*Control_DX12'){throw 'Unexpected prompt, key handling, preflight capture or game relaunch'}
if($text -match 'GeneralProfile|cpu-gpu-timeline|function Wpr|package-lighting-capture.py'){throw 'Heavy trace path must not be part of compact capture'}
Write-Output 'PASS: five-second delay; still and pan phases; exact 1/1/2 beep order; early failure never signals done; no console prompts/hotkeys/preflight/relaunch.'
