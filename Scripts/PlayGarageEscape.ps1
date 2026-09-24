param([switch]$QA,[switch]$Mechanics,[switch]$BeforeGate,[switch]$Guidance)
$ErrorActionPreference='Stop'
$root='C:/UEProjects/LockdownZoneRepo'
$started=[DateTime]::UtcNow
$argsNative='"'+$root+'/LockdownZone.uproject" /Game/Chapter2Greybox/Maps/L_GarageEscape -game -windowed -ResX=1600 -ResY=900 -ForceRes -NoSplash -ExecCmds="t.MaxFPS 60" -abslog="'+$root+'/Saved/Logs/GarageEscape.log"'
if($BeforeGate){$argsNative+=' -GarageBeforeGate'}
if($Guidance){$argsNative+=' -GarageGuidanceQA -unattended'}
if($Mechanics){$argsNative+=' -GarageMechanicsQA -unattended'}elseif($QA){$argsNative+=' -GarageEscapeQA -unattended'}
$p=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsNative -WorkingDirectory $root -WindowStyle $(if($QA -or $Mechanics -or $Guidance){"Hidden"}else{"Normal"}) -PassThru
if($Guidance){$p.WaitForExit();exit $p.ExitCode}
if($QA -or $Mechanics){
 $p.WaitForExit()
 $report=if($Mechanics){"$root/Saved/GarageMechanicsQA.txt"}else{"$root/Saved/GarageEscapeQA.txt"}
 $r=Get-Item $report -ErrorAction SilentlyContinue
 if(!$r -or $r.LastWriteTimeUtc -lt $started -or (Get-Content $r.FullName -Raw) -notmatch 'GARAGE_QA assertions=\d+ failures=0'){exit 1}
 exit $p.ExitCode
}
