import unreal
p='/Game/Gameplay/Signage'
t=unreal.AssetImportTask();t.filename='C:/UEProjects/LockdownZoneRepo/SourceAssets/Signage/T_PantryMedical.png';t.destination_path=p;t.automated=True;t.save=True;t.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
lib=unreal.EditorAssetLibrary; edit=unreal.MaterialEditingLibrary
mat=lib.load_asset(p+'/M_PantryMedical') if lib.does_asset_exist(p+'/M_PantryMedical') else None
if not mat:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_PantryMedical',p,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
tex=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample)
tex.texture=lib.load_asset(p+'/T_PantryMedical')
edit.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
r=edit.create_material_expression(mat,unreal.MaterialExpressionConstant);r.r=1
edit.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property('two_sided',True)
edit.recompile_material(mat);lib.save_loaded_asset(mat,False)
unreal.log('PANTRY_SIGN_IMPORT_OK')

import unreal
p='/Game/Gameplay/Signage'
t=unreal.AssetImportTask();t.filename='C:/UEProjects/LockdownZoneRepo/SourceAssets/Signage/T_GuardPatch.png';t.destination_path=p;t.automated=True;t.save=True;t.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
lib=unreal.EditorAssetLibrary; edit=unreal.MaterialEditingLibrary
mat=lib.load_asset(p+'/M_GuardPatch') if lib.does_asset_exist(p+'/M_GuardPatch') else None
if not mat:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_GuardPatch',p,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
tex=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample)
tex.texture=lib.load_asset(p+'/T_GuardPatch')
edit.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
r=edit.create_material_expression(mat,unreal.MaterialExpressionConstant);r.r=1
edit.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property('two_sided',True)
edit.recompile_material(mat);lib.save_loaded_asset(mat,False)
unreal.log('PANTRY_SIGN_IMPORT_OK')

# Dedicated cloth material supports skeletal meshes without changing shared wall/prop materials.
name='M_GuardUniform'
mat=lib.load_asset(p+'/'+name) if lib.does_asset_exist(p+'/'+name) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,p,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
c=edit.create_material_expression(mat,unreal.MaterialExpressionVectorParameter)
c.set_editor_property('parameter_name','Color');c.set_editor_property('default_value',unreal.LinearColor(.025,.045,.10,1))
edit.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
r=edit.create_material_expression(mat,unreal.MaterialExpressionConstant);r.r=.95
edit.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property('used_with_skeletal_mesh',True)
edit.recompile_material(mat);lib.save_loaded_asset(mat,False)
