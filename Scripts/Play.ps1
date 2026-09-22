[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$JunctionPath = 'C:\UEProjects\LockdownZoneRepo',
    [switch]$QA
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

$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
    throw "UE editor not found: $editor. Supply -EngineRoot with your Unreal Engine installation."
}
$projectDirectory = Get-LZProjectDirectory
$projectFile = Join-Path $projectDirectory 'LockdownZone.uproject'
$module = Join-Path $projectDirectory 'Binaries\Win64\UnrealEditor-LockdownZone.dll'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) {
    throw 'Development Editor module not found. Run Scripts/Build.ps1 before playing.'
}
$logDirectory = Join-Path $projectDirectory 'Saved\Logs'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$playLog = Join-Path $logDirectory "Play-$stamp.log"
$gameArguments = @(
    $projectFile, '-game', '-windowed', '-ResX=1280', '-ResY=720',
    '-ForceRes', '-NoSplash', "-abslog=$playLog"
)
$windowStyle = 'Normal'
if ($QA) {
    $gameArguments += @('-LZSliceQA', '-LZQAExit', '-unattended')
    $windowStyle = 'Hidden'
}
# Start-Process joins ArgumentList into one native command line. Quote every
# argument explicitly so custom engine, log, and project paths retain spaces.
$nativeArguments = ($gameArguments | ForEach-Object { '"' + $_ + '"' }) -join ' '
Write-Host "Project: $projectFile"
Write-Host "Run log: $playLog"
if ($QA) {
    Write-Host ("QA report: " + (Join-Path $projectDirectory 'Saved\LZSliceQA_Report.txt'))
    Write-Host ("Screenshots: " + (Join-Path $projectDirectory 'Saved\Screenshots\WindowsEditor\SliceQA_*.png'))
}
$gameProcess = Start-Process -FilePath $editor -ArgumentList $nativeArguments -WorkingDirectory $projectDirectory -WindowStyle $windowStyle -Wait -PassThru
exit $gameProcess.ExitCode
