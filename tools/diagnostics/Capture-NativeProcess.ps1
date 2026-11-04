function Invoke-CaptureNative {
    param([Parameter(Mandatory=$true)][string]$Executable,
          [string[]]$Arguments=@(),[string]$LogPath='',[int]$TimeoutSeconds=120)
    $quoted = foreach ($argument in $Arguments) {
        if ($argument.Contains('"') -or $argument.EndsWith('\')) { throw 'Unsupported quote or trailing slash in native argument.' }
        '"' + $argument + '"'
    }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $Executable
    $info.Arguments = $quoted -join ' '
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    if ($LogPath) {
        @('BEGIN: ' + [DateTime]::UtcNow.ToString('o'),"COMMAND: $Executable $($info.Arguments)") | Add-Content -LiteralPath $LogPath -Encoding UTF8
        Write-Host "Running: $([IO.Path]::GetFileName($Executable)) $($Arguments -join ' ')"
    }
    $proc = New-Object Diagnostics.Process
    $proc.StartInfo = $info
    try {
        if (-not $proc.Start()) { throw "Could not start $Executable" }
        $proc.StandardInput.Close()
        $stdout = $proc.StandardOutput.ReadToEndAsync()
        $stderr = $proc.StandardError.ReadToEndAsync()
        $clock = [Diagnostics.Stopwatch]::StartNew()
        $nextProgress = 10
        while (-not $proc.WaitForExit(250)) {
            if ($clock.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                $proc.Kill()
                if ($LogPath) { "TIMEOUT: $TimeoutSeconds seconds" | Add-Content -LiteralPath $LogPath -Encoding UTF8 }
                throw "$Executable timed out after $TimeoutSeconds seconds."
            }
            if ($LogPath -and $clock.Elapsed.TotalSeconds -ge $nextProgress) {
                $progress = "Still running $($Arguments[0]): $([int]$clock.Elapsed.TotalSeconds)s (limit $($TimeoutSeconds)s)"
                Write-Host $progress
                $progress | Add-Content -LiteralPath $LogPath -Encoding UTF8
                $nextProgress += 10
            }
        }
        # WPR may spawn children inheriting output handles. Do not wait forever
        # for EOF after the command process itself has already exited.
        if (-not $stdout.Wait(5000) -or -not $stderr.Wait(5000)) {
            if ($LogPath) { "OUTPUT_TIMEOUT: process exited with $($proc.ExitCode), output handles remain open" | Add-Content -LiteralPath $LogPath -Encoding UTF8 }
            throw "$Executable exited with $($proc.ExitCode), but a child kept its output handles open. See the log and retain any ETL files."
        }
        $output = $stdout.GetAwaiter().GetResult()
        $errors = $stderr.GetAwaiter().GetResult()
        $result = [pscustomobject]@{ExitCode=$proc.ExitCode;Stdout=$output;Stderr=$errors;Text=($output + [Environment]::NewLine + $errors)}
        if ($LogPath) {
            @("COMMAND: $Executable $($info.Arguments)","EXIT: $($result.ExitCode)",'STDOUT:', $output,'STDERR:', $errors) | Add-Content -LiteralPath $LogPath -Encoding UTF8
        }
        return $result
    } finally { $proc.Dispose() }
}
