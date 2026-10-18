param([string]$BuildDirectory='build',[string]$OutputDirectory='.')
$ErrorActionPreference='Stop'
$buildName='M12R103DEV'
$name='Control-FG-PT-Primary-M12R103DEV-Test'
$repo=Split-Path -Parent $PSScriptRoot
$destination=Join-Path $OutputDirectory $name
if(Test-Path -LiteralPath $destination){throw "Package directory already exists: $destination"}
New-Item -ItemType Directory -Path $destination | Out-Null
$shaders=@('pt_surface_reconstruct.dxil','pt_primary_surface.dxil','pt_emitter_build.dxil',
    'pt_primary_path.dxil','pt_reflection_p1.dxil','pt_reflection_p2.dxil',
    'pt_reflection_p2_composite.dxbc','pt_reflection_p2_verify.dxbc',
    'pt_reflection_p2_local_light_probe.dxbc','pt_reflection_p2_diffuse_bridge.dxbc',
    'pt_glass_raygen.dxil','pt_glass_secondary.dxbc','pt_glass_composite.dxbc')
foreach($file in @('dxgi.dll','dxgi.pdb')+$shaders){
    $source=Join-Path $BuildDirectory $file
    if(-not (Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing build output: $file"}
    if($file -in $shaders){
        $bytes=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $source).Path)
        if($bytes.Length -lt 32 -or $bytes.Length -gt 1MB -or
            [Text.Encoding]::ASCII.GetString($bytes,0,4) -ne 'DXBC' -or
            [BitConverter]::ToUInt32($bytes,24) -ne $bytes.Length){throw "Invalid shader sidecar: $file"}
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $destination $file)
}
$helpers=@('PT-Profile.cmd','PT-Profile.ps1','PT-Capture-Baseline.cmd','PT-Capture-Restore.cmd',
    'PT-Enable.cmd','PT-Enable.ps1','RT-Only.cmd',
    'Collect-PT-Primary-M12R103DEV.cmd','Collect-PT-Primary-M12R103DEV.ps1')
foreach($file in $helpers){Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $destination}
Copy-Item -LiteralPath (Join-Path $repo 'research/pt-reflections/PT-Primary-M12R103DEV-README.txt') -Destination (Join-Path $destination 'README.txt')
if(-not $env:GITHUB_SHA -or -not $env:GITHUB_RUN_ID){throw 'Build provenance is missing.'}
[ordered]@{
    Build=$buildName;Commit=$env:GITHUB_SHA;Run=$env:GITHUB_RUN_ID
    PackageKind='experimental_update_for_same_installation';FullPTComplete=$false
    SurfaceReplacementEnabled=$true;CompositionProfiles=17
    OpaqueTransportBakedGI=$false;OpaqueTransportProbeLighting=$false
    NativeEnvironmentAdmissionDependency=$false;LightVisibility='all_traced'
    EmitterSampling='triangle_area_nee_and_bsdf_mis';EmitterCapacity=65536
    PrimaryNormals='grazing_regularized_shared_with_reconstruction_and_rr'
    MissingLightingFixed=$false;FlickerResolved=$false;InGameValidated=$false
    RemainingNativeLighting=@('glass','fog_volumetrics','unsupported_or_rejected_surface_fallback')
    RemainingCoverage=@('complete_scene_lights','physical_sky','runtime_emitter_coverage')
    Validation='Windows host builds and WARP regressions; in-game test required'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destination 'Build.json') -Encoding UTF8
Get-ChildItem -LiteralPath $destination -File | Sort-Object Name | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(),$_.Name
} | Set-Content -LiteralPath (Join-Path $destination 'SHA256SUMS.txt') -Encoding ASCII
$zip=Join-Path $OutputDirectory ($name+'.zip')
Compress-Archive -Path (Join-Path $destination '*') -DestinationPath $zip -Force
# Validate the actual downloadable bytes, not only the staging directory.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $zip).Path)
try {
    $expected=@(Get-ChildItem -LiteralPath $destination -File)
    if($archive.Entries.Count -ne $expected.Count){throw 'Archive file count mismatch.'}
    foreach($file in $expected){
        $entry=$archive.GetEntry($file.Name)
        if(-not $entry -or $entry.Length -ne $file.Length){throw "Archive entry mismatch: $($file.Name)"}
        $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
        try {$actual=([BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-','')}
        finally {$sha.Dispose();$stream.Dispose()}
        if($actual -ne (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash){throw "Archive hash mismatch: $($file.Name)"}
    }
} finally {$archive.Dispose()}
(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant() | Set-Content -LiteralPath (Join-Path $OutputDirectory ($name+'.sha256')) -Encoding ASCII
Write-Host "PASS: $buildName test ZIP includes DLL, symbols, 13 shader sidecars and diagnostics; every archived file matches its staging hash."
