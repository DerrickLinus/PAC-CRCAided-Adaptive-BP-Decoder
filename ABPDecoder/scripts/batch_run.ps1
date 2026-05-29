<#
.SYNOPSIS
  ABPDecoder batch simulation runner.
.DESCRIPTION
  Reads a CSV parameter table, generates Profile.txt for each row,
  runs the simulation, and saves results to uniquely-named output files.
  Supports resume-by-deletion: remove completed rows from CSV and re-run.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$ConfigCsv,
    [string]$ExePath = "",
    [string]$ResultsDir = "Results",
    [switch]$ContinueOnError = $true                                                                                                                                                                                                                                                                                                                                                                                                                                                    
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path "$scriptDir\.."
$resultsDir = Join-Path $rootDir $ResultsDir
$logsDir = Join-Path $rootDir "logs"
if (-not (Test-Path $resultsDir)) { New-Item -ItemType Directory $resultsDir -Force | Out-Null }
if (-not (Test-Path $logsDir)) { New-Item -ItemType Directory $logsDir -Force | Out-Null }
$logFile = Join-Path $logsDir ("batch_run_{0:yyyyMMdd_HHmmss}.log" -f (Get-Date))

function Write-Log {
    param([string]$Message, [string]$Color = "White")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $line = "[$timestamp] $Message"
    Write-Host $line -ForegroundColor $Color
    Add-Content -Path $logFile -Value $line -Encoding UTF8
}

function Write-ProfileFile {
    param($Row)
    $profilePath = Join-Path $rootDir "Profile.txt"
    $content = @"
ABP Decoder Profile:

Codefile:			$($Row.Codefile)
N:					$($Row.N)
K:      			$($Row.K)
CRC:				$($Row.CRC_len)
Add Zeros:			$($Row.EncodeAdd0)
Puncture:			$($Row.Puncture)
Shorten:			$($Row.Shorten)
Decoding Method:	$($Row.DecodingMethod)

ABP Scheme:			$($Row.N1) $($Row.N2)
ABP Deg-2:			$($Row.Deg2)
ABP Interchange:	$($Row.Interchange)
ABP Use CRC:		$($Row.CRC_len_for_ABP)
ABP Use ChannelLLR: $($Row.UseChannelLLR)
Metric Threshold:   $($Row.ML_metric_th)
SCLD ListSize:      $($Row.ListSize)
PAC	SystemCode:     $($Row.SystemCode)

MS Type:            $($Row.ms_type)
NMS Alpha:          $($Row.Alpha)
OMS Beta:           $($Row.Beta)
NMS Alpha2:         $($Row.Alpha2)
OMS Beta2:          $($Row.Beta2)

Convergence Epsilon:$($Row.ConvEpsilon)
Convergence Window: $($Row.ConvWindow)

Damping Mode:       $($Row.DampMode)
Damping Fixed:      $($Row.DampFixed)
Damping Start:      $($Row.DampStart)
Damping End:        $($Row.DampEnd)
Damping P:          $($Row.DampP)

SNR Type:   $($Row.SNRtype)
Start SNR:  $($Row.StartSNR)
End SNR:    $($Row.EndSNR)
Step SNR:   $($Row.StepSNR)
Least Test Frame: $($Row.LeastTestFrame)
Least Error Frame: $($Row.LeastErrorFrame)
Random Codewords:	$($Row.SourceType)
Display Step:	    $($Row.DisplayStep)
"@
    [System.IO.File]::WriteAllText($profilePath, $content, [System.Text.UTF8Encoding]::new($false))
}

# ===== Main =====

# Auto-detect executable
if ($ExePath -eq "") {
    $candidates = @(
        "$rootDir\x64\Release\ABPDecoder.exe",              # VS MSBuild (exe inside project dir)
        "$rootDir\x64\Debug\ABPDecoder.exe",
        "$rootDir\out\build\x64-Release\ABPDecoder.exe",   # VS Studio CMake
        "$rootDir\out\build\x64-Debug\ABPDecoder.exe",
        "$rootDir\build\ABPDecoder.exe",                    # CMake (VS Code / MinGW)
        "$rootDir\build\Release\ABPDecoder.exe",
        "$rootDir\..\x64\Release\ABPDecoder.exe",           # VS MSBuild (exe above project dir)
        "$rootDir\..\x64\Debug\ABPDecoder.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $ExePath = $c; break }
    }
    # Fallback: recursive search from repo root (covers exe in parent dir, VS MSBuild quirk)
    if ($ExePath -eq "") {
        Write-Host "Scanning for ABPDecoder.exe..." -ForegroundColor Yellow
        $searchRoot = if (Test-Path "$rootDir\..\.git") { (Resolve-Path "$rootDir\..") } else { $rootDir }
        $found = Get-ChildItem -Path $searchRoot -Recurse -Filter "ABPDecoder.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { $ExePath = $found.FullName }
    }
    if ($ExePath -eq "") {
        Write-Error "Cannot find ABPDecoder.exe. Please build first."
        exit 1
    }
}
$exeFullPath = Resolve-Path $ExePath -ErrorAction SilentlyContinue
if (-not $exeFullPath) {
    Write-Error "Executable not found: $ExePath"
    exit 1
}

