param([int]$GameProcessId, [int]$Session)
$ErrorActionPreference='Stop'
$script=Join-Path $PSScriptRoot 'collect-rr-suite.py'
if (!(Test-Path -LiteralPath $script)) {$script=Join-Path $PSScriptRoot 'tools\collect-rr-suite.py'}
$arguments=@($script,'--pid',$GameProcessId,'--session',$Session,'--output',[Environment]::GetFolderPath('Desktop'))
if (Get-Command py -ErrorAction SilentlyContinue) {& py -3 @arguments} else {& python @arguments}
if ($LASTEXITCODE -ne 0) {throw 'Sequence incomplete. Send the packaged diagnostics; originals were retained.'}
