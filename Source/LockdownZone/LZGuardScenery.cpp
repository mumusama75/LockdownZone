#include "LZGuardScenery.h"
#include "LZGameMode.h"
#include "LZWeaponPickup.h"
#include "LZLoot.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

LZGuardScenery::FGuard LZGuardScenery::Spawn(ALZGameMode* GM,FVector Center,FName Tag)
{
 auto* World=GM->GetWorld();
 auto Box=[&](const TCHAR* Name,FVector P,FVector S,FLinearColor Color){auto* A=GM->SpawnBlock(Name,P,S,FRotator::ZeroRotator,Color);A->SetActorEnableCollision(false);return A;};
 // Keep the corpse as scenery, outside enemy AI/kill counting; freeze an authored death pose.
 auto* Body=World->SpawnActor<AActor>();Body->Tags.Add(Tag);
 auto* Mesh=NewObject<USkeletalMeshComponent>(Body);Body->SetRootComponent(Mesh);Body->AddInstanceComponent(Mesh);
 Mesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);Mesh->RegisterComponent();
 Mesh->SetWorldLocation(FVector(340,-1470,0));Mesh->SetWorldRotation(FRotator(90,90,0));
 auto* Death=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"));
 if(Death){Mesh->PlayAnimation(Death,false);Mesh->SetPosition(Death->GetPlayLength(),false);Mesh->TickAnimation(0,false);Mesh->RefreshBoneTransforms();Mesh->bPauseAnims=true;}
 Mesh->UpdateBounds();
 const FBox PoseBounds=Mesh->Bounds.GetBox();
 Mesh->AddWorldOffset(FVector(Center.X-PoseBounds.GetCenter().X,Center.Y-PoseBounds.GetCenter().Y,2-PoseBounds.Min.Z));
 Mesh->SetComponentTickEnabled(false);
 auto* Uniform=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/Signage/M_GuardUniform.M_GuardUniform")),Body);
 Uniform->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.025f,.045f,.10f));
 Uniform->SetVectorParameterValue(TEXT("Color"),FLinearColor(.025f,.045f,.10f));
 for(int I=0;I<Mesh->GetNumMaterials();++I)Mesh->SetMaterial(I,Uniform);
 // A visible SECURITY patch identifies the uniform even in the emergency pool of light.
 const FVector Chest=Mesh->GetBoneLocation(TEXT("spine_03"));
 Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Mesh->SetCollisionResponseToAllChannels(ECR_Block);
 FHitResult BadgeHit;FCollisionQueryParams BadgeQuery(SCENE_QUERY_STAT(GuardBadge),true);
 const bool BadgeSurface=Mesh->LineTraceComponent(BadgeHit,Chest+FVector(0,0,100),Chest-FVector(0,0,100),BadgeQuery);
 const FVector BadgePosition=BadgeSurface?BadgeHit.ImpactPoint+BadgeHit.ImpactNormal*.4f:Chest+FVector(0,0,20);
 const FRotator BadgeRotation=BadgeSurface?FRotationMatrix::MakeFromZ(BadgeHit.ImpactNormal).Rotator():FRotator::ZeroRotator;
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 UE_LOG(LogTemp,Display,TEXT("PANTRY_BADGE surface=%d position=%s"),BadgeSurface,*BadgePosition.ToString());
 auto* Patch=GM->SpawnArtMesh(TEXT("GuardUniformPatch"),TEXT("/Engine/BasicShapes/Plane.Plane"),BadgePosition,BadgeRotation,FVector(.35f,.126f,1),false);
 if(Patch)Patch->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/Signage/M_GuardPatch.M_GuardPatch")));
 auto* Cap=GM->SpawnArtMesh(TEXT("GuardCap"),TEXT("/Engine/BasicShapes/Sphere.Sphere"),Mesh->GetBoneLocation(TEXT("head"))+FVector(0,0,9),FRotator::ZeroRotator,FVector(.24f,.26f,.12f),false);
 if(Cap)Cap->GetStaticMeshComponent()->SetMaterial(0,Uniform);
 Box(TEXT("GuardCapVisor"),Mesh->GetBoneLocation(TEXT("head"))+FVector(0,12,6),FVector(22,16,2),FLinearColor(.018f,.025f,.05f));
 UE_LOG(LogTemp,Display,TEXT("PANTRY_BONES head=%s pelvis=%s foot=%s"),*Mesh->GetBoneLocation(TEXT("head")).ToString(),*Mesh->GetBoneLocation(TEXT("pelvis")).ToString(),*Mesh->GetBoneLocation(TEXT("foot_l")).ToString());

 return {Body,BadgePosition};
}
