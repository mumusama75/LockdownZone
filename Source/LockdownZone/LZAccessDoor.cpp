#include "LZAccessDoor.h"
#include "LZCharacter.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "LZStealthSettings.h"
#include "LZAcoustics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
ALZAccessDoor::ALZAccessDoor()
{
 PrimaryActorTick.bCanEverTick=true;
 auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));SetRootComponent(Root);
 LeftLeaf=CreateDefaultSubobject<USceneComponent>(TEXT("LeftLeaf"));LeftLeaf->SetupAttachment(Root);
 RightLeaf=CreateDefaultSubobject<USceneComponent>(TEXT("RightLeaf"));RightLeaf->SetupAttachment(Root);
 Mesh->SetupAttachment(LeftLeaf);Mesh->SetRelativeLocation(FVector(0,0,55));Mesh->SetRelativeScale3D(FVector(2.4f,.18f,1.1f));Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);
 Label->SetupAttachment(Root);Label->SetVisibility(false);Glow->SetupAttachment(Root);Glow->SetIntensity(0);
 auto Part=[&](const TCHAR* Name,USceneComponent* Parent,FVector P,FVector Size,bool Glass=false)
 {
  auto* M=CreateDefaultSubobject<UStaticMeshComponent>(Name);M->SetupAttachment(Parent);M->SetStaticMesh(Mesh->GetStaticMesh());M->SetRelativeLocation(P);M->SetRelativeScale3D(Size/100);
  M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);
  if(Glass)M->ComponentTags.Add(TEXT("DoorWindow"));return M;
 };
 Part(TEXT("RightBottom"),RightLeaf,FVector(0,0,55),FVector(240,18,110));
 for(int I=0;I<2;++I)
 {
  auto* Leaf=I?RightLeaf:LeftLeaf;
  Part(I?TEXT("RightTop"):TEXT("LeftTop"),Leaf,FVector(0,0,275),FVector(240,18,50));
  Part(I?TEXT("RightGlass"):TEXT("LeftGlass"),Leaf,FVector(0,0,180),FVector(220,4,140),true);
  Part(I?TEXT("RightEdgeA"):TEXT("LeftEdgeA"),Leaf,FVector(-115,0,180),FVector(10,18,140));
  Part(I?TEXT("RightEdgeB"):TEXT("LeftEdgeB"),Leaf,FVector(115,0,180),FVector(10,18,140));
 }
 Barrier=CreateDefaultSubobject<UBoxComponent>(TEXT("DoorBarrier"));Barrier->SetupAttachment(Root);Barrier->SetRelativeLocation(FVector(0,0,150));Barrier->SetBoxExtent(FVector(240,14,150));Barrier->SetCollisionProfileName(TEXT("BlockAll"));
 // Window visibility remains real glass; a separate full-height barrier provides simple stable collision.
 auto* Outer=Part(TEXT("OuterReader"),Root,FVector(265,-36,135),FVector(18,12,28));
 auto* Inner=Part(TEXT("InsideRelease"),Root,FVector(265,36,135),FVector(18,12,28));
 for(auto* R:{Outer,Inner}){R->SetCollisionEnabled(ECollisionEnabled::QueryOnly);R->SetCollisionResponseToAllChannels(ECR_Ignore);R->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);}
 LeftLeaf->SetRelativeLocation(FVector(-120,0,0));RightLeaf->SetRelativeLocation(FVector(120,0,0));
}
void ALZAccessDoor::BeginPlay()
{
 Super::BeginPlay();const auto* T=GetDefault<ULZStealthSettings>();HoldSeconds=T->DoorHoldSeconds;ClearDelay=T->DoorClearDelay;TravelSeconds=T->DoorTravelSeconds;SafetyDepth=T->DoorSafetyDepth;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel"));
 TArray<UStaticMeshComponent*> Parts;GetComponents(Parts);for(auto* P:Parts)
 {
  if(P->ComponentHasTag(TEXT("DoorWindow"))){P->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/M_ObservationGlass.M_ObservationGlass")));P->SetCastShadow(false);}
  else {auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),P->GetName().Contains(TEXT("Reader"))?FLinearColor(.08f,.45f,.36f):FLinearColor(.045f,.15f,.16f));P->SetMaterial(0,M);}
 }
}
void ALZAccessDoor::Block(bool Enabled)
{
 Barrier->SetCollisionEnabled(Enabled?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 FNavigationSystem::UpdateComponentData(*Barrier);
}
bool ALZAccessDoor::IsOccupied() const
{
 TArray<FOverlapResult> Overlaps;FCollisionQueryParams Q(SCENE_QUERY_STAT(AccessDoorSafety),false,this);
 GetWorld()->OverlapMultiByObjectType(Overlaps,GetActorLocation()+FVector(0,0,100),FQuat::Identity,FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeBox(FVector(270,SafetyDepth,170)),Q);
 for(const auto& O:Overlaps)if(Cast<ACharacter>(O.GetActor()))return true;
 return false;
}
void ALZAccessDoor::Open(){State=ELZAccessDoorState::Opening;ClearSince=-1;}
void ALZAccessDoor::Interact(ALZCharacter* P)
{
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();auto* C=GM?GM->GetChapter():nullptr;
 if(!P || !C || GM->IsRunOver() || P->IsInventoryOpen() || P->IsTraversing() || FVector::Dist2D(P->GetActorLocation(),GetActorLocation())>420)return;
 const bool Inside=P->GetActorLocation().Y>GetActorLocation().Y;
 if(!Inside && !C->HasAccess(RequiredAccess)){C->ShowFeedback(TEXT("需要门禁卡"));return;}
 if(GetWorld()->GetTimeSeconds()<NextUse)return;NextUse=GetWorld()->GetTimeSeconds()+.6f;
 if(State==ELZAccessDoorState::Open){OpenedAt=GetWorld()->GetTimeSeconds();return;}
 if(State==ELZAccessDoorState::Opening)return;
 LZAcoustics::Emit(this,GetActorLocation()+FVector(265,Inside?36:-36,135),GetDefault<ULZStealthSettings>()->DoorSoundRadius,TEXT("CardBeep"),TEXT("Door"),.18f);Open();
}
FString ALZAccessDoor::GetInteractionPrompt(const ALZCharacter* P) const
{
 if(State==ELZAccessDoorState::Opening)return TEXT("门正在开启");
 if(State==ELZAccessDoorState::Open)return TEXT("[E] 延长开门时间");
 return P && P->GetActorLocation().Y>GetActorLocation().Y?TEXT("[E] 内侧开门"):TEXT("[E] 读卡器");
}
void ALZAccessDoor::Tick(float Delta)
{
 Super::Tick(Delta);auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();if(!GM || GM->IsRunOver())return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(State==ELZAccessDoorState::Opening)
 {
  Fraction=FMath::Min(1.f,Fraction+Delta/FMath::Max(.1f,TravelSeconds));
  if(Fraction>=1){Block(false);State=ELZAccessDoorState::Open;OpenedAt=Now;ClearSince=-1;}
 }
 else if(State==ELZAccessDoorState::Open)
 {
  if(IsOccupied())ClearSince=-1;
  else if(ClearSince<0)ClearSince=Now;
  if(Now-OpenedAt>=HoldSeconds && ClearSince>=0 && Now-ClearSince>=ClearDelay){State=ELZAccessDoorState::Closing;LZAcoustics::Emit(this,GetActorLocation()+FVector(0,0,150),GetDefault<ULZStealthSettings>()->DoorSoundRadius,TEXT("SoftDoor"),TEXT("Door"),.14f);}
 }
 else if(State==ELZAccessDoorState::Closing)
 {
  if(IsOccupied()){Open();return;}
  Fraction=FMath::Max(0.f,Fraction-Delta/FMath::Max(.1f,TravelSeconds));
  if(Fraction<=0){Block(true);State=ELZAccessDoorState::Closed;}
 }
 LeftLeaf->SetRelativeLocation(FVector(-120-Fraction*245,0,0));RightLeaf->SetRelativeLocation(FVector(120+Fraction*245,0,0));
}
bool ALZAccessDoor::SoundApproach(FVector Listener,FVector Sound,FVector& Approach) const
{
 if(IsOpen())return false;
 const FVector L=Listener-GetActorLocation(),S=Sound-GetActorLocation();
 if(L.Y*S.Y>=0 || FMath::Abs(L.Y-S.Y)<1)return false;
 const float X=FMath::Lerp(L.X,S.X,-L.Y/(S.Y-L.Y));if(FMath::Abs(X)>290)return false;
 Approach=GetActorLocation()+FVector(FMath::Clamp(L.X,-155.f,155.f),L.Y<0?-150:150,90);return true;
}
void ALZAccessDoor::Knock(FVector From)
{
 const float Now=GetWorld()->GetTimeSeconds();if(!IsClosed() || Now<NextKnock)return;
 NextKnock=Now+3;++KnockCount;LZAcoustics::Emit(this,GetActorLocation()+FVector(0,From.Y<GetActorLocation().Y?-22:22,140),90,TEXT("DoorTap"),TEXT("Door"),.16f);
}
