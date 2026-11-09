$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression
$root=Join-Path ([IO.Path]::GetTempPath()) ('TextureTraceTest-'+[guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($root)
try {
    $path=Join-Path $root 'probe-fixture.log'
    $bytes=New-Object byte[] (5MB)
    for($i=0;$i -lt $bytes.Length;$i+=4096){$bytes[$i]=[byte](($i/4096)%251)}
    [IO.File]::WriteAllBytes($path,$bytes)
    # Keep the source open for writes, as in the running game.
    $held=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
    try {$zip=& "$PSScriptRoot/Collect-Texture-Trace.ps1" -LogDirectory $root -OutputDirectory $root} finally {$held.Dispose()}
    $file=[IO.File]::OpenRead($zip);$archive=New-Object IO.Compression.ZipArchive($file,[IO.Compression.ZipArchiveMode]::Read)
    try {
        $entry=$archive.GetEntry('probe-fixture.log');if($entry.Length -ne $bytes.Length){throw 'Length mismatch'}
        $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
        try {$actual=[Convert]::ToBase64String($sha.ComputeHash($stream));$expected=[Convert]::ToBase64String($sha.ComputeHash($bytes));if($actual -ne $expected){throw 'Byte mismatch'}} finally {$sha.Dispose();$stream.Dispose()}
    } finally {$archive.Dispose();$file.Dispose()}
    $defaultFolder=Join-Path $root 'Default Output With Spaces'
    [void][IO.Directory]::CreateDirectory($defaultFolder)
    $script=Join-Path $defaultFolder 'Collect-Texture-Trace.ps1'
    Copy-Item -LiteralPath "$PSScriptRoot/Collect-Texture-Trace.ps1" -Destination $script
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script -LogDirectory $root
    if ($LASTEXITCODE -ne 0) {throw 'Default output -File invocation failed'}
    if (@(Get-ChildItem -LiteralPath $defaultFolder -Filter '*.zip').Count -ne 1) {throw 'Default ZIP missing'}
    Write-Host 'PASS R254 full-session collector: byte equality, active writer sharing, default output via PowerShell -File and paths with spaces'
} finally {Remove-Item -LiteralPath $root -Recurse -Force}
