import unreal
path='/Game/Tests/L_Chapter1AnimationLab'
# Dedicated runtime fixture world; never open or write ArtTrials / external set-dressing projects.
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    if not unreal.EditorLevelLibrary.new_level(path):
        raise RuntimeError('Could not create animation test map')
else:
    unreal.EditorLevelLibrary.load_level(path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/LockdownZone.LZGameMode'))
unreal.EditorLevelLibrary.save_current_level()
unreal.log('ACTION_LAB_SAVED '+path)
for path in ['/Game/Characters','/Game/Weapons']:
    for asset in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(path,True):
        if str(asset.asset_class_path.asset_name) in ['SkeletalMesh','AnimSequence']:
            unreal.log('ACTION_RESOURCE '+str(asset.package_name)+' '+str(asset.asset_class_path.asset_name))
