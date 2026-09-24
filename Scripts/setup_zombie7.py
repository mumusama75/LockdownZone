import unreal
base='/Game/Characters/Zombie7Sample/'
a=unreal.AssetToolsHelpers.get_asset_tools()
for old in ['/Game/Gameplay/DA_Zombie7','/Game/Gameplay/BS_Zombie7']:
 if unreal.EditorAssetLibrary.does_asset_exist(old):unreal.EditorAssetLibrary.delete_asset(old)
mesh=unreal.load_asset(base+'Zombie7');skel=mesh.get_editor_property('skeleton')
unreal.log('Z7_BOUNDS '+str(mesh.get_bounds()))
f=unreal.BlendSpaceFactoryNew();f.set_editor_property('target_skeleton',skel)
bs=unreal.load_asset('/Game/Gameplay/BS_Zombie7') or a.create_asset('BS_Zombie7','/Game/Gameplay',unreal.BlendSpace,f)
params=bs.get_editor_property('blend_parameters');params[0].set_editor_property('display_name','Speed');params[0].set_editor_property('min',0);params[0].set_editor_property('max',210);bs.set_editor_property('blend_parameters',params)
f=unreal.DataAssetFactory();f.set_editor_property('data_asset_class',unreal.LZMotionProfile)
p=unreal.load_asset('/Game/Gameplay/DA_Zombie7') or a.create_asset('DA_Zombie7','/Game/Gameplay',unreal.LZMotionProfile,f)
p.set_editor_property('source_label','Zombie Number 7 / Tony Flanagan / CC BY 4.0 / ordinary infected')
p.set_editor_property('mesh',mesh);p.set_editor_property('locomotion',bs);p.set_editor_property('speed_on_x_axis',True);p.set_editor_property('preserve_materials',True)
p.set_editor_property('physics_root','mixamorig_Hips');p.set_editor_property('mesh_rotation',unreal.Rotator(pitch=0,yaw=-90,roll=0))
p.set_editor_property('attack',unreal.load_asset(base+'Zombie7Attack'));p.set_editor_property('death',unreal.load_asset(base+'Zombie7Death'))
p.build_custom_locomotion(*[unreal.load_asset(base+'Zombie7'+c) for c in ['Idle','Walk','Run']])
for c in ['Idle','Walk','Run','Death','Attack']:
 clip=unreal.load_asset(base+'Zombie7'+c);unreal.log('Z7_CLIP '+c+' '+str(clip.get_editor_property('sequence_length')))
unreal.EditorAssetLibrary.save_asset('/Game/Gameplay/DA_Zombie7');unreal.EditorAssetLibrary.save_asset('/Game/Gameplay/BS_Zombie7')

