$ErrorActionPreference='Stop'
$root='C:/UEProjects/LockdownZoneRepo'
$started=[DateTime]::UtcNow
$argsNative='"'+$root+'/LockdownZone.uproject" -game -windowed -ResX=1600 -ResY=900 -ForceRes -NoSplash -LZOpeningQA -unattended -abslog="'+$root+'/Saved/Logs/OpeningQA.log"'
$p=Start-Process 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsNative -WorkingDirectory $root -WindowStyle Hidden -Wait -PassThru
$r=Get-Item "$root/Saved/LZOpeningQA_Report.txt" -ErrorAction SilentlyContinue
if(!$r -or $r.LastWriteTimeUtc -lt $started -or (Get-Content $r.FullName -Raw) -notmatch 'OPENING_QA assertions=\d+ failures=0'){exit 1}
exit $p.ExitCode
