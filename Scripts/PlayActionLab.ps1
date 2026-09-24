param([switch]$QA)
$ErrorActionPreference='Stop'
$root='C:/UEProjects/LockdownZoneRepo'
$started=[DateTime]::UtcNow
$argsNative='"'+$root+'/LockdownZone.uproject" /Game/Tests/L_Chapter1AnimationLab -game -windowed -ResX=1600 -ResY=900 -ForceRes -NoSplash -abslog="'+$root+'/Saved/Logs/ActionLab.log"'
if($QA){$argsNative+=' -LZActionSamplesQA -unattended'}
$p=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsNative -WorkingDirectory $root -WindowStyle Hidden -PassThru
if($QA){$p.WaitForExit();$r=Get-Item "$root/Saved/LZActionQA_Report.txt" -ErrorAction SilentlyContinue;if(!$r -or $r.LastWriteTimeUtc -lt $started -or (Get-Content $r.FullName -Raw) -notmatch 'ACTION_QA assertions=\d+ failures=0'){exit 1};exit $p.ExitCode}
