#requires -Version 5.1
param([string]$Root = (Join-Path $PSScriptRoot '../../..'))
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path -LiteralPath $Root).Path
. (Join-Path $Root 'Build-Metadata.ps1')
$parse = @()
foreach ($file in @(Get-ChildItem -LiteralPath $Root -File -Filter '*.ps1' -Recurse)) {
    $tokens=$null; $errors=$null
    [void][System.Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors)
    if ($errors.Count) { throw ('PowerShell syntax: ' + $file.FullName + ': ' + $errors[0].Message) }
    $parse += $file.FullName.Substring($Root.Length+1).Replace('\','/')
}
$checks=[ordered]@{}
$checks.Version = $ControlFGBuild.Version -ceq '1.0.0-RR-Native-G5'
$checks.Revision = $ControlFGBuild.SourceRevision -ceq 'r1-rr-native-g5'
$checks.Phase = $ControlFGBuild.RRPhase -ceq 'NativeG5JoinedFrameMaterialImages'
$checks.TypedJoinedFrameEnabled = $ControlFGBuild.RRNativeJoinedFrameCaptureEnabled -is [bool] -and $ControlFGBuild.RRNativeJoinedFrameCaptureEnabled -eq $true
$checks.ExistingPrimaryViewEnabled = $ControlFGBuild.RRNativePrimaryViewRoutingCorrected -is [bool] -and $ControlFGBuild.RRNativePrimaryViewRoutingCorrected -eq $true
$checks.PairedMarker = $ControlFGGuideMarkers -ccontains 'RR_GUIDE_G5_PAIRED_OPPORTUNITY'
$checks.CapturePrefix = $ControlFGCapture.DirectoryPrefix -ceq 'rr-guide-g3-'
$checks.CaptureSchema = $ControlFGCapture.Schema -ceq 'ControlFG.RRGuideG3.Capture.v1'
$checks.CollectionSchema = $ControlFGCapture.CollectionSchema -ceq 'ControlFG.RRGuideG3.Collection.v1'
$checks.OneFrame = $ControlFGBuild.RRPrimaryGuideCaptureLimit -eq 1
$checks.RREvaluationOff = $ControlFGBuild.RREvaluationEnabled -is [bool] -and $ControlFGBuild.RREvaluationEnabled -eq $false
$checks.DenoiserBypassOff = $ControlFGBuild.NativeDenoiserBypassEnabled -is [bool] -and $ControlFGBuild.NativeDenoiserBypassEnabled -eq $false
$probe = Get-Content -LiteralPath (Join-Path $Root 'src/probe.cpp') -Raw
$checks.ProbeIdentity = $probe.Contains('PROBE v1.0.0-RR-Native-G5 ') -and $probe.Contains('source_revision=r1-rr-native-g5') -and $probe.Contains('rr_phase=NativeG5JoinedFrameMaterialImages')
$guide=''
foreach($file in @(Get-ChildItem -LiteralPath (Join-Path $Root 'src') -File -Filter 'rr_*.h')) { $guide += Get-Content -LiteralPath $file.FullName -Raw }
$checks.MarkersInSource = @($ControlFGGuideMarkers | Where-Object { -not ($probe+$guide).Contains($_) }).Count -eq 0
$checks.SharedInstallContract = $true
foreach ($name in @('Verify-Build.ps1','Manage-Probe.ps1','Make-DropIn.ps1')) {
    $s=Get-Content -LiteralPath (Join-Path $Root $name) -Raw
    if (-not $s.Contains('Build-Metadata.ps1') -or -not $s.Contains('Assert-ControlFGBuildValidation')) { $checks.SharedInstallContract = $false }
}
$frozen = Get-Content -LiteralPath (Join-Path $Root 'g3-frozen-files.json') -Raw | ConvertFrom-Json
$checks.ElevenFrozenHashes = $frozen.Files.Count -eq 11
foreach ($entry in $frozen.Files) {
    if ((Get-FileHash -LiteralPath (Join-Path $Root $entry.Path) -Algorithm SHA256).Hash -ine $entry.SHA256) { $checks.ElevenFrozenHashes = $false }
}
foreach ($key in $checks.Keys) { if (-not $checks[$key]) { throw ('Identity review: ' + $key) } }
[ordered]@{
    Status='PASS'
    HostPowerShell=$PSVersionTable.PSVersion.ToString()
    NativeWindowsExecution=$false
    CheckCount=$checks.Count
    Checks=$checks
    ParsedScriptCount=$parse.Count
    ParsedScripts=$parse
    SourceManifest='Deferred until final package report collection; not refreshed by this check.'
} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'identity-review.json') -Encoding UTF8
Write-Host ('PASS: ' + $checks.Count + ' identity/frozen checks and ' + $parse.Count + ' PowerShell parse checks.')
