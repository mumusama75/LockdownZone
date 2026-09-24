import unreal
from pathlib import Path
paths=list(Path('C:/UEProjects/LockdownZoneRepo/SourceAssets/ChapterAudio').glob('*.wav'))
tasks=[]
for p in paths:
 t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/Gameplay/ChapterAudio';t.automated=True;t.save=True;t.replace_existing=False;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
unreal.log('CHAPTER_AUDIO_IMPORTED '+str(len(tasks)))
