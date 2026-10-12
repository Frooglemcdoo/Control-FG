param([string]$GameExe, [string]$GameArguments = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
try {
    if (-not $GameExe) {
        Add-Type -AssemblyName System.Windows.Forms
        $picker = New-Object System.Windows.Forms.OpenFileDialog
        $picker.Title = 'Select Control_DX12.exe'
        $picker.Filter = 'Control DX12|Control_DX12.exe'
        if ($picker.ShowDialog() -ne 'OK') { exit 1 }
        $GameExe = $picker.FileName
        $picker.Dispose()
    }
    if (Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue) {
        throw 'Exit Control first. PIX must launch it before D3D12 initializes.'
    }
    $python = Get-Command py.exe -ErrorAction SilentlyContinue
    if (-not $python) { throw 'Install 64-bit Python 3.12 with the Windows py launcher. See README.' }
    & $python.Source -3.12 -c "import struct; assert struct.calcsize('P') == 8"
    if ($LASTEXITCODE -ne 0) { throw '64-bit Python 3.12 is required. See README.' }
    $venv = Join-Path $PSScriptRoot '.venv'
    $runner = Join-Path $venv 'Scripts\python.exe'
    if (-not (Test-Path -LiteralPath $runner)) {
        & $python.Source -3.12 -m venv $venv
        if ($LASTEXITCODE -ne 0) { throw 'Could not create the private Python environment.' }
    }
    & $runner -c "import pythonnet"
    if ($LASTEXITCODE -ne 0) {
        Write-Host 'Installing pythonnet into this tool folder only...'
        & $runner -m pip install 'pythonnet==3.0.5'
        if ($LASTEXITCODE -ne 0) { throw 'pythonnet installation failed.' }
    }
    & $runner (Join-Path $PSScriptRoot 'capture.py') --exe $GameExe "--game-args=$GameArguments"
    if ($LASTEXITCODE -ne 0) { throw 'Capture did not finish. Existing data was retained; see the output folder and console error.' }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
