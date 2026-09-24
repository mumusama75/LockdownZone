#include "LZOfficeOpening.h"
#include "LZChapter.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "LZOpeningSettings.h"
#include "LZCrowbarVisual.h"
#include "LZAcoustics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
ALZOfficeOpening::ALZOfficeOpening(){PrimaryActorTick.bCanEverTick=true;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("OpeningRoot"));}
void ALZOfficeOpening::Setup(ALZChapter* Chapter,ALZChapterNode* InDoor,ALZChapterNode* Tool,AActor* DoorLeaf)
{
 C=Chapter;Door=InDoor;auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 const auto* S=GetDefault<ULZOpeningSettings>();
 Leaf=Cast<AStaticMeshActor>(DoorLeaf);
 Closed=Leaf->GetActorLocation();ClosedRotation=Leaf->GetActorRotation();Hinge=Closed+FVector(0,-115,0);
 Leaf->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 Door->SetGlow(0);auto* Proxy=Door->FindComponentByClass<UStaticMeshComponent>();Proxy->SetVisibility(false);
 Proxy->SetCollisionResponseToAllChannels(ECR_Ignore);Proxy->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Proxy->SetCanEverAffectNavigation(false);
 // Keep the full bar silhouette above the worktop. The broad invisible query proxy is easy to target.
 Tool->SetGlow(0);Tool->SetActorLocation(S->RepairBench+FVector(0,0,9));Tool->SetActorScale3D(FVector(1));
 auto* ToolMesh=Tool->FindComponentByClass<UStaticMeshComponent>();ToolMesh->SetVisibility(false);ToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 auto* Query=NewObject<UBoxComponent>(Tool);Tool->AddInstanceComponent(Query);Query->SetupAttachment(Tool->GetRootComponent());Query->SetBoxExtent(FVector(43,16,8));Query->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Query->SetCollisionResponseToAllChannels(ECR_Ignore);Query->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Query->SetCanEverAffectNavigation(false);Query->RegisterComponent();
 auto* Bar=LZCrowbarVisual::Build(Tool,Tool->GetRootComponent());Bar->SetRelativeRotation(FRotator(90,12,0));
 PryBar=LZCrowbarVisual::Build(this,RootComponent);PryBar->SetWorldLocation(FVector(-2570,-501,93));PryBar->SetVisibility(false,true);
 const FLinearColor Steel(.21f,.25f,.26f);
 // A chair and interrupted paperwork sit behind the spawn, outside the direct exit route.
 GM->SpawnArtMesh(TEXT("OpeningAbandonedChair"),TEXT("/Game/Art/KenneyFurniture/SM_chairDesk.SM_chairDesk"),FVector(-3480,40,0),FRotator(0,18,0),FVector(.19f));
 for(int I=0;I<4;++I)GM->SpawnBlock(TEXT("OpeningScatteredPaper"),FVector(-3450+I*37,-100+I*19,1.8f),FVector(21,29.7f,.15f),FRotator(0,21+I*39,0),FLinearColor(.58f,.55f,.46f))->SetActorEnableCollision(false);
 GM->SpawnArtMesh(TEXT("OpeningAbandonedFiles"),TEXT("/Game/Art/KenneyFurniture/SM_books.SM_books"),FVector(-3330,-400,79),FRotator(0,12,0),FVector(.16f),false);
 GM->SpawnBlock(TEXT("OpeningRepairBenchTop"),S->RepairBench-FVector(0,0,3),FVector(125,66,6),FRotator::ZeroRotator,Steel);
 for(int X:{-1,1})for(int Y:{-1,1})GM->SpawnBlock(TEXT("OpeningRepairBenchLeg"),S->RepairBench+FVector(X*53,Y*24,-43),FVector(5,5,74),FRotator::ZeroRotator,Steel);
 // Parts stay off the crowbar and off the walking route.
 for(int I=0;I<3;++I)GM->SpawnBlock(TEXT("OpeningRepairPart"),S->RepairBench+FVector(-45+I*12,23,3),FVector(8,5,5),FRotator(0,I*17,0),Steel)->SetActorEnableCollision(false);
 GM->SpawnBlock(TEXT("OpeningLampBase"),S->RepairBench+FVector(48,19,3),FVector(18,18,6),FRotator::ZeroRotator,Steel)->SetActorEnableCollision(false);
 GM->SpawnBlock(TEXT("OpeningLampStem"),S->RepairBench+FVector(48,19,32),FVector(3,3,58),FRotator::ZeroRotator,Steel)->SetActorEnableCollision(false);
 GM->SpawnBlock(TEXT("OpeningLampHood"),S->RepairBench+FVector(42,19,61),FVector(28,16,8),FRotator(0,0,-12),FLinearColor(.65f,.46f,.16f))->SetActorEnableCollision(false);
 auto* Lamp=GetWorld()->SpawnActor<APointLight>(S->RepairBench+FVector(38,12,52),FRotator::ZeroRotator);
 auto* L=Cast<UPointLightComponent>(Lamp->GetLightComponent());L->SetIntensity(95);L->SetAttenuationRadius(380);L->SetLightColor(FLinearColor(1,.82f,.52f));L->SetCastShadows(true);
 GM->SpawnBlock(TEXT("BentOfficeDoorJamb"),FVector(-2532,-493,149),FVector(18,9,292),FRotator(0,0,1.4f),Steel)->SetActorEnableCollision(false);
 for(int I=0;I<4;++I)GM->SpawnBlock(TEXT("PryScoredMetal"),FVector(-2543,-499,107+I*6),FVector(2,22-I*3,1),FRotator(0,0,12),FLinearColor(.58f,.60f,.59f))->SetActorEnableCollision(false);
}
void ALZOfficeOpening::Pose(float Degrees)
{
 if(!IsValid(Leaf))return;const FRotator Turn(0,Degrees,0);Leaf->SetActorLocation(Hinge+Turn.RotateVector(Closed-Hinge));Leaf->SetActorRotation(ClosedRotation+Turn);
}
void ALZOfficeOpening::Interact(ALZCharacter* P)
{
 if(!P || bPrying || C->bDoorBreached || P->IsTraversing() || P->IsInventoryOpen() || GetWorld()->GetAuthGameMode<ALZGameMode>()->IsRunOver())return;
 if(FVector::Dist(P->GetActorLocation(),Door->GetActorLocation())>350)return;
 const float Now=GetWorld()->GetTimeSeconds();const auto* S=GetDefault<ULZOpeningSettings>();
 if(!P->HasCrowbarTool())
 {
  if(Now<NextCheck)return;NextCheck=Now+S->CheckCooldown;RattleUntil=Now+.32f;
  C->ShowFeedback(TEXT("门框变形了，需要撬开。"));LZAcoustics::Emit(this,Door->GetActorLocation(),S->CheckRadius,TEXT("Rattle"),TEXT("Rattle"),.15f);return;
 }
 Operator=P;Started=Now;bPrying=true;PryBar->SetVisibility(true,true);P->SetPrying(true);C->ShowFeedback(TEXT("插入撬棍……撬动门框"));
 LZAcoustics::Emit(this,Door->GetActorLocation(),S->CheckRadius,TEXT("Rattle"),TEXT("Rattle"),.12f);
}
void ALZOfficeOpening::Tick(float Delta)
{
 Super::Tick(Delta);if(!C || !IsValid(Leaf))return;const float Now=GetWorld()->GetTimeSeconds();
 if(!bPrying){if(RattleUntil>0){Pose(Now<RattleUntil?FMath::Sin((RattleUntil-Now)*55)*.35f:0);if(Now>=RattleUntil)RattleUntil=0;}return;}
 if(!IsValid(Operator) || GetWorld()->GetAuthGameMode<ALZGameMode>()->IsRunOver())
 {if(IsValid(Operator))Operator->SetPrying(false);PryBar->SetVisibility(false,true);bPrying=false;Pose(0);return;}
 const auto* S=GetDefault<ULZOpeningSettings>();const float T=FMath::Clamp((Now-Started)/FMath::Max(.4f,S->PrySeconds),0.f,1.f);
 Operator->UpdatePryPose(T);
 // Show the hook entering the scored jamb, then levering around its contact point.
 const float Insert=FMath::Clamp(T/.25f,0.f,1.f);
 const float Effort=FMath::Sin(FMath::Clamp((T-.25f)/.55f,0.f,1.f)*PI);
 const FRotator Lever(-Effort*18,0,0);
 const FVector Contact(-2539,-501,128);
 PryBar->SetWorldRotation(Lever);
 PryBar->SetWorldLocation(Contact-Lever.RotateVector(FVector(0,0,35))-FVector((1-Insert)*28,0,0));
 Pose(T<.65f?FMath::Sin(T*28)*.55f:FMath::InterpEaseInOut(0.f,S->OpenDegrees,(T-.65f)/.35f,2.f));
 if(T>=1)
 {
  bPrying=false;RattleUntil=0;C->bDoorBreached=true;++SuccessfulPrys;
  Leaf->SetActorEnableCollision(false);Leaf->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
  Door->bUsed=true;Door->SetActorEnableCollision(false);
  LZAcoustics::Emit(this,Door->GetActorLocation(),S->PopRadius,TEXT("Clang"),TEXT("MetalImpact"),.85f);
  C->ShowFeedback(TEXT("门框松开了。"));
 }
 if(T>=1){bPrying=false;PryBar->SetVisibility(false,true);Operator->SetPrying(false);Operator=nullptr;}
}
