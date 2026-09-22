[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$JunctionPath = 'C:\UEProjects\LockdownZoneRepo'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-LZProjectDirectory {
    $source = Get-Item -LiteralPath (Split-Path -Parent $PSScriptRoot)
    $projectRoot = $source.FullName
    if ($source.LinkType -eq 'Junction') {
        $projectRoot = [string]@($source.Target)[0]
    }
    $projectRoot = [IO.Path]::GetFullPath($projectRoot).TrimEnd('\')
    if ($projectRoot -eq 'C:\UEProjects\LockdownZone') {
        throw 'This is the historical development copy. Run the script in the repository project.'
    }
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot 'LockdownZone.uproject') -PathType Leaf)) {
        throw "Project not found next to Scripts: $projectRoot"
    }
    if ($projectRoot -notmatch '[^\x00-\x7F]') {
        return $projectRoot
    }

    $aliasPath = [IO.Path]::GetFullPath($JunctionPath).TrimEnd('\')
    if ($aliasPath -match '[^\x00-\x7F]') {
        throw 'JunctionPath must contain ASCII characters only.'
    }
    if ($aliasPath -eq 'C:\UEProjects\LockdownZone') {
        throw 'The historical C:\UEProjects\LockdownZone path is reserved. Choose another JunctionPath.'
    }
    $existing = Get-Item -LiteralPath $aliasPath -Force -ErrorAction SilentlyContinue
    if ($null -ne $existing) {
        if ($existing.LinkType -ne 'Junction') {
            throw "ASCII project path already exists and is not a junction: $aliasPath"
        }
        $target = [string]@($existing.Target)[0]
        if (-not [IO.Path]::IsPathRooted($target)) {
            $target = Join-Path (Split-Path -Parent $aliasPath) $target
        }
        $target = [IO.Path]::GetFullPath($target).TrimEnd('\')
        if ($target -ne $projectRoot) {
            throw "Junction points to a different project: $aliasPath -> $target; expected $projectRoot. No files were changed."
        }
    }
    else {
        $parent = Split-Path -Parent $aliasPath
        if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
            New-Item -ItemType Directory -Path $parent | Out-Null
        }
        New-Item -ItemType Junction -Path $aliasPath -Target $projectRoot | Out-Null
    }
    return $aliasPath
}

$buildBatch = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
if (-not (Test-Path -LiteralPath $buildBatch -PathType Leaf)) {
    throw "UE build script not found: $buildBatch. Supply -EngineRoot with your Unreal Engine installation."
}
$projectDirectory = Get-LZProjectDirectory
$projectFile = Join-Path $projectDirectory 'LockdownZone.uproject'
$logDirectory = Join-Path $projectDirectory 'Saved\Logs'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$buildLog = Join-Path $logDirectory "Build-$stamp.log"
$ubtLog = Join-Path $logDirectory "UBT-$stamp.log"
Write-Host "Project: $projectFile"
Write-Host "Build log: $buildLog"
& $buildBatch 'LockdownZoneEditor' 'Win64' 'Development' "-Project=$projectFile" '-WaitMutex' '-NoHotReloadFromIDE' "-Log=$ubtLog" 2>&1 |
    Tee-Object -FilePath $buildLog
$buildExitCode = $LASTEXITCODE
if ($buildExitCode -ne 0) {
    Write-Warning "Build failed with exit code $buildExitCode. See $buildLog"
}
exit $buildExitCode
