param([Parameter(Mandatory=$true)][string]$Folder,[Parameter(Mandatory=$true)][string]$ModuleFolder)
$ErrorActionPreference='Stop'
try {
    $source=(Resolve-Path -LiteralPath $Folder).Path
    $module=(Resolve-Path -LiteralPath $ModuleFolder).Path
    $installed=@(Get-Item -LiteralPath (Join-Path $module 'dxgi.dll'))
    $shaders=Join-Path $module 'ControlTOH\Shaders'
    if (Test-Path -LiteralPath $shaders -PathType Container) {
        $installed+=@(Get-ChildItem -LiteralPath $shaders -File | Where-Object { $_.Extension -in '.dxil','.dxbc' })
    }
    $hashes=@($installed | ForEach-Object {
        $h=Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256
        [pscustomobject]@{name=$_.FullName.Substring($module.Length+1);bytes=$_.Length;sha256=$h.Hash}
    })
    $hashes | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $source 'installed-files.json') -Encoding UTF8
    Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,PNPDeviceID | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $source 'gpu-driver.json') -Encoding UTF8
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $temporary=$source+'.pending.zip'
    [System.IO.Compression.ZipFile]::CreateFromDirectory($source,$temporary)
    if((Get-Item -LiteralPath $temporary).Length -ge 200000000){throw 'Package exceeds 200 MB; originals retained. Use Package-Lighting-Capture.cmd to split.'}
    $archive=[System.IO.Compression.ZipFile]::OpenRead($temporary)
    try {foreach($entry in $archive.Entries){$stream=$entry.Open();try{$stream.CopyTo([System.IO.Stream]::Null)}finally{$stream.Dispose()}}} finally {$archive.Dispose()}
    Move-Item -LiteralPath $temporary -Destination ($source+'.zip')
    (Get-FileHash -LiteralPath ($source+'.zip') -Algorithm SHA256).Hash | Set-Content -LiteralPath ($source+'.sha256') -Encoding ASCII
    exit 0
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $Folder 'packaging-error.txt') -Encoding UTF8
    exit 1
}
