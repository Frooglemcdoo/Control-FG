param([switch]$SelfTest)
$ErrorActionPreference='Stop'
$child=$null
try {
  if (-not [Environment]::Is64BitProcess) { throw 'Run this collector with 64-bit Windows PowerShell.' }
  Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class GIFreezeDump {
 [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr OpenProcess(uint access,bool inherit,uint pid);
 [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
 [DllImport("Dbghelp.dll",SetLastError=true)] static extern bool MiniDumpWriteDump(IntPtr process,uint pid,IntPtr file,uint type,IntPtr exception,IntPtr user,IntPtr callback);
 public static void Save(uint pid,string path) {
  IntPtr process=OpenProcess(0x0410,false,pid);
  if(process==IntPtr.Zero)throw new Win32Exception(Marshal.GetLastWin32Error());
  try { using(var file=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None)) {
   if(!MiniDumpWriteDump(process,pid,file.SafeFileHandle.DangerousGetHandle(),0x1020,IntPtr.Zero,IntPtr.Zero,IntPtr.Zero))throw new Win32Exception(Marshal.GetLastWin32Error());
  }} finally {CloseHandle(process);}
 }
}
'@
  if($SelfTest) {
    $child=Start-Process "$PSHOME\powershell.exe" -ArgumentList '-NoProfile -Command Start-Sleep -Seconds 90' -PassThru -WindowStyle Hidden
    $game=$child
  } else {
    $games=@(Get-Process -Name Control_DX12 -ErrorAction SilentlyContinue)
    if($games.Count -ne 1){throw 'Leave exactly one frozen Control_DX12.exe process running, then run this collector.'}
    $game=$games[0]
  }
  $folder=Join-Path $PSScriptRoot ('Control-GI-Freeze-'+(Get-Date -Format yyyyMMdd-HHmmss)+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
  New-Item -ItemType Directory -Path $folder | Out-Null
  Write-Host 'Capturing three small thread-stack dumps. Leave the game running.'
  for($i=1;$i -le 3;$i++) {
    $game.Refresh()
    if($game.HasExited){throw 'The game exited before capture completed.'}
    $dump=Join-Path $folder ("threads-$i.dmp")
    [GIFreezeDump]::Save([uint32]$game.Id,$dump)
    $stream=[IO.File]::OpenRead($dump)
    try {$magic=New-Object byte[] 4;[void]$stream.Read($magic,0,4)}finally{$stream.Dispose()}
    if([Text.Encoding]::ASCII.GetString($magic) -ne 'MDMP'){throw 'Invalid minidump output.'}
    [ordered]@{Utc=[DateTime]::UtcNow.ToString('o');ProcessId=$game.Id;CpuSeconds=$game.TotalProcessorTime.TotalSeconds;WorkingSet=$game.WorkingSet64;Threads=@($game.Threads | ForEach-Object { $reason='';try{if($_.ThreadState -eq 'Wait'){$reason=$_.WaitReason.ToString()}}catch{};[ordered]@{Id=$_.Id;State=$_.ThreadState.ToString();Wait=$reason} })} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $folder "threads-$i.json") -Encoding UTF8
    if($i -lt 3){Start-Sleep -Seconds 2}
  }
  $zip="$folder.zip"
  Compress-Archive -Path "$folder\*" -DestinationPath $zip
  Write-Host "SUCCESS: Upload this ZIP: $zip"
  if(-not $SelfTest){Start-Process explorer.exe -ArgumentList ('/select,"'+$zip+'"')}
} catch {
  Write-Host "Capture failed: $($_.Exception.Message)" -ForegroundColor Red
  Write-Host 'If access was denied, right-click Collect-GI-Freeze.cmd and select Run as administrator.'
  exit 1
} finally {
  if($child -and -not $child.HasExited){Stop-Process -Id $child.Id -Force}
}
exit 0
