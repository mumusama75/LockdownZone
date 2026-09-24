#include "LZChapter.h"
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

void ALZChapter::BuildDarkPantry()
{
 // A permanently dark optional supply room. The only light inside is a narrow emergency beam.
 for(TActorIterator<APointLight> It(GetWorld());It;++It)
 {
  const FVector V=It->GetActorLocation();
  if(V.X>-650 && V.X<1150 && V.Y<-1200 && V.Z<400)It->GetLightComponent()->SetIntensity(0);
  It->GetLightComponent()->SetCastShadows(true);
 }
 auto Box=[&](const TCHAR* Name,FVector P,FVector S,FLinearColor Color,FRotator R=FRotator::ZeroRotator)
 {
  auto* A=GM->SpawnBlock(Name,P,S,R,Color);A->SetActorEnableCollision(false);return A;
 };
 auto Spot=[&](const TCHAR* Name,FVector P,FVector Target,float Intensity,float Radius,float Cone)
 {
  auto* A=GetWorld()->SpawnActor<AActor>();A->Tags.Add(Name);
  auto* L=NewObject<USpotLightComponent>(A);A->SetRootComponent(L);A->AddInstanceComponent(L);L->RegisterComponent();
  L->SetWorldLocation(P);L->SetWorldRotation((Target-P).Rotation());L->SetIntensityUnits(ELightUnits::Candelas);
  L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetInnerConeAngle(Cone*.65f);L->SetOuterConeAngle(Cone);
  L->SetLightColor(FLinearColor(.72f,.84f,.78f));L->SetCastShadows(true);L->SetIndirectLightingIntensity(0);
 };
 Box(TEXT("PantryDoorLamp"),FVector(300,-1120,332),FVector(65,20,10),FLinearColor(.55f,.60f,.56f));
 Spot(TEXT("LZPantryDoorLight"),FVector(300,-1080,305),FVector(300,-1190,80),1.2f,460,55);
 Box(TEXT("PantryEmergencyLamp"),FVector(510,-1450,325),FVector(22,18,10),FLinearColor(.30f,.35f,.32f));
 Spot(TEXT("LZPantryEmergencyLight"),FVector(510,-1450,312),FVector(340,-1460,35),.65f,390,25);
 // Small uneven paired smears lead across D13 and stop beside the fallen guard.
 for(int I=0;I<10;++I)
 {
  const float Y=-1060-I*43.f, X=305+FMath::Sin(I*1.7f)*16;
  for(int Side:{-1,1})
   {
    auto* Smear=GM->SpawnArtMesh(TEXT("PantryDragBlood"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(X+Side*17,Y,.35f),FRotator(0,(I%3-1)*9,0),FVector((8+(I%3)*4)/100.f,(29+(I%2)*14)/100.f,.003f),false);
    auto* Blood=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel")),Smear);
    Blood->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.045f,.002f,.003f));Smear->GetStaticMeshComponent()->SetMaterial(0,Blood);
   }
 }
 auto* Pool=GM->SpawnArtMesh(TEXT("GuardBloodPool"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),FVector(340,-1470,.45f),FRotator::ZeroRotator,FVector(.85f,.60f,.005f),false);
 if(Pool)
 {
  auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel")),Pool);
  M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.045f,.002f,.003f));Pool->GetStaticMeshComponent()->SetMaterial(0,M);
 }
 auto* Sign=GM->SpawnArtMesh(TEXT("PantryMedicalSign"),TEXT("/Engine/BasicShapes/Plane.Plane"),FVector(510,-1168,205),FRotator(0,0,90),FVector(1.20f,.48f,1),false);
 if(Sign){Sign->Tags.Add(TEXT("LZPantryMedicalSign"));Sign->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/Signage/M_PantryMedical.M_PantryMedical")));}
 LZGuardScenery::Spawn(GM,FVector(340,-1470,0),TEXT("LZPantryGuardCorpse"));
 auto* Gun=GetWorld()->SpawnActor<ALZWeaponPickup>(FVector(920,-1870,12),FRotator(0,25,0));Gun->Configure(EPlayerWeapon::Firearm);Gun->Tags.Add(TEXT("LZPantryPistol"));
 auto* Kit=GetWorld()->SpawnActor<ALZLoot>(FVector(1040,-1870,9),FRotator(0,12,0));Kit->Configure(ELootType::Medical);Kit->Tags.Add(TEXT("LZPantryMedkit"));
 Gun->bRequiresFlashlight=true;Kit->bRequiresFlashlight=true;
 GM->SpawnBlock(TEXT("PantryCounterReturn"),FVector(840,-1850,45),FVector(35,240,90),FRotator::ZeroRotator,FLinearColor(.13f,.16f,.15f));
 // Opaque matte surfaces only: even the medical cross has no emissive material.
 for(AActor* A:{static_cast<AActor*>(Gun),static_cast<AActor*>(Kit)})
 {
  TArray<UStaticMeshComponent*> Parts;A->GetComponents(Parts);
  for(auto* Part:Parts)
  {
   auto* Matte=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/Signage/M_GuardUniform.M_GuardUniform")),A);
   const bool Cross=Part->GetName()==TEXT("LootDetail0") || Part->GetName()==TEXT("LootDetail1");
   Matte->SetVectorParameterValue(TEXT("Color"),Cross?FLinearColor(.5f,.52f,.48f):A==Gun?FLinearColor(.035f,.04f,.045f):FLinearColor(.025f,.10f,.065f));
   for(int I=0;I<Part->GetNumMaterials();++I)Part->SetMaterial(I,Matte);
  }
 }
 // Loot does not flood the dark room with the standard pickup glow.
 for(AActor* A:{static_cast<AActor*>(Gun),static_cast<AActor*>(Kit)})
 {
  TArray<UPointLightComponent*> Lights;A->GetComponents(Lights);for(auto* L:Lights)L->SetIntensity(0);
  TArray<UTextRenderComponent*> Labels;A->GetComponents(Labels);for(auto* L:Labels)L->SetVisibility(false);
 }

}
