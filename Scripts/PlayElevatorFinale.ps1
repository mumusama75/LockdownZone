param([switch]$QA,[switch]$RetryCheck)
$ErrorActionPreference='Stop'
$root='C:/UEProjects/LockdownZoneRepo'
$mode=if($QA){'-LZElevatorQA -unattended -ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0'}else{'-LZElevatorTest'}
$argsNative='"'+$root+'/LockdownZone.uproject" -game -windowed -ResX=1280 -ResY=720 -ForceRes -NoSplash -ExecCmds="t.MaxFPS 60" '+$mode+' -abslog="'+$root+'/Saved/Logs/ElevatorFinale.log"'
if($RetryCheck){$argsNative+=' -LZElevatorRetryQA'}
$started=[DateTime]::UtcNow
$p=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsNative -WorkingDirectory $root -WindowStyle $(if($QA){'Hidden'}else{'Normal'}) -PassThru
if($QA){$p.WaitForExit();$r=Get-Item "$root/Saved/ElevatorFinaleQA.txt" -ErrorAction SilentlyContinue;if(!$r -or $r.LastWriteTimeUtc -lt $started -or (Get-Content $r.FullName -Raw) -match 'FAIL'){exit 1};exit $p.ExitCode}
