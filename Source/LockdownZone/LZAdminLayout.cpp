#include "LZChapter.h"
#include "LZAccessDoor.h"
#include "LZIntercom.h"
#include "LZStealthSettings.h"
#include "LZGuardScenery.h"
#include "LZGameMode.h"
#include "LZEnemy.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"

void ALZChapter::BuildAdminStealthRoute()
{
 AdminDoor=GetWorld()->SpawnActor<ALZAccessDoor>(FVector(300,900,0),FRotator::ZeroRotator);
 GM->SpawnBlock(TEXT("AdminDutyDesk"),FVector(700,1260,38),FVector(140,70,76),FRotator::ZeroRotator,FLinearColor(.16f,.12f,.08f));
 AdminRadio=GetWorld()->SpawnActor<ALZIntercom>(FVector(700,1240,92),FRotator::ZeroRotator);
 auto* TaskLamp=GetWorld()->SpawnActor<APointLight>(FVector(700,1200,225),FRotator::ZeroRotator);
 auto* TaskLight=Cast<UPointLightComponent>(TaskLamp->GetLightComponent());TaskLight->SetIntensity(35);TaskLight->SetAttenuationRadius(230);TaskLight->SetLightColor(FLinearColor(.7f,.83f,.75f));TaskLight->SetCastShadows(true);
 GM->SpawnBlock(TEXT("DutyDeskLight"),FVector(700,1324,235),FVector(45,10,12),FRotator::ZeroRotator,FLinearColor(.25f,.30f,.27f))->SetActorEnableCollision(false);
 for(int I=0;I<4;++I)GM->SpawnBlock(TEXT("RadioSpeakerSlot"),FVector(693+I*4,1229,92),FVector(2,1,14),FRotator::ZeroRotator,FLinearColor(.008f,.01f,.009f))->SetActorEnableCollision(false);
 GM->SpawnBlock(TEXT("RadioAntenna"),FVector(707,1240,119),FVector(2,2,32),FRotator::ZeroRotator,FLinearColor(.04f,.05f,.04f))->SetActorEnableCollision(false);
 const auto Guard=LZGuardScenery::Spawn(GM,FVector(700,610,0),TEXT("LZAdminGuardCorpse"));
 AdminCard=GetWorld()->SpawnActor<ALZAccessCard>(FVector(747,635,49),FRotator(0,-20,0));
 auto* Lanyard=GM->SpawnBlock(TEXT("GuardCardLanyard"),FVector(729,623,47),FVector(42,2,1),FRotator(0,32,0),FLinearColor(.5f,.34f,.055f));Lanyard->SetActorEnableCollision(false);
 auto* CardPrint=GM->SpawnBlock(TEXT("AccessCardStripe"),FVector(747,635,50),FVector(8,2,.3f),FRotator(0,-20,0),FLinearColor(.06f,.24f,.30f));CardPrint->SetActorEnableCollision(false);CardPrint->AttachToActor(AdminCard,FAttachmentTransformRules::KeepWorldTransform);
 // Lamp lights the uniform/lanyard locally, never adds a through-wall marker.
 auto* Lamp=GetWorld()->SpawnActor<APointLight>(FVector(760,650,240),FRotator::ZeroRotator);
 auto* L=Cast<UPointLightComponent>(Lamp->GetLightComponent());L->SetIntensity(60);L->SetAttenuationRadius(390);L->SetLightColor(FLinearColor(.78f,.75f,.59f));L->SetCastShadows(true);
 GM->SpawnBlock(TEXT("GuardLocalLamp"),FVector(760,650,339),FVector(45,18,8),FRotator::ZeroRotator,FLinearColor(.5f,.5f,.4f))->SetActorEnableCollision(false);
 // East-side observation and approach avoid the crowd's northwest investigation route.
 GM->SpawnBlock(TEXT("ObservationLowCabinet"),FVector(1090,120,45),FVector(140,65,90),FRotator::ZeroRotator,FLinearColor(.18f,.2f,.19f));
 for(int I=0;I<3;++I)
 {
  auto* Bottle=Add(ELZChapterNode::Throwable,FVector(1050+I*35,120,100),FVector(6,6,20),FColor(90,125,100));
  Bottle->Tags.Add(TEXT("LZSpareBottle"));Bottle->SetGlow(0);
  auto* M=Bottle->FindComponentByClass<UStaticMeshComponent>();M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 }
 // Ordinary workstations frame an accessible patch of floor, not a trigger volume.
 GM->SpawnBlock(TEXT("LureLowOfficePartition"),FVector(-970,-220,60),FVector(220,15,120),FRotator::ZeroRotator,FLinearColor(.13f,.19f,.18f));
 GM->SpawnBlock(TEXT("LureFileCabinet"),FVector(-1040,180,50),FVector(75,80,100),FRotator::ZeroRotator,FLinearColor(.14f,.15f,.15f));
 auto* Landing=GetWorld()->SpawnActor<AActor>(FVector(-600,0,5),FRotator::ZeroRotator);Landing->Tags.Add(TEXT("LZSuggestedBottleLanding"));
 const FVector Starts[]={FVector(-40,620,90),FVector(180,680,90),FVector(400,580,90),FVector(-210,380,90),FVector(180,360,90),FVector(440,370,90)};
 const auto* T=GetDefault<ULZStealthSettings>();
 for(int I=0;I<FMath::Clamp(T->CrowdCount,0,UE_ARRAY_COUNT(Starts));++I)
 {
  auto* E=GetWorld()->SpawnActor<ALZEnemy>(Starts[I],FRotator(0,90,0));E->Configure(EEnemyType::Infected);E->SearchDuration=T->SearchSeconds;E->Tags.Add(TEXT("LZDoorBCrowd"));
  if(I==2) E->Tags.Add(TEXT("LZMotionQAAnchor"));
 }
}