# Read CSV (skip header, comments, blank lines)
$csvPath = $ConfigCsv
if (-not (Test-Path $csvPath)) {
    $csvPath = Join-Path $scriptDir $ConfigCsv
}
if (-not (Test-Path $csvPath)) {
    Write-Error "CSV not found: $ConfigCsv"
    exit 1
}
$rawLines = Get-Content $csvPath -Encoding UTF8
$configs = @()
$skipHeader = $true
foreach ($line in $rawLines) {
    $t = $line.Trim()
    if ($t -eq "" -or $t.StartsWith("#")) { continue }
    if ($skipHeader) { $skipHeader = $false; continue }
    $configs += $line
}
if ($configs.Count -eq 0) {
    Write-Error "No valid data rows in CSV"
    exit 1
}

Write-Log "============================================"
Write-Log "  Batch start: $($configs.Count) config(s)"
Write-Log "  EXE : $exeFullPath"
Write-Log "  CSV : $csvPath"
Write-Log "  Continue on error: $ContinueOnError"
Write-Log "============================================"

Push-Location $rootDir
$succeeded = 0
$failed = 0

foreach ($line in $configs) {
    $fields = $line -split ","
    if ($fields.Count -lt 39) {
        Write-Log "SKIP: expected 39 fields, got $($fields.Count)" "Yellow"
        continue
    }

    $row = [PSCustomObject]@{
        RunID            = $fields[0].Trim()
        Label            = $fields[1].Trim()
        Codefile         = $fields[2].Trim()
        N                = $fields[3].Trim()
        K                = $fields[4].Trim()
        CRC_len          = $fields[5].Trim()
        EncodeAdd0       = $fields[6].Trim()
        Puncture         = $fields[7].Trim()
        Shorten          = $fields[8].Trim()
        DecodingMethod   = $fields[9].Trim()
        N1               = $fields[10].Trim()
        N2               = $fields[11].Trim()
        Deg2             = $fields[12].Trim()
        Interchange      = $fields[13].Trim()
        CRC_len_for_ABP  = $fields[14].Trim()
        UseChannelLLR    = $fields[15].Trim()
        ML_metric_th     = $fields[16].Trim()
        ListSize         = $fields[17].Trim()
        SystemCode       = $fields[18].Trim()
        ms_type          = $fields[19].Trim()
        Alpha            = $fields[20].Trim()
        Beta             = $fields[21].Trim()
        Alpha2           = $fields[22].Trim()
        Beta2            = $fields[23].Trim()
        ConvEpsilon      = $fields[24].Trim()
        ConvWindow       = $fields[25].Trim()
        DampMode         = $fields[26].Trim()
        DampFixed        = $fields[27].Trim()
        DampStart        = $fields[28].Trim()
        DampEnd          = $fields[29].Trim()
        DampP            = $fields[30].Trim()
        SNRtype          = $fields[31].Trim()
        StartSNR         = $fields[32].Trim()
        EndSNR           = $fields[33].Trim()
        StepSNR          = $fields[34].Trim()
        LeastTestFrame   = $fields[35].Trim()
        LeastErrorFrame  = $fields[36].Trim()
        SourceType       = $fields[37].Trim()
        DisplayStep      = $fields[38].Trim()
    }

    $runId = [int]$row.RunID
    $label = $row.Label
    $outFile = Join-Path $resultsDir ("Performance_R{0:D3}_{1}.txt" -f $runId, $label)
    Write-Log "[Run $runId] $label : STARTED"

    Write-ProfileFile $row

    $startTime = Get-Date
    $exitCode = 0
    try {
        & $exeFullPath $outFile
        $exitCode = $LASTEXITCODE
    } catch {
        $exitCode = -1
        Write-Log "[Run $runId] $label : process exception: $_" "Red"
    }
    $elapsed = ((Get-Date) - $startTime).TotalMinutes

    if ($exitCode -eq 0) {
        Write-Log "[Run $runId] $label : OK ($($elapsed.ToString('F1')) min)" "Green"
        $succeeded++
    } else {
        Write-Log "[Run $runId] $label : FAILED (exit=$exitCode, $($elapsed.ToString('F1')) min)" "Red"
        $failedDir = Join-Path $resultsDir "failed_configs"
        if (-not (Test-Path $failedDir)) { New-Item -ItemType Directory $failedDir -Force | Out-Null }
        $failedProfile = Join-Path $failedDir "Profile_R$($runId.ToString('D3'))_$label.txt"
        Copy-Item (Join-Path $rootDir "Profile.txt") $failedProfile
        Write-Log "[Run $runId] Config saved: failed_configs/Profile_R$($runId.ToString('D3'))_$label.txt"
        $failed++
        if (-not $ContinueOnError) {
            Write-Log "Stopped (-ContinueOnError:`$false)" "Red"
            break
        }
    }
}

Pop-Location

Write-Log "============================================"
Write-Log "  Done: $succeeded OK / $failed FAILED / $($configs.Count) total"
Write-Log "  Log : $logFile"
Write-Log "============================================"
