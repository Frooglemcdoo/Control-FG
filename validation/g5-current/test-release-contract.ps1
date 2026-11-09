#requires -Version 7.0
param([Parameter(Mandatory=$true)][string]$SourceRoot, [Parameter(Mandatory=$true)][string]$OutputPath)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
. (Join-Path $SourceRoot 'Build-Metadata.ps1')
$results = [Collections.Generic.List[object]]::new()
function Assert-Case([string]$Name,[bool]$Pass) {
    if (-not $Pass) { throw ('FAIL: ' + $Name) }
    $results.Add([pscustomobject]@{name=$Name;passed=$true})
}
$valid = [ordered]@{}
foreach ($key in $ControlFGBuild.Keys) { $valid[$key] = $ControlFGBuild[$key] }
$valid['AbiCheck']='Passed'; $valid['ExportCheck']='Passed'; $valid['Architecture']='x64'
$valid['SHA256']='0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
Assert-ControlFGBuildValidation ([pscustomobject]$valid)
Assert-Case 'Current full G5 contract accepted' $true
Assert-Case 'G5 identity advertised' ($ControlFGBuild.Version -ceq '1.0.0-RR-Native-G5')
Assert-Case 'Joined-frame scheduling advertised' ($ControlFGBuild.RRNativeJoinedFrameCaptureEnabled -eq $true)
Assert-Case 'View correction advertised' ($ControlFGBuild.RRNativePrimaryViewRoutingCorrected -eq $true)
Assert-Case 'RR evaluation disabled' ($ControlFGBuild.RREvaluationEnabled -eq $false)
Assert-Case 'Denoiser bypass disabled' ($ControlFGBuild.NativeDenoiserBypassEnabled -eq $false)
Assert-Case 'Semantic guide mapping disabled' ($ControlFGBuild.RRSemanticGuideMappingEnabled -eq $false)
Assert-Case 'Capture file format retained accurately' ($ControlFGCapture.Schema -ceq 'ControlFG.RRGuideG3.Capture.v1')
foreach ($key in $ControlFGBuild.Keys) {
    $bad = [ordered]@{}
    foreach ($k in $valid.Keys) { $bad[$k]=$valid[$k] }
    $bad.Remove($key)
    $rejected=$false
    try { Assert-ControlFGBuildValidation ([pscustomobject]$bad) } catch { $rejected=$true }
    Assert-Case ('Missing field rejected: '+$key) $rejected
    $bad[$key]= if ($valid[$key] -is [bool]) { -not $valid[$key] } elseif ($valid[$key] -is [string]) { 'incorrect' } else { -999 }
    $rejected=$false
    try { Assert-ControlFGBuildValidation ([pscustomobject]$bad) } catch { $rejected=$true }
    Assert-Case ('Wrong field rejected: '+$key) $rejected
}
$old=[ordered]@{}
foreach ($k in $valid.Keys) { $old[$k]=$valid[$k] }
$old['Version']='1.0.0-RR-Native-G3'
$old['SourceRevision']='r1-rr-native-g3'
$old['RRPhase']='NativeG3ShaderModel51MaterialImages'
$rejected=$false
try { Assert-ControlFGBuildValidation ([pscustomobject]$old) } catch { $rejected=$true }
Assert-Case 'Old G3 validation rejected' $rejected
$report=[ordered]@{status='PASS';cases=$results.Count;source_root=$SourceRoot;tests=$results;scope='Actual shared validator, synthetic build metadata. No Windows build/install/game or GPU execution.'}
$report|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $OutputPath -Encoding UTF8
Write-Host ('G5_CONTRACT_FIXTURES=PASS COUNT='+$results.Count)
