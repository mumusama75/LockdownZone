#include "LZMotionProfile.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Engine/SkeletalMesh.h"
void ULZMotionProfile::BuildCustomLocomotion(UAnimSequence* Idle,UAnimSequence* Walk,UAnimSequence* Run)
{
#if WITH_EDITOR
    if(!Locomotion || !Idle || !Walk || !Run) return;
    Locomotion->AddSample(Idle,FVector(0,0,0));
    Locomotion->AddSample(Walk,FVector(100,0,0));
    Locomotion->AddSample(Run,FVector(210,0,0));
    Locomotion->ValidateSampleData();Locomotion->ResampleData();Locomotion->MarkPackageDirty();
#endif
}
void ULZMotionProfile::InitializeTemplateFallback()
{
    Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    Locomotion=LoadObject<UBlendSpace>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run"));
    Stagger=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
    Knockdown=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
    Death=Knockdown;
    // Never label a reversed death clip as a GASP get-up or use a jump as a vault.
    // Missing authored clips use a pose blend / capsule traversal until imported.
}

void ULZMotionProfile::BuildLocomotionSamples()
{
#if WITH_EDITOR
    if(!Locomotion) return;
    const TCHAR* Clips[]={TEXT("/Game/Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop"),TEXT("/Game/Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F"),TEXT("/Game/Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F")};
    const float Speeds[]={0,150,450};
    for(int32 I=0;I<3;++I) Locomotion->AddSample(LoadObject<UAnimSequence>(nullptr,Clips[I]),FVector(Speeds[I],0,0));
    Locomotion->ValidateSampleData(); Locomotion->ResampleData(); Locomotion->MarkPackageDirty();
#endif
}
