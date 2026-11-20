Set-StrictMode -Version Latest
function Find-PixTool([string]$Explicit) {
 if($Explicit){return (Resolve-Path -LiteralPath $Explicit -ErrorAction Stop).Path}
 $root=Join-Path $env:ProgramFiles 'Microsoft PIX'
 $found=@(Get-ChildItem -LiteralPath $root -Filter pixtool.exe -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending)
 if(!$found.Count){throw 'PIX was not found. Install PIX or pass -PixTool with the full path to pixtool.exe.'}
 return $found[0].FullName
}
function Invoke-Pix([string]$Tool,[string[]]$Arguments,[string]$Log) {
 # Argument arrays preserve spaces. Never evaluate command text.
 $text=@(& $Tool @Arguments 2>&1 | ForEach-Object {"$_"})
 $code=$LASTEXITCODE
 $text | Set-Content -LiteralPath $Log -Encoding UTF8
 if($code -ne 0){throw "PIX failed (exit $code). See $Log"}
 return ($text -join "`n")
}
function Assert-PixCommand([string]$Tool,[string]$Command,[string[]]$Options,[string]$Folder) {
 $help=Invoke-Pix $Tool @('--help',$Command) (Join-Path $Folder ('help-'+$Command+'.txt'))
 if($help -notmatch [regex]::Escape($Command)){throw "Installed PIX does not advertise $Command. See the saved help file."}
 foreach($option in $Options){if($help -notmatch [regex]::Escape($option)){throw "Installed PIX does not advertise $option for $Command. Update PIX; no capture started."}}
}
function New-CaptureArgs([int]$ProcessId,[int]$Frames,[string]$Path) {
 if($ProcessId -le 0 -or $Frames -lt 1 -or $Frames -gt 32){throw 'Invalid capture process/frame count.'}
 return @('attach',"$ProcessId",'take-capture',"--frames=$Frames",'--open','save-capture',$Path)
}
function Get-AnalysisSteps([string]$Capture,[string]$Folder) {
 return @(
  @{name='screenshot'; output=(Join-Path $Folder 'capture-screenshot.png'); args=@('open-capture',$Capture,'save-screenshot',(Join-Path $Folder 'capture-screenshot.png'))},
  @{name='events'; output=(Join-Path $Folder 'events.csv'); args=@('open-capture',$Capture,'save-event-list',(Join-Path $Folder 'events.csv'))},
  @{name='playback'; output=$null; args=@('open-capture',$Capture,'perform-single-playback')},
  @{name='rtv0'; output=(Join-Path $Folder 'replayed-rtv0.png'); args=@('open-capture',$Capture,'save-resource',(Join-Path $Folder 'replayed-rtv0.png'))}
 )
}
Export-ModuleMember -Function *
