param(
    [string]$Source = (Join-Path $PSScriptRoot '..\src\shaders\pt3_pathtrace.hlsl'),
    [string]$Output = (Join-Path $PSScriptRoot '..\build\pt3_pathtrace.dxil'),
    [string]$Header = (Join-Path $PSScriptRoot '..\build\pt3_pathtrace_compiled.h')
)
$ErrorActionPreference='Stop'
$candidates=@()
$cmd=Get-Command dxc.exe -ErrorAction SilentlyContinue
if($cmd){$candidates+=$cmd.Source}
$kits=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
if(Test-Path $kits){$candidates+=Get-ChildItem -LiteralPath $kits -Directory|Sort-Object Name -Descending|ForEach-Object{Join-Path $_.FullName 'x64\dxc.exe'}}
$dxc=$candidates|Where-Object{$_ -and (Test-Path -LiteralPath $_ -PathType Leaf)}|Select-Object -First 1
if(!$dxc){throw 'DXC not found.'}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output)|Out-Null
$include=Split-Path -Parent $Source
& $dxc -T cs_6_5 -E main -I $include -O3 -Ges -Qstrip_debug -Qstrip_reflect -Fo $Output $Source
if($LASTEXITCODE -ne 0){throw "DXC failed with exit code $LASTEXITCODE"}
$bytes=[IO.File]::ReadAllBytes($Output)
if($bytes.Length -lt 128){throw 'PT3 DXIL output is unexpectedly small.'}
$sb=[Text.StringBuilder]::new()
[void]$sb.AppendLine('#pragma once')
[void]$sb.AppendLine('#include <cstddef>')
[void]$sb.AppendLine('static const unsigned char kPT3PathTraceShader[] = {')
for($i=0;$i -lt $bytes.Length;$i++){
 if(($i%16)-eq 0){[void]$sb.Append('    ')}
 [void]$sb.Append(('0x{0:X2}' -f $bytes[$i]))
 if($i+1 -lt $bytes.Length){[void]$sb.Append(',')}
 if(($i%16)-eq 15 -or $i+1 -eq $bytes.Length){[void]$sb.AppendLine()}else{[void]$sb.Append(' ')}
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('static constexpr std::size_t kPT3PathTraceShaderSize = sizeof(kPT3PathTraceShader);')
[IO.File]::WriteAllText($Header,$sb.ToString(),[Text.UTF8Encoding]::new($false))
Write-Host ("PASS PT3 DXIL: dxc={0} bytes={1}" -f $dxc,$bytes.Length)
