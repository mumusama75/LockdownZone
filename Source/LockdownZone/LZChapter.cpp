#include "LZChapter.h"
#include "LZElevatorFinale.h"
#include "LZOfficeOpening.h"
#include "LZOpeningSettings.h"
#include "LZCrowbarVisual.h"
#include "LZIntercom.h"
#include "LZStealthSettings.h"
#include "LZAcoustics.h"
#include "LZFlashlightPickup.h"
#include "LZVentNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "LZGarage.h"
#include "LZRunState.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "LZBreakableGlass.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

void ALZChapterNode::Configure(ELZChapterNode InKind,FVector Size,FColor Color)
{
 ClosedPosition=GetActorLocation();
 Kind=InKind; Mesh->SetWorldScale3D(Size/100); SetLabel(TEXT(""),Color);
 Glow->SetAbsolute(false,false,true); Glow->SetIntensity(2); Glow->SetAttenuationRadius(90);
}
void ALZChapterNode::ToggleDoor(bool bVertical)
{
 bOpen=!bOpen;
 SetActorRotation(bOpen?FRotator(0,90,0):FRotator::ZeroRotator);
 SetActorLocation(bOpen?ClosedPosition+(bVertical?FVector(120,-120,0):FVector(-120,120,0)):ClosedPosition);
}
void ALZChapterNode::SetGlow(float Value){Glow->SetIntensity(Value);}
void ALZChapterNode::Interact(ALZCharacter* P){if(Chapter) Chapter->Use(this,P);}
float ALZChapterNode::TakeDamage(float Amount,const FDamageEvent& Event,AController* DamageInstigator,AActor* Causer)
{if(Chapter && Amount>0) Chapter->Strike(this,Cast<ALZCharacter>(Causer));return Amount;}
FString ALZChapterNode::GetInteractionPrompt(const ALZCharacter* P) const
{
 if(bUsed) return TEXT("");
 switch(Kind)
 {
 case ELZChapterNode::Crowbar:return LZCrowbarVisual::InteractionKey()+TEXT("拾取撬棍");
 case ELZChapterNode::Desk:case ELZChapterNode::EmptyDesk:return TEXT("[E] 翻找抽屉");
 case ELZChapterNode::Cabinet:return Chapter->bKey?TEXT("[E] 打开消防器材柜"):TEXT("锁着的消防器材柜");
 case ELZChapterNode::DoorLock:return Chapter->Opening && Chapter->Opening->IsPrying()?TEXT("正在撬动门框"):LZCrowbarVisual::InteractionKey()+(P && P->HasCrowbarTool()?TEXT("撬开卡住的门"):TEXT("检查卡住的出口"));
 case ELZChapterNode::RecordsDoor:return TEXT("[E] 撬动锁舌 · 会发出噪声");
 case ELZChapterNode::ArchiveDoor:case ELZChapterNode::DoorD:return bOpen?TEXT("[E] 关门"):TEXT("[E] 开门");
 case ELZChapterNode::DoorB:return TEXT("[E] 读卡器");
 case ELZChapterNode::DoorC:
  if(!Chapter->bCUnlocked)return P && P->GetActorLocation().Y>950?TEXT("[E] 从内侧解锁"):TEXT("[E] 尝试开门");
  return bOpen?TEXT("[E] 关门"):TEXT("[E] 开门");
 case ELZChapterNode::Fuse:return TEXT("[E] 取出 15A 保险丝");
 case ELZChapterNode::Breaker:return Chapter->bFuse?TEXT("[E] 更换保险丝"):TEXT("烧毁的 15A 保险丝座");
 case ELZChapterNode::Elevator:return Chapter->ElevatorPrompt();
 case ELZChapterNode::Throwable:return ActorHasTag(TEXT("LZSpareBottle"))?TEXT("[E] 拾取空瓶"):TEXT("[E] 拾取投掷物");
 case ELZChapterNode::Flashlight:return TEXT("[E] 拾取手电筒");
 case ELZChapterNode::Pistol:return TEXT("[E] 拾取保安手枪");
 case ELZChapterNode::Ammo:return TEXT("[E] 拾取 9mm 弹药");
 case ELZChapterNode::Vehicle:return TEXT("[E] 进入应急车辆");
 case ELZChapterNode::Medical:return TEXT("[E] 拾取医疗包");
 default:return TEXT("");
 }
}
ALZChapter::ALZChapter(){PrimaryActorTick.bCanEverTick=true;}
void ALZChapter::BeginPlay()
{
 Super::BeginPlay(); GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 Attenuation=NewObject<USoundAttenuation>(this);
 Attenuation->Attenuation.bAttenuate=true; Attenuation->Attenuation.FalloffDistance=3500;
 Attenuation->Attenuation.AttenuationShapeExtents=FVector(100);
 Build();
 VentNetwork=GetWorld()->SpawnActor<ALZVentNetwork>();VentNetwork->Build(GM);
 Finale=GetWorld()->SpawnActor<ALZElevatorFinale>();Finale->Initialize(this,ElevatorGate);
}
ALZChapterNode* ALZChapter::Add(ELZChapterNode Kind,FVector Pos,FVector Size,FColor Color)
{
 auto* N=GetWorld()->SpawnActor<ALZChapterNode>(Pos,FRotator::ZeroRotator);
 N->Chapter=this;N->Configure(Kind,Size,Color);Nodes.Add(N);return N;
}
ALZChapterNode* ALZChapter::FindNode(ELZChapterNode Kind) const
{for(auto* N:Nodes) if(IsValid(N) && N->Kind==Kind && !N->bUsed) return N;return nullptr;}
void ALZChapter::Build()
{
 // Replace old scenario objects, keeping the authored building and its collision.
 TArray<AActor*> Remove;
 for(TActorIterator<ALZInteractable> It(GetWorld());It;++It) Remove.Add(*It);
 for(TActorIterator<ALZEnemy> It(GetWorld());It;++It) Remove.Add(*It);
 for(TActorIterator<ALZBreakableGlass> It(GetWorld());It;++It) Remove.Add(*It);
 for(auto* A:Remove) A->Destroy();
 GM->bOfficeBlackout=true;GM->StatusText.Empty();
 // Observation glass is reinforced: the lock is the deliberate sound tutorial trigger.
 auto* Glass=GM->SpawnBlock(TEXT("ReinforcedObservationGlass"),FVector(-2520,120,195),FVector(4,810,230));
 auto* GlassMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/M_ObservationGlass.M_ObservationGlass"));
 if(GlassMaterial) Glass->GetStaticMeshComponent()->SetMaterial(0,GlassMaterial);
 Glass->GetStaticMeshComponent()->SetCastShadow(false);
 const FColor Amber(210,160,70),Teal(65,150,150),Red(180,45,30);
 auto* Tool=Add(ELZChapterNode::Crowbar,GetDefault<ULZOpeningSettings>()->RepairBench,FVector(1),Amber);
 auto* Jammed=Add(ELZChapterNode::DoorLock,FVector(-2545,-610,145),FVector(20,220,280),Amber);
 Opening=GetWorld()->SpawnActor<ALZOfficeOpening>();Opening->Setup(this,Jammed,Tool,GM->StartRoomDoor);
 for(auto Pos:{FVector(-3160,-440,90),FVector(-1730,-480,88),FVector(-1250,500,88),FVector(300,-1680,80),FVector(2900,750,35)})
     Add(ELZChapterNode::Throwable,Pos,FVector(12,12,17),Teal);
 Add(ELZChapterNode::Medical,FVector(-1150,-1670,90),FVector(25,18,12),Red);
 RecordsGate=GM->VaultBarrier;
 Add(ELZChapterNode::RecordsDoor,FVector(-450,1300,115),FVector(35,18,35),Amber);
 ArchiveGate=Add(ELZChapterNode::ArchiveDoor,FVector(500,1350,150),FVector(240,24,300),Teal);

 Add(ELZChapterNode::DoorC,FVector(3380,900,150),FVector(240,24,300),Teal);
 Add(ELZChapterNode::DoorD,FVector(2200,1150,150),FVector(24,240,300),Teal);
 // Clear floor patch opposite the low-sill corridor window; no crate hides the pickup.
 ArchiveFlashlight=GetWorld()->SpawnActor<ALZFlashlightPickup>(FVector(1020,1560,1),FRotator(0,-105,0));
 ArchiveFlashlight->EnableGuideBeam();
 GM->SpawnBlock(TEXT("ElectricalFuseBench"),FVector(2900,1720,38),FVector(140,70,76));
 Add(ELZChapterNode::Fuse,FVector(2900,1720,88),FVector(25,15,12),Amber);
 // Surface-mounted riser, then clipped conduit immediately below the ceiling.
 for(auto Segment:TArray<TPair<FVector,FVector>>{
  {FVector(3477,230,258.5f),FVector(4,4,157)},
  {FVector(3428.5f,230,337),FVector(97,4,4)},
  {FVector(3380,549,337),FVector(4,638,4)},
  {FVector(3380,868,325),FVector(4,4,24)}})
 {
  auto* Cable=GM->SpawnBlock(TEXT("LiftToElectricalCable"),Segment.Key,Segment.Value,FRotator::ZeroRotator,FLinearColor(.55f,.30f,.045f));
  Cable->SetActorEnableCollision(false);Cable->Tags.Add(TEXT("LZCeilingCable"));
 }
 for(float Y:{300.f,480.f,660.f,825.f})
  GM->SpawnBlock(TEXT("CableCeilingClip"),FVector(3380,Y,342),FVector(10,6,8),FRotator::ZeroRotator,FLinearColor(.10f,.12f,.13f))->SetActorEnableCollision(false);
 Add(ELZChapterNode::Medical,FVector(-460,1650,45),FVector(25,18,12),Red);
 // The damaged panel beside the lift points to the missing fuse.
 Panel=Add(ELZChapterNode::Breaker,FVector(3470,230,135),FVector(18,65,90),Red);
 Spark=NewObject<UPointLightComponent>(this);AddInstanceComponent(Spark);Spark->RegisterComponent();
 Spark->SetWorldLocation(FVector(3450,210,160));Spark->SetLightColor(FLinearColor(1,.4f,.06f));Spark->SetAttenuationRadius(220);
 Lift=Add(ELZChapterNode::Elevator,FVector(3470,-180,135),FVector(20,18,45),Red);
 ElevatorGate=GM->ExitDoor;
 auto* Indicator=GetWorld()->SpawnActor<ATextRenderActor>(FVector(3470,0,270),FRotator(0,180,0));
 FloorDisplay=Indicator->GetTextRender();FloorDisplay->SetWorldSize(38);FloorDisplay->SetHorizontalAlignment(EHTA_Center);FloorDisplay->SetText(FText::FromString(TEXT("12  /  --")));
 for(int32 I=0;I<5;++I)
 {
   auto* Particle=GM->SpawnBlock(TEXT("PanelSpark"),Panel->GetActorLocation(),FVector(1,1,6),FRotator(25,I*70,20));
   Particle->SetActorEnableCollision(false);
   Particle->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDAmber.M_ZT_LEDAmber")));
   Sparks.Add(Particle);
 }
 // Same amber spark streaks as the elevator, on a damaged junction at the vent lip.
 const FVector VentSparkOrigin(-1700,-1240,340);
 GM->SpawnBlock(TEXT("MeetingVentDamagedJunction"),VentSparkOrigin+FVector(0,-7,0),FVector(24,12,28));
 VentGuideLight=NewObject<UPointLightComponent>(this);AddInstanceComponent(VentGuideLight);VentGuideLight->RegisterComponent();
 VentGuideLight->SetWorldLocation(VentSparkOrigin+FVector(0,16,0));VentGuideLight->SetLightColor(FLinearColor(1,.4f,.06f));VentGuideLight->SetAttenuationRadius(360);
 for(int32 I=0;I<5;++I){auto* Particle=GM->SpawnBlock(TEXT("MeetingVentGuideSpark"),VentSparkOrigin,FVector(1,1,6),FRotator(25,I*70,20));
 Particle->SetActorEnableCollision(false);Particle->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDAmber.M_ZT_LEDAmber")));VentGuideSparks.Add(Particle);}
 // Inside release creates a persistent return loop for either entry method.
 Add(ELZChapterNode::RecordsDoor,FVector(-450,1410,115),FVector(25,18,30),Teal);
 for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It) It->GetLightComponent()->SetIntensity(0);
 for(TActorIterator<ASkyLight> It(GetWorld());It;++It) It->GetLightComponent()->SetIntensity(0);
 for(TActorIterator<APointLight> It(GetWorld());It;++It)
 {const FVector P=It->GetActorLocation();if(P.X>-200 && P.X<1700 && P.Y>1350) It->GetLightComponent()->SetIntensity(0);}
 // Local illumination establishes readable safe spaces while the archive stays unlit.
 const FVector GuideLights[]={FVector(-3300,-200,260),FVector(-3100,450,250),FVector(-2100,-400,270),FVector(-1200,0,280),FVector(0,0,280),FVector(1400,0,280),FVector(2800,0,280),FVector(-1150,1480,260),FVector(-950,1550,525),FVector(-450,1650,240)};
 for(int32 I=0;I<10;++I)
 {
   auto* Light=GetWorld()->SpawnActor<APointLight>(GuideLights[I],FRotator::ZeroRotator);
   auto* Component=Cast<UPointLightComponent>(Light->GetLightComponent());
   Component->SetIntensity(I<7?170:90);Component->SetAttenuationRadius(I<7?750:380);
   Component->SetLightColor(I==8?FLinearColor(.25f,.6f,.7f):FLinearColor(.85f,.70f,.45f));Component->SetCastShadows(true);
 }
 // Visible local fixtures light the door leaves and the gathering/interaction space.
 for(auto Position:{FVector(300,650,285),FVector(3380,660,285),FVector(2050,1150,285),FVector(3350,-220,245)})
 {
  auto* Light=GetWorld()->SpawnActor<APointLight>(Position,FRotator::ZeroRotator);
  auto* L=Cast<UPointLightComponent>(Light->GetLightComponent());L->SetIntensityUnits(ELightUnits::Unitless);
  L->SetIntensity(Position.X<1000?220:Position.Y<0?85:200);L->SetAttenuationRadius(Position.Y<0?420:700);
  L->SetLightColor(FLinearColor(.88f,.84f,.70f));L->SetCastShadows(true);Light->Tags.Add(TEXT("LZDoorAreaLight"));
  auto* Fixture=GM->SpawnBlock(TEXT("DoorAreaFixture"),FVector(Position.X,Position.Y,339),FVector(70,18,8),FRotator::ZeroRotator,FLinearColor(.7f,.7f,.6f));
  Fixture->SetActorEnableCollision(false);
 }
 // Texture-backed note renders Chinese reliably on the elevator jamb.
 auto* Note=GM->SpawnArtMesh(TEXT("LiftFuseNote"),TEXT("/Engine/BasicShapes/Plane.Plane"),FVector(3478,-285,190),FRotator(0,-90,90),FVector(-.80f,.4375f,1),false);
 if(Note){Note->Tags.Add(TEXT("LZLiftFuseNote"));Note->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/Signage/M_LiftFuseNote.M_LiftFuseNote")));}
 BuildDarkPantry();
 BuildAdminStealthRoute();
 GM->SpawnEnemy(GetDefault<ULZOpeningSettings>()->TeachingInfected,false);
 GM->SpawnEnemy(FVector(-1700,320,90),false);
 GM->SpawnEnemy(FVector(1800,-1100,90),false);
 auto* Rating=GetWorld()->SpawnActor<ATextRenderActor>(FVector(3458,230,190),FRotator(0,180,0));
 Rating->GetTextRender()->SetText(FText::FromString(TEXT("15A")));
 Rating->GetTextRender()->SetWorldSize(12);Rating->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
}
void ALZChapter::Say(const TCHAR* Text){Feedback=Text;FeedbackUntil=GetWorld()->GetTimeSeconds()+2;}
FString ALZChapter::GetFeedback() const{if(Finale && !Finale->Prompt().IsEmpty())return Finale->Prompt();if(Garage && !Garage->Hint().IsEmpty())return Garage->Hint();return GetWorld()->GetTimeSeconds()<FeedbackUntil?Feedback:(IsValid(AdminRadio)?AdminRadio->AudibleSubtitle(Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0))):FString());}
void ALZChapter::Noise(FVector Location,float Radius,FName Sound,FName Category)
{
 if(GM->IsRunOver())return;
 const float Volume=Sound==TEXT("Step")?FMath::Clamp(Radius/1200.f,.05f,.5f):1.f;
 LZAcoustics::Emit(this,Location,Radius,Sound,Category.IsNone()?Sound:Category,Volume);
}
void ALZChapter::Strike(ALZChapterNode* Node,ALZCharacter* P)
{
 // A jammed steel frame is released by deliberate interaction, not weapon damage.
}
void ALZChapter::Use(ALZChapterNode* N,ALZCharacter* P)
{
 if(!N || !P || N->bUsed || GM->IsRunOver() || P->IsInventoryOpen() || P->IsTraversing() || FVector::Dist(P->GetActorLocation(),N->GetActorLocation())>360)return;
 switch(N->Kind)
 {
 case ELZChapterNode::DoorLock:if(Opening)Opening->Interact(P);return;
 case ELZChapterNode::Pistol:if(!P->AcquireWeapon(EPlayerWeapon::Firearm))return;break;
 case ELZChapterNode::Ammo:if(!P->TryStoreItem(ELZInventoryItemType::Ammo,17))return;break;
 case ELZChapterNode::Vehicle:if(Garage)Garage->EnterVehicle(P);return;
 case ELZChapterNode::Crowbar:
   if(!P->AcquireCrowbar())return;bCrowbar=true;P->PresentCrowbarPickup(FTransform(FRotator(90,12,0),N->GetActorLocation()));Say(TEXT("撬棍：可撬开卡住的门与格栅。"));break;
 case ELZChapterNode::Desk:if(!bCrowbar){Say(TEXT("抽屉卡住了，需要撬动"));return;}bKey=true;Noise(N->GetActorLocation(),120,TEXT("Rattle"));Say(TEXT("小钥匙 · 消防柜"));break;
 case ELZChapterNode::EmptyDesk:Noise(N->GetActorLocation(),120,TEXT("Rattle"));Say(TEXT("抽屉是空的"));break;
 case ELZChapterNode::Cabinet:
   if(!bKey){Say(TEXT("钥匙孔旁有消防斧图案"));return;}
   if(!P->AcquireWeapon(EPlayerWeapon::Melee))return;bAxe=true;break;

 case ELZChapterNode::ArchiveDoor:case ELZChapterNode::DoorD:case ELZChapterNode::DoorC:
 {
  if(N->Kind==ELZChapterNode::DoorC && !bCUnlocked)
  {
   if(P->GetActorLocation().Y<=950){Say(TEXT("无法从这一侧打开"));return;}
   bCUnlocked=true;Say(TEXT("已从配电间内侧解锁"));
  }
  if(GetWorld()->GetTimeSeconds()<NextUse)return;
  NextUse=GetWorld()->GetTimeSeconds()+.45f;N->ToggleDoor(N->Kind==ELZChapterNode::DoorD);
  Noise(N->ClosedPosition,200,TEXT("Rattle"));return;
 }
 case ELZChapterNode::RecordsDoor:
 {
   const bool Inside=P->GetActorLocation().Y>1380;
   if(!Inside && !bCrowbar){Say(TEXT("锁舌需要撬动"));return;}
   if(GetWorld()->GetTimeSeconds()<NextUse)return;NextUse=GetWorld()->GetTimeSeconds()+.6f;
   if(!Inside){Noise(N->GetActorLocation(),1100);if(++N->Hits<3){Say(TEXT("锁舌松动了"));return;}}
   AActor* Gate=N->Kind==ELZChapterNode::RecordsDoor?RecordsGate:ArchiveGate;
   if(IsValid(Gate))Gate->Destroy();
   for(auto* Other:Nodes)if(IsValid(Other) && Other!=N && Other->Kind==N->Kind){Other->bUsed=true;Other->Destroy();}
   break;
 }
 case ELZChapterNode::Flashlight:if(!P->AcquireFlashlight())return;break;
 case ELZChapterNode::Throwable:if(!P->TryStoreItem(ELZInventoryItemType::Scrap))return;break;
 case ELZChapterNode::Medical:if(!P->TryStoreItem(ELZInventoryItemType::Medical))return;break;
 case ELZChapterNode::Fuse:if(bFuse || bFuseInstalled){Say(TEXT("已有备用保险丝"));return;}bFuse=true;Say(TEXT("15A 保险丝"));break;
 case ELZChapterNode::Breaker:
   if(!bFuse){Say(TEXT("空缺的 15A 保险丝座"));return;}
   bFuse=false;bFuseInstalled=true;Spark->SetIntensity(0);GM->bOfficePowerRestored=true;SetElevator(ELZElevatorState::Ready);Noise(N->GetActorLocation(),450,TEXT("Relay"));break;
 case ELZChapterNode::Elevator:
   if(ElevatorState==ELZElevatorState::Ready){SetElevator(ELZElevatorState::Arriving);Noise(LiftLocation,900,TEXT("Motor"));}

   else if(ElevatorState==ELZElevatorState::Offline){Say(TEXT("按钮不亮，旁边电闸正在冒火星"));Noise(Panel->GetActorLocation(),150,TEXT("Spark"));}
   return;
 default:return;
 }
 N->bUsed=true;N->SetGlow(0);
 if(N->Kind!=ELZChapterNode::Cabinet && N->Kind!=ELZChapterNode::Breaker)N->Destroy();
}
FString ALZChapter::ElevatorPrompt() const
{
 switch(ElevatorState){
 case ELZElevatorState::Ready:return LZCrowbarVisual::InteractionKey()+TEXT("呼叫电梯");
 case ELZElevatorState::Arriving:return TEXT("轿厢正在接近…");
 case ELZElevatorState::Jammed:return TEXT("已停稳 · 轿门卡住");
 case ELZElevatorState::Open:return TEXT("轿门已开");
 default:return TEXT("电梯按钮");}
}
bool ALZChapter::QTE(bool bE,ALZCharacter* P)
{return false;} // Legacy API deliberately disabled; only the door-seam interactable starts the finale.
void ALZChapter::SetElevator(ELZElevatorState State)
{ElevatorState=State;StateTime=GetWorld()->GetTimeSeconds();Lift->SetGlow(State==ELZElevatorState::Ready || State==ELZElevatorState::Open?12:3);}
void ALZChapter::Tick(float Delta)
{
 Super::Tick(Delta);if(!GM || GM->IsRunOver())return;
 auto* P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));if(!P)return;
 const float Now=GetWorld()->GetTimeSeconds();
 const FVector StepPosition=P->GetActorLocation();const float Moved=FVector::Dist2D(LastStepPosition,StepPosition);LastStepPosition=StepPosition;
 if(Moved>150)StepDistance=0; // teleport/traversal transitions do not emit phantom steps
 else if(bDoorBreached && !P->IsTraversing())StepDistance+=Moved;
 const float Stride=P->bIsCrouched?85.f:P->IsSprinting()?165.f:140.f;
 if(bDoorBreached && !P->IsTraversing() && Moved>.1f && StepDistance>=Stride && Now>NextFootstep)
 {
  StepDistance=0;NextFootstep=Now+(P->bIsCrouched?.65f:P->IsSprinting()?.28f:.38f);
  const auto* T=GetDefault<ULZStealthSettings>();
  Noise(StepPosition,P->bIsCrouched?T->CrouchRadius:P->IsSprinting()?T->SprintRadius:T->WalkRadius,TEXT("Step"),P->bIsCrouched?TEXT("Crouch"):P->IsSprinting()?TEXT("Sprint"):TEXT("Walk"));
 }
 if(Garage)return;
 for(int32 I=0;I<Sparks.Num();++I)
 {
   const float Phase=FMath::Fmod(Now*2+I*.14f,1.f);Sparks[I]->SetActorHiddenInGame(bFuseInstalled || Phase>.3f);
   Sparks[I]->SetActorLocation(Panel->GetActorLocation()+FVector(-25-Phase*160,(I-2)*Phase*75,25-Phase*180));
 }
 if(ElevatorState==ELZElevatorState::Arriving)FloorDisplay->SetText(FText::FromString(FString::Printf(TEXT("%02d  ^"),FMath::Clamp(7+int32(Now-StateTime),7,12))));
 if(ElevatorState==ELZElevatorState::Jammed)FloorDisplay->SetText(FText::FromString(TEXT("12  !")));
 if(ElevatorState==ELZElevatorState::Descending)FloorDisplay->SetText(FText::FromString(FString::Printf(TEXT("%02d  v"),FMath::Clamp(12-int32((Now-StateTime)*3),1,12))));
 for(int32 I=0;I<VentGuideSparks.Num();++I){const float Phase=FMath::Fmod(Now*2+I*.14f,1.f);VentGuideSparks[I]->SetActorHiddenInGame(Phase>.3f);
 VentGuideSparks[I]->SetActorLocation(FVector(-1700,-1240,340)+FVector((I-2)*Phase*65,Phase*100,15-Phase*210));}
 if(VentGuideLight)VentGuideLight->SetIntensity(FMath::Sin(Now*37)>.8f?90:0);
 if(!bFuseInstalled){Spark->SetIntensity(FMath::Sin(Now*37)>.8f?80:0);if(Now>NextSpark){NextSpark=Now+3.3f;Noise(Panel->GetActorLocation(),0,TEXT("Spark"));}}
 if(ElevatorState==ELZElevatorState::Arriving && Now-StateTime>5)
 {
   SetElevator(ELZElevatorState::Jammed);if(Finale)Finale->Jam();
 }
}
