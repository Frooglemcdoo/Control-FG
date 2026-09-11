#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$Version = '2.14.1'
$SourceCommit = '2122257e0fce486f91b385aa63b9a09b0a34b363'
$Manifest = @'
27b01b61fb2b2fe08f9b1736c95a89a31198dc68|sl.h
d1179bae0b68afda393ef2dcfe0be03ff9d2281d|sl_appidentity.h
15589e577df98edcbba1fcdb54e9b6cfbdc409b7|sl_consts.h
26d9f617f365b1be7c5a788b664e8ef0f5e3c2fd|sl_core_api.h
a255f5c5e99aa1a3a1a8121c76b77f6e71ed1bb1|sl_core_types.h
87cffc96ba349f1e8c378a4734a837523a178012|sl_deepdvc.h
5804ccbd960a7704b274fe56a8e15fd780373e78|sl_device_wrappers.h
ac80959fd4ce94b6a3185fade3953db209fbc93c|sl_directsr.h
9a875bd4f843fdd594ee1afd01eef8060b75b0f4|sl_dlss.h
f17cab37510b835e4bb231a9a5e93843290f4ce5|sl_dlss_d.h
67ba0a0505e4df314aeb9695f61ea3787dc26132|sl_dlss_g.h
05ce1560143dd28a2269abd81fd0f108792024e8|sl_helpers.h
a6deba60a9abc3b0cc40f6d785cb16295163d282|sl_helpers_vk.h
4b523bbcd212bd10119b77791f88c008bda47297|sl_hooks.h
96a544beec5e767fa2f5015dda2605e9611f53c8|sl_matrix_helpers.h
68ec7b6d69752cb5edb3b9aaf2494f42edf63ac7|sl_nis.h
704169754224243a0ca0440d645127898ba25006|sl_nvperf.h
d7c26c93c5adc715eeaee285f9e613230ae9cb75|sl_pcl.h
ad821fe648c151681d047014441fc112789df58a|sl_reflex.h
8a844b68ff555c8c1ace7c45c3a8efc69c5754b3|sl_result.h
db1d1255d98005ce25e9b9aa165d15e792dc26cd|sl_security.h
28af1aaec7d34b5f5ede067b893809605855fc88|sl_struct.h
af049516a4adb9e10aa72ef38b64bdbfc7c34c60|sl_template.h
9bfcf8892116944cdedae63f2e4a31bf5ff19a78|sl_version.h
'@
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $thirdParty = Join-Path $PSScriptRoot 'third_party\streamline'
    $includeStage = Join-Path $thirdParty 'include'
    $infoPath = Join-Path $thirdParty 'headers-info.json'
    if (Test-Path -LiteralPath $thirdParty) { Remove-Item -LiteralPath $thirdParty -Recurse -Force }
    New-Item -ItemType Directory -Path $includeStage -Force | Out-Null
    $records = @()
    foreach ($line in ($Manifest -split "\r?\n")) {
        if (-not $line.Trim()) { continue }
        $parts = $line.Split('|', 2)
        $expectedBlob = $parts[0]
        $name = $parts[1]
        $uri = 'https://raw.githubusercontent.com/NVIDIA-RTX/Streamline/' + $SourceCommit + '/include/' + $name
        $dest = Join-Path $includeStage $name
        Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $dest
        $actualBlob = (& git hash-object --no-filters -- $dest).Trim()
        if ($LASTEXITCODE -ne 0 -or $actualBlob -ne $expectedBlob) { throw ('Streamline header blob mismatch: ' + $name) }
        $records += [ordered]@{ Path=('include/' + $name); GitBlob=$actualBlob; SHA256=(Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash; Size=(Get-Item -LiteralPath $dest).Length }
    }
    [ordered]@{ Version=$Version; Source='NVIDIA-RTX/Streamline immutable GitHub commit'; SourceCommit=$SourceCommit; Scope='Public headers only for FSR input bridge'; Files=$records } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $infoPath -Encoding UTF8
    Write-Host ('PASS: staged and Git-blob-verified ' + $records.Count + ' Streamline headers from 2.14.1.')
} catch { Write-Error $_; exit 1 }
