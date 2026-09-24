import unreal,json
from pathlib import Path
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(True)
base='/Game/Characters/UEFN_Mannequin'
selected=[base+'/Meshes/SKM_UEFN_Mannequin',base+'/Animations/Idle/M_Neutral_Stand_Idle_Loop',base+'/Animations/Walk/M_Neutral_Walk_Loop_F',base+'/Animations/Run/M_Neutral_Run_Loop_F',base+'/Animations/Ragdoll/M_ragdoll_getup_stand_B',base+'/Animations/Traversal/Vault/M_Neutral_Traversal_Vault_1_0_stand_F_Lfoot']
selected += [base+'/Animations/Interactions/Shoves/M_relaxed_ragdoll_shove_stand_F_V']
seen=set();pending=list(selected)
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
while pending:
 p=pending.pop()
 if p in seen or not p.startswith('/Game/'):continue
 seen.add(p);pending.extend(str(x) for x in reg.get_dependencies(p,options))
root=Path(unreal.Paths.project_content_dir());size=sum((root/(p[6:]+'.uasset')).stat().st_size for p in seen if (root/(p[6:]+'.uasset')).exists())
Path('C:/UEProjects/LockdownZoneRepo/Saved/GASP-dependencies.json').write_text(json.dumps({'selected':selected,'dependencies':sorted(seen),'bytes':size},indent=2),encoding='utf-8')
unreal.log('LZ_GASP_DEPS '+str(len(seen))+' bytes '+str(size))
for p in selected:
 a=unreal.load_asset(p)
 unreal.log('LZ_GASP_ASSET '+p+' '+str(type(a)))
 if isinstance(a,unreal.AnimSequence):unreal.log('LZ_GASP_LENGTH '+str(a.get_editor_property('sequence_length')))
