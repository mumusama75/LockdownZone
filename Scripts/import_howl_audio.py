import unreal
t=unreal.AssetImportTask();t.filename='C:/UEProjects/LockdownZoneRepo/SourceAssets/ChapterAudio/Howl.wav';t.destination_path='/Game/Gameplay/ChapterAudio';t.automated=True;t.save=True;t.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
