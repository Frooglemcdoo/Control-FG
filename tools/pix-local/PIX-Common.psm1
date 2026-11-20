Set-StrictMode -Version Latest
function Find-PixTool([string]$Explicit) {
 if($Explicit){return (Resolve-Path -LiteralPath $Explicit -ErrorAction Stop).Path}
 $root=Join-Path $env:ProgramFiles 'Microsoft PIX'
 $found=@(Get-ChildItem -LiteralPath $root -Filter pixtool.exe -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTimeUtc -Descending)
 if(!$found.Count){throw 'PIX was not found. Install PIX or pass -PixTool with the full path to pixtool.exe.'}
 return $found[0].FullName
}
function Invoke-Pix([string]$Tool,[string[]]$Arguments,[string]$Log,[switch]$HelpQuery) {
 if($HelpQuery -and ($Arguments.Count -ne 2 -or $Arguments[0] -ne '--help')){throw 'Help-only exit handling cannot be used for PIX operations.'}
 # Some PIX versions return -1 after printing help. Only a help query may
 # tolerate that status; Assert-PixCommand must still validate its contents.
 $priorPreference=$ErrorActionPreference
 try {
  $ErrorActionPreference='Continue'
  $text=@(& $Tool @Arguments 2>&1 | ForEach-Object {"$_"})
  $code=$LASTEXITCODE
 } finally {$ErrorActionPreference=$priorPreference}
 @("PIX executable: $Tool", "Exit code: $code")+$text | Set-Content -LiteralPath $Log -Encoding UTF8
 if($code -ne 0 -and !($HelpQuery -and $code -eq -1)){
  throw "PIX failed (exit $code). See $Log`n$($text -join "`n")"
 }
 return ($text -join "`n")
}
function Assert-PixCommand([string]$Tool,[string]$Command,[string[]]$Options,[string]$Folder) {
 $log=Join-Path $Folder ('help-'+$Command+'.txt')
 $help=Invoke-Pix $Tool @('--help',$Command) $log -HelpQuery
 if($help -notmatch [regex]::Escape($Command)){throw "PIX help did not confirm $Command. See $log`n$help"}
 foreach($option in $Options){if($help -notmatch [regex]::Escape($option)){throw "PIX help did not confirm $option for $Command. See $log`n$help"}}
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
