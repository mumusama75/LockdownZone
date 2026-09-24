import unreal
base='/Game/Characters/Zombie7Sample/'
a=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.MaterialEditingLibrary
materials=[]
for part in ['Body','Bottom']:
 name='M_Zombie7'+part
 m=unreal.load_asset(base+name) or a.create_asset(name,base.rstrip('/'),unreal.Material,unreal.MaterialFactoryNew())
 lib.delete_all_material_expressions(m)
 m.set_editor_property('two_sided',True)
 m.set_editor_property('used_with_skeletal_mesh',True)
 for suffix,prop,y in [('diffuse',unreal.MaterialProperty.MP_BASE_COLOR,0),('normal',unreal.MaterialProperty.MP_NORMAL,200)]:
  t=unreal.load_asset(base+'ZOM_7_'+part+'_'+suffix);assert t
  e=lib.create_material_expression(m,unreal.MaterialExpressionTextureSample,-400,y);e.set_editor_property('texture',t)
  if suffix=='normal':e.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
  lib.connect_material_property(e,'RGB',prop)
 r=lib.create_material_expression(m,unreal.MaterialExpressionConstant,-300,400);r.set_editor_property('r',.78);lib.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
 lib.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials.append(m)
mesh=unreal.load_asset(base+'Zombie7');slots=mesh.get_editor_property('materials')
for i,s in enumerate(slots):
 unreal.log('Z7_SLOT '+str(i)+' '+str(s.material_slot_name)+' '+str(s.material_interface))
 s.set_editor_property('material_interface',materials[1 if 'bottom' in str(s.material_slot_name).lower() else 0])
 slots[i]=s
mesh.set_editor_property('materials',slots);unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
p=unreal.load_asset('/Game/Gameplay/DA_Zombie7');p.set_editor_property('mesh_rotation',unreal.Rotator(pitch=0,yaw=-90,roll=0));unreal.EditorAssetLibrary.save_loaded_asset(p)
