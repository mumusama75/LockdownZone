import unreal
p='/Game/Gameplay/Signage'
t=unreal.AssetImportTask();t.filename='C:/UEProjects/LockdownZoneRepo/SourceAssets/Signage/T_LiftFuseNote.png';t.destination_path=p;t.automated=True;t.save=True;t.replace_existing=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
lib=unreal.EditorAssetLibrary; edit=unreal.MaterialEditingLibrary
mat=lib.load_asset(p+'/M_LiftFuseNote') if lib.does_asset_exist(p+'/M_LiftFuseNote') else None
if not mat:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_LiftFuseNote',p,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(mat)
tex=edit.create_material_expression(mat,unreal.MaterialExpressionTextureSample)
tex.texture=lib.load_asset(p+'/T_LiftFuseNote')
edit.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
r=edit.create_material_expression(mat,unreal.MaterialExpressionConstant);r.r=1
edit.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
mat.set_editor_property('two_sided',True)
edit.recompile_material(mat);lib.save_loaded_asset(mat,False)
unreal.log('LIFT_NOTE_IMPORT_OK')
