import unreal
pipe=unreal.InterchangeGenericAssetsPipeline()
pipe.set_editor_property('import_offset_uniform_scale',1.0)
pipe.set_editor_property('import_offset_rotation',unreal.Rotator(pitch=0,yaw=0,roll=0))
params=unreal.ImportAssetParameters();params.set_editor_property('is_automated',True);params.set_editor_property('replace_existing',True);params.set_editor_property('override_pipelines',[unreal.SoftObjectPath(pipe.get_path_name())])
m=unreal.InterchangeManager.get_interchange_manager_scripted()
source=m.create_source_data('C:/UEProjects/LockdownZoneRepo/SourceAssets/Zombie7/Zombie7.glb')
m.import_asset('/Game/Characters/Zombie7Sample',source,params)
unreal.EditorAssetLibrary.save_directory('/Game/Characters/Zombie7Sample')
