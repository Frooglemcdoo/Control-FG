#requires -Version 5.1
# Dot-source from the compact collector. Reads only the collected text logs.
function Write-ControlFGPerformanceSummary([string]$LogPath,[string]$OutputPath) {
    $frames=@{}; $gpu=New-Object 'System.Collections.Generic.List[object]'
    $notes=New-Object 'System.Collections.Generic.List[string]'
    $culture=[Globalization.CultureInfo]::InvariantCulture
    foreach ($line in [IO.File]::ReadLines($LogPath)) {
        if ($line -match ' RR_PERF_(DISABLED|GPU_INVALID|READY) ') { $notes.Add($line) }
        if ($line -notmatch ' RR_PERF_(FRAME|GPU) ') { continue }
        $kind=$Matches[1]; $row=@{}
        foreach ($match in [regex]::Matches($line,'([a-z_]+)=([^\s]+)')) { $row[$match.Groups[1].Value]=$match.Groups[2].Value }
        if (-not $row.ContainsKey('frame')) { continue }
        if ($kind -eq 'FRAME') { $frames[$row.frame]=$row } else { $gpu.Add($row) }
    }
    if (-not $frames.Count) { return $false }
    $groups=@{}
    foreach ($row in $frames.Values) {
        if ($row.eligible -ne '1' -or $row.mode -notin @('SR','RR')) { continue }
        $key=($row.segment,$row.mode,$row.width,$row.height,$row.output_width,$row.output_height,$row.fg,$row.hdr,$row.rt_effects)-join ':'
        if (-not $groups.ContainsKey($key)) {
            $groups[$key]=@{Key=$key;Row=$row;Frames=(New-Object 'System.Collections.Generic.List[object]');GPU=@{}}
        }
        $groups[$key].Frames.Add($row)
        $row['group']=$key
    }
    foreach ($sample in $gpu) {
        if ($sample.completed -ne '1' -or -not $frames.ContainsKey($sample.frame)) { continue }
        $frame=$frames[$sample.frame]
        if (-not $frame.ContainsKey('group') -or $frame.mode -cne $sample.planned -or
            $frame.width -ne $sample.width -or $frame.height -ne $sample.height) { continue }
        $group=$groups[$frame.group]
        if (-not $group.GPU.ContainsKey($sample.stage)) { $group.GPU[$sample.stage]=New-Object 'System.Collections.Generic.List[object]' }
        $group.GPU[$sample.stage].Add($sample)
    }
    function Measure-PerfValues($Rows,[string]$Field) {
        $unsorted=@(foreach ($r in $Rows) {
            [double]$value=0
            if ($r.ContainsKey($Field) -and [double]::TryParse($r[$Field],[Globalization.NumberStyles]::Float,$culture,[ref]$value) -and
                -not [double]::IsNaN($value) -and -not [double]::IsInfinity($value) -and $value -ge 0) { $value }
        })
        $values=@($unsorted | Sort-Object)
        if (-not $values.Count) { return $null }
        $mean=($values | Measure-Object -Average).Average
        return [ordered]@{Samples=$values.Count;MeanMs=[Math]::Round($mean,6);P95Ms=$values[[int][Math]::Ceiling($values.Count*0.95)-1];MinMs=$values[0];MaxMs=$values[-1]}
    }
    $segments=@(foreach ($key in ($groups.Keys | Sort-Object)) {
        $group=$groups[$key];$row=$group.Row;$cpu=[ordered]@{};$timings=[ordered]@{}
        foreach ($field in @('cadence_ms','guides_cpu_ms','eval_cpu_ms','replay_prepare_cpu_ms','replay_workers_cpu_ms','filter_cpu_ms')) {
            $cpu[$field]=Measure-PerfValues $group.Frames $field
        }
        foreach ($stage in ($group.GPU.Keys | Sort-Object)) { $timings[$stage]=Measure-PerfValues $group.GPU[$stage] 'ms' }
        [ordered]@{Segment=[long]$row.segment;Mode=$row.mode;Render=($row.width+'x'+$row.height);Output=($row.output_width+'x'+$row.output_height);
            FGSelection=[int]$row.fg;HDRBridge=[int]$row.hdr;RTEffects=$row.rt_effects;StableFrameSamples=$group.Frames.Count;CPU=$cpu;GPU=$timings}
    })
    [ordered]@{Schema='ControlFG.RRPerformance.v1';ProbeLog=[IO.Path]::GetFileName($LogPath);FrameRecords=$frames.Count;Segments=$segments;
        Notes=@($notes.ToArray());Limitations=@('Release profiling samples every 240th frame; warmup/reset/mode/extent/RT-settings/FG/HDR changes and rendering gaps excluded from eligible segments.',
            'GPU intervals are queue timestamp spans, not total frame GPU time. Native material replay GPU time is not separately measured.',
            'replay_workers_cpu_ms sums worker recording time, not elapsed frame time. CPU and GPU times must not be added.',
            'RR OFF retains guide warmup work. Compare the same scene and settings; logs cannot prove that the camera stayed fixed.',
            'Missing GPU stages mean unavailable samples, not zero cost. Truncated source logs can omit earlier segments.')} |
        ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
    return $true
}
