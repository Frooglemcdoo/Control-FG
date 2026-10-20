$ErrorActionPreference = 'Stop'
try {
    $game = Split-Path -Parent $MyInvocation.MyCommand.Path
    if (!(Test-Path (Join-Path $game 'Control_DX12.exe'))) { throw 'Place this selector beside Control_DX12.exe with the R53 DLL.' }
    if (Get-Process -Name Control,Control_DX12,Control_DX11 -ErrorAction SilentlyContinue) { throw 'Close Control before changing diagnostic mode.' }
    Write-Host 'R53 lighting isolation - restart Control after selecting a mode'
    Write-Host '0: Native lighting (comparison)'
    Write-Host '1: Disable baked GI contribution'
    Write-Host '2: Disable deferred direct lighting contribution'
    $choice = Read-Host 'Choose 0, 1, or 2'
    if ($choice -notin @('0','1','2')) { throw 'Invalid selection. No files changed.' }
    foreach ($marker in @('ControlFG-Disable-Direct-R53.txt','ControlFG-Disable-BakedGI-R53.txt')) {
        $path = Join-Path $game $marker
        if (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path }
    }
    if ($choice -eq '1') { Set-Content -LiteralPath (Join-Path $game 'ControlFG-Disable-BakedGI-R53.txt') -Value 'R53 baked GI isolation' -Encoding ASCII }
    if ($choice -eq '2') { Set-Content -LiteralPath (Join-Path $game 'ControlFG-Disable-Direct-R53.txt') -Value 'R53 direct lighting isolation' -Encoding ASCII }
    Write-Host "Selected mode $choice. Launch Control and compare the same stationary view."
} catch {
    Write-Host $_ -ForegroundColor Red
    exit 1
}
