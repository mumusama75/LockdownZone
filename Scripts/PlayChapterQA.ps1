$ErrorActionPreference='Stop'
$root='C:\UEProjects\LockdownZoneRepo'
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$log="$root\Saved\Logs\Chapter-$stamp.log"
$editor='C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$started=[DateTime]::UtcNow
$argsNative='"'+$root+'\LockdownZone.uproject" -game -windowed -ResX=1280 -ResY=720 -ForceRes -NoSplash -LZChapterQA -LZQAExit -unattended -abslog="'+$log+'"'
Write-Host "Chapter run: $log"
$p=Start-Process -FilePath $editor -ArgumentList $argsNative -WorkingDirectory $root -WindowStyle Hidden -Wait -PassThru
$r=Get-Item "$root\Saved\LZChapterQA_Report.txt" -ErrorAction SilentlyContinue
if(!$r -or $r.LastWriteTimeUtc -lt $started -or (Get-Content $r.FullName -Raw) -notmatch 'CHAPTER_QA SUMMARY PASS assertions=\d+ failures=0'){exit 1}
exit $p.ExitCode
