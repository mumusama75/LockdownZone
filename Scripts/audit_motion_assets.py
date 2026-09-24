import unreal,json
paths=['/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run','/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd','/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd','/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01','/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01']
for path in paths:
 a=unreal.load_asset(path)
 if isinstance(a,unreal.BlendSpace):
  unreal.log('LZ_ANIM '+path+' params='+str(a.get_editor_property('blend_parameters'))+' samples='+str(a.get_editor_property('sample_data')))
 elif a:unreal.log('LZ_ANIM '+path+' length='+str(a.get_editor_property('sequence_length'))+' skeleton='+str(a.get_editor_property('skeleton')))
