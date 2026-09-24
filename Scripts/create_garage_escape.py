import unreal
path='/Game/Chapter2Greybox/Maps/L_GarageEscape'
ed=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(path):
    ed.load_level(path)
else:
    ed.new_level(path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/LockdownZone.LZGameMode'))
ed.save_current_level()
unreal.log('GARAGE_MAP_SAVED '+path)
