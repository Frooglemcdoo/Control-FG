param([string]$PackageRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PackageRoot 'Build-Metadata.ps1')
$checks = New-Object 'System.Collections.Generic.List[string]'
function Pass([string]$Name) { $checks.Add($Name) }
function Expect-Rejected([string]$Name, $Value) {
    $rejected=$false
    try { Assert-ControlFGBuildValidation $Value } catch { $rejected=$true }
    if (-not $rejected) { throw ('Validation accepted invalid fixture: '+$Name) }
    Pass $Name
}
function New-Validation {
    $value=[ordered]@{}
    foreach($key in $ControlFGBuild.Keys) { $value[$key]=$ControlFGBuild[$key] }
    $value.AbiCheck='Passed';$value.ExportCheck='Passed';$value.Architecture='x64';$value.SHA256=('a'*64)
    return [pscustomobject]$value
}
$good=New-Validation
Assert-ControlFGBuildValidation $good
Pass 'Current G4 shared contract accepted'
foreach($key in $ControlFGBuild.Keys) {
    $bad=New-Validation
    $bad.PSObject.Properties.Remove($key)
    Expect-Rejected ('Missing '+$key) $bad
}
$bad=New-Validation;$bad.Version='1.0.0-RR-Native-G3';Expect-Rejected 'G3 validation rejected' $bad
$bad=New-Validation;$bad.RRPhase='NativeG3ShaderModel51MaterialImages';Expect-Rejected 'G3 phase rejected' $bad
$bad=New-Validation;$bad.SourceRevision='r1-rr-native-g3';Expect-Rejected 'G3 revision rejected' $bad
$bad=New-Validation;$bad.RRNativePrimaryViewRoutingCorrected=$false;Expect-Rejected 'Disabled primary routing correction rejected' $bad
$bad=New-Validation;$bad.RRNativePrimaryViewRoutingCorrected='true';Expect-Rejected 'String bool rejected' $bad
$bad=New-Validation;$bad.RREvaluationEnabled=$true;Expect-Rejected 'Enabled RR evaluation rejected' $bad
$bad=New-Validation;$bad.NativeDenoiserBypassEnabled=$true;Expect-Rejected 'Enabled denoiser bypass rejected' $bad
$bad=New-Validation;$bad.SHA256='';Expect-Rejected 'Missing binary hash rejected' $bad
foreach($file in @(Get-ChildItem -LiteralPath $PackageRoot -Filter '*.ps1' -File)) {
    $tokens=$null;$errors=$null
    [void][System.Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors)
    if($errors.Count) { throw ('Parse failure '+$file.Name+': '+$errors[0].Message) }
    Pass ('PowerShell parse '+$file.Name)
}
# Run the actual collector against mixed G3/G4 logs. The legacy export format is deliberate;
# the first PROBE line, not file recency or later log text, defines package identity.
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('control-g4-metadata-'+[guid]::NewGuid().ToString('N'))
$priorLocal=$env:LOCALAPPDATA;$priorTemp=$env:TEMP
try {
    $fixturePackage=Join-Path $fixture 'package';$fixtureLocal=Join-Path $fixture 'local';$fixtureTemp=Join-Path $fixture 'temp'
    $logs=Join-Path $fixtureLocal 'ControlFGProbe'
    foreach($path in @($fixturePackage,$logs,$fixtureTemp)) { [void](New-Item -ItemType Directory -Path $path -Force) }
    foreach($name in @('Build-Metadata.ps1','Collect-ControlFG-Logs.ps1')) { Copy-Item -LiteralPath (Join-Path $PackageRoot $name) -Destination (Join-Path $fixturePackage $name) }
    $g3=Join-Path $logs 'probe-20260913-150000-001-111.log';$g4=Join-Path $logs 'probe-20260913-140000-001-222.log';$decoy=Join-Path $logs 'probe-20260913-160000-001-333.log'
    '[0] PROBE v1.0.0-RR-Native-G3 source_revision=r1-rr-native-g3' | Set-Content -LiteralPath $g3
    '[0] PROBE v1.0.0-RR-Native-G4 source_revision=r1-rr-native-g4 rr_phase=NativeG4PrimaryViewMaterialImages' | Set-Content -LiteralPath $g4
    "[0] PROBE v1.0.0-RR-Native-G3 old`n[1] PROBE v1.0.0-RR-Native-G4 unrelated later text" | Set-Content -LiteralPath $decoy
    (Get-Item $g4).LastWriteTimeUtc=[DateTime]::UtcNow.AddHours(-1)
    $env:LOCALAPPDATA=$fixtureLocal;$env:TEMP=$fixtureTemp
    $powershell=(Get-Process -Id $PID).Path
    & $powershell -NoProfile -File (Join-Path $fixturePackage 'Collect-ControlFG-Logs.ps1')
    if($LASTEXITCODE -ne 0) { throw 'Actual G4 collector failed fixture' }
    $zips=@(Get-ChildItem -LiteralPath $fixturePackage -Filter '*.zip' -File)
    if($zips.Count -ne 1 -or $zips[0].Name -notlike 'Control-FG-v1.0.0-RR-Native-G4-Logs-*') { throw 'Collector ZIP identity wrong' }
    Pass 'Actual collector ZIP has G4 identity'
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($zips[0].FullName)
    try {
        $entries=@($zip.Entries | ForEach-Object {$_.FullName})
        $entry=$zip.GetEntry('collection-report.json');if($null -eq $entry){throw 'Missing collection report'}
        $reader=New-Object IO.StreamReader($entry.Open())
        try {$report=$reader.ReadToEnd() | ConvertFrom-Json}finally{$reader.Dispose()}
        if($report.Version -cne $ControlFGBuild.Version -or $report.ProbeLogs.Count -ne 1 -or $report.ProbeLogs[0] -cne (Split-Path -Leaf $g4) -or $report.MatchingProcessIds.Count -ne 1 -or $report.MatchingProcessIds[0] -cne '222') {throw 'Collector selected non-G4 logs'}
        Pass 'Only G4 first-line identity and PID selected'
        if(@($entries | Where-Object {$_ -match '111|333'}).Count){throw 'Collector included G3 files'}
        Pass 'Newer G3 logs and later-line G4 decoy excluded'
        if($report.Captures.Count -ne 0){throw 'Unexpected fake capture'}
        Pass 'No-image run remains collectable'
    } finally {$zip.Dispose()}
} finally {
    $env:LOCALAPPDATA=$priorLocal;$env:TEMP=$priorTemp
    if(Test-Path -LiteralPath $fixture){Remove-Item -LiteralPath $fixture -Recurse -Force}
}
$result=[ordered]@{Status='PASS';Checks=$checks.Count;Cases=@($checks.ToArray());Scope='PowerShell 7.6.6 on Linux. Actual shared contract and collector fixtures; no Windows install, MSVC or game execution.'}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'metadata-validation.json') -Encoding utf8
Write-Host ('PASS: '+$checks.Count+' metadata and collector checks')
