#include "LZChapterQA.h"
#include "LZOfficeOpening.h"
#include "LZOpeningSettings.h"
#include "LZAccessDoor.h"
#include "LZIntercom.h"
#include "LZStealthSettings.h"
#include "LZVentNetwork.h"
#include "LZFlashlightPickup.h"
#include "LZChapter.h"
#include "LZGarage.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Camera/CameraComponent.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "LZWeaponPickup.h"
#include "LZNoiseProjectile.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
ALZChapterQA::ALZChapterQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZChapterQA::BeginPlay()
{Super::BeginPlay();Player=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));Chapter=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();Started=GetWorld()->GetTimeSeconds();Next=Started+3;}
void ALZChapterQA::Check(bool Condition,const TCHAR* Text)
{++Assertions;if(!Condition)++Failures;const FString Line=FString::Printf(TEXT("CHAPTER_QA %s %s"),Condition?TEXT("PASS"):TEXT("FAIL"),Text);Lines.Add(Line);UE_LOG(LogTemp,Display,TEXT("%s"),*Line);}
void ALZChapterQA::View(FVector Position,FVector Target)
{Player->GetCharacterMovement()->StopMovementImmediately();Player->SetActorLocation(Position);const FRotator Aim=(Target-(Position+FVector(-10,0,64))).Rotation();Cast<APlayerController>(Player->GetController())->SetControlRotation(Aim);Player->FindComponentByClass<UCameraComponent>()->SetWorldRotation(Aim);}
void ALZChapterQA::Capture(const TCHAR* Name)
{FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Chapter_")+Name+TEXT(".png"),true,false);}
void ALZChapterQA::Use(uint8 Kind)
{
 auto* N=Chapter->FindNode(static_cast<ELZChapterNode>(Kind));Check(N!=nullptr,TEXT("required interactive node exists"));if(!N)return;
 bool Accessible=false;
 const FVector Directions[]={FVector(-1,0,0),FVector(1,0,0),FVector(0,-1,0),FVector(0,1,0),FVector(-.7f,-.7f,0),FVector(.7f,-.7f,0)};
 for(float Range:{150.f,240.f,310.f})
 {
   for(FVector Direction:Directions)
   {
     FVector Pos=N->GetActorLocation()+Direction*Range;Pos.Z=Chapter->Garage?Chapter->Garage->Origin.Z+90:90;
     FCollisionQueryParams Params(SCENE_QUERY_STAT(ChapterApproach),false,Player);FHitResult Hit;
     if(GetWorld()->OverlapBlockingTestByChannel(Pos,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params))continue;
     if(GetWorld()->LineTraceSingleByChannel(Hit,Pos+FVector(-10,0,64),N->GetActorLocation(),ECC_Visibility,Params) && Hit.GetActor()==N)
     {View(Pos,N->GetActorLocation());Accessible=true;break;}
   }
   if(Accessible)break;
 }
 Check(Accessible,TEXT("node has a free standing approach and unobstructed interaction trace"));
 if(!Accessible)UE_LOG(LogTemp,Display,TEXT("CHAPTER_INACCESSIBLE kind=%d position=%s"),Kind,*N->GetActorLocation().ToString());
 N->Interact(Player);
}
void ALZChapterQA::Tick(float Delta)
{
 Super::Tick(Delta);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
 const float Now=GetWorld()->GetTimeSeconds();if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;
 if(!Player || !Chapter || Now-Started>150){Check(false,TEXT("chapter runtime exists within timeout"));Finish();return;}
 Next=Now+.8f;
 auto UseNode=[&](ELZChapterNode N){Use(static_cast<uint8>(N));};
 switch(Step++)
 {
 case 0:
   Chapter->AdminRadio->NextCallAt=BIG_NUMBER; // isolated legacy mechanics; radio encounter has its own live QA
   Check(!Player->HasFirearm() && !Player->HasMeleeWeapon(),TEXT("spawn is unarmed without a gun"));
   {int32 Legacy=0,Pantry=0;for(TActorIterator<ALZWeaponPickup> It(GetWorld());It;++It){if(It->ActorHasTag(TEXT("LZPantryPistol")) && FVector::Dist(It->GetActorLocation(),FVector(920,-1870,12))<1)++Pantry;else ++Legacy;}Check(Legacy==0 && Pantry==1,TEXT("legacy pickups removed; sole authored floor pistol is in dark pantry"));}
   for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){Check(!It->HasHeardNoise() && It->GetVelocity().Size2D()<1,TEXT("unarmed opening enemies remain still"));It->SetActorTickEnabled(false);}
   {bool Transparent=false;for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
   {auto* M=It->GetStaticMeshComponent()->GetMaterial(0);if(M && M->GetName().Contains(TEXT("M_ObservationGlass")))Transparent=M->GetBlendMode()==BLEND_Translucent;}
   Check(Transparent,TEXT("opening observation pane uses dedicated translucent material"));}
   Capture(TEXT("01_OfficeStart"));break;
 case 1:
   Check(!Chapter->FindNode(ELZChapterNode::Cabinet) && !Chapter->FindNode(ELZChapterNode::Desk),TEXT("opening no longer spawns key or axe chain"));
   UseNode(ELZChapterNode::DoorLock);Check(!Chapter->bDoorBreached,TEXT("jammed door needs crowbar"));
   UseNode(ELZChapterNode::Crowbar);Check(Chapter->bCrowbar && Player->GetSelectedWeapon()==EPlayerWeapon::Crowbar,TEXT("crowbar pickup equips permanent melee tool"));break;
 case 2:Check(Player->GetUsedBagSlots()==0 && Player->HasCrowbarTool(),TEXT("required crowbar occupies no inventory cells"));break;
 case 3:
   View(FVector(-2740,-570,90),Chapter->FindNode(ELZChapterNode::DoorLock)->GetActorLocation());Player->QAFire();Check(!Chapter->bDoorBreached,TEXT("weapon strike does not bypass deliberate pry interaction"));break;
 case 4:
   UseNode(ELZChapterNode::DoorLock);Check(Chapter->Opening->IsPrying(),TEXT("tool interaction starts short pry"));Next=Now+GetDefault<ULZOpeningSettings>()->PrySeconds+.2f;break;
 case 5:Check(Chapter->bDoorBreached && !Chapter->bKey && !Chapter->bAxe,TEXT("pry opens office without key or axe"));Capture(TEXT("02_BreachedDoor"));Next=Now+1;break;
 case 6:
   for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)It->GetCharacterMovement()->StopMovementImmediately();
   View(FVector(-500,0,90),FVector(0,0,90));Probe=GetWorld()->SpawnActor<ALZEnemy>(FVector(0,0,90),FRotator(0,180,0));Before=Probe->GetActorLocation();Next=Now+1;break;
 case 7:
   Check(FVector::Dist2D(Before,Probe->GetActorLocation())<1,TEXT("visible stationary player does not attract blind infected"));
   Chapter->Noise(FVector(650,0,90),1500);Next=Now+1.1f;break;
 case 8:
   Check(Probe->GetActorLocation().X>Before.X+25,TEXT("infected moves to sound away from visible player"));Probe->Destroy();
   UseNode(ELZChapterNode::Throwable);View(FVector(650,-20,90),FVector(-600,0,500));
   {auto* Camera=Player->FindComponentByClass<UCameraComponent>();FCollisionQueryParams Q(SCENE_QUERY_STAT(ChapterThrowClearance),false,Player);FHitResult Hit;
   const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,Camera->GetComponentLocation(),Camera->GetComponentLocation()+Camera->GetForwardVector()*55,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(6),Q);
   Check(!Blocked,TEXT("throw test uses clear launch space away from destroyed probe"));if(Blocked)UE_LOG(LogTemp,Warning,TEXT("CHAPTER_THROW_BLOCK %s"),*GetNameSafe(Hit.GetActor()));}
   Before.X=Player->GetThrowableCount();Player->ThrowNoiseItem();UE_LOG(LogTemp,Display,TEXT("CHAPTER_THROW_INVENTORY before=%.0f after=%d"),Before.X,Player->GetThrowableCount());Check(Player->GetThrowableCount()==Before.X-1,TEXT("throw consumes one real inventory item"));Next=Now+1.3f;break;
 case 9:
   {int32 ImpactCount=0;for(TActorIterator<ALZNoiseProjectile> It(GetWorld());It;++It)ImpactCount+=It->Impacts;Check(ImpactCount>0,TEXT("thrown object follows ballistic flight and impacts geometry"));}
   Probe=GetWorld()->SpawnActor<ALZEnemy>(FVector(2270,200,90),FRotator(0,180,0));Probe->SetActorTickEnabled(false);View(FVector(2100,200,90),Probe->GetActorLocation());Before=Probe->GetActorLocation();Player->QAStartAim();
   {FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(PushDiagnostic),false,Player);GetWorld()->LineTraceSingleByChannel(Hit,Player->GetActorLocation(),Probe->GetActorLocation(),ECC_Visibility,Q);UE_LOG(LogTemp,Display,TEXT("CHAPTER_PUSH blocking=%d player=%s before=%s after=%s hit=%s"),Player->IsBlocking(),*Player->GetActorLocation().ToString(),*Before.ToString(),*Probe->GetActorLocation().ToString(),*GetNameSafe(Hit.GetActor()));}
   Check(Player->IsBlocking() && FVector::Dist2D(Before,Probe->GetActorLocation())>50,TEXT("right mouse blocks and physically pushes enemy"));
   BeforeHealth=Player->GetHealth();UGameplayStatics::ApplyDamage(Player,20,Probe->GetController(),Probe,nullptr);Check(Player->GetHealth()>BeforeHealth-10,TEXT("frontal melee block reduces damage"));Player->QAStopAim();Probe->Destroy();Step=40;break;
 case 10:
   View(FVector(-2070,-1670,90),FVector(-1600,-1350,355));Capture(TEXT("03a_MeetingVentEntry"));break;
 case 11:
   Check(Chapter->VentNetwork && ALZVentNetwork::Outlets().Num()==1,TEXT("meeting-to-archive ventilation has only one room exit"));
   {FCollisionQueryParams Params(SCENE_QUERY_STAT(VentQA),false,Player);FHitResult Hit;
   Check(!GetWorld()->SweepSingleByChannel(Hit,FVector(800,-1600,605),FVector(800,1500,605),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,48),Params),TEXT("single ventilation spine passes crouched capsule"));
   Check(GetWorld()->SweepSingleByChannel(Hit,FVector(800,500,645),FVector(800,700,645),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params),TEXT("vent refuses standing capsule"));}
   View(FVector(800,0,605),FVector(800,1550,605));Capture(TEXT("03_VentRoute"));break;
 case 12:
   View(FVector(200,1750,90),FVector(420,1850,60));Player->StopCrouch();
   Check(IsValid(Chapter->ArchiveFlashlight),TEXT("archive supplies a visible flashlight pickup"));
   if(IsValid(Chapter->ArchiveFlashlight))Chapter->ArchiveFlashlight->Interact(Player);
   Check(Player->HasFlashlight() && !IsValid(Chapter->ArchiveFlashlight),TEXT("archive pickup equips flashlight and removes world beam"));break;
 case 13:
 {
   View(FVector(500,1550,90),FVector(500,1410,115));
   auto* Archive=Chapter->FindNode(ELZChapterNode::ArchiveDoor);if(Archive)Archive->Interact(Player);
   Check(Archive && Archive->bOpen,TEXT("D04 opens normally from archive without a key or breach"));break;
 }
 case 14:
   {FCollisionQueryParams Params(SCENE_QUERY_STAT(ArchiveExitQA),false,Player);FHitResult Hit;
   Check(!GetWorld()->SweepSingleByChannel(Hit,FVector(500,1490,90),FVector(500,1160,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params),TEXT("released archive door returns player to admin gallery"));}break;
 case 15:
   View(FVector(600,1550,90),FVector(1000,1790,85));if(Player->IsFlashlightOn())Player->ToggleFlashlight();Next=Now+.8f;break;
 case 16:Capture(TEXT("04_DarkRoom_Off"));Next=Now+.5f;break;
 case 17:Player->ToggleFlashlight();Next=Now+.5f;break;
 case 18:Capture(TEXT("05_DarkRoom_On"));Step=46;break;
 case 19:
   UseNode(ELZChapterNode::Breaker);Check(!Chapter->bFuseInstalled && Chapter->GetElevatorState()==ELZElevatorState::Offline,TEXT("panel stays offline without a replacement fuse"));
   UseNode(ELZChapterNode::Fuse);Check(Chapter->bFuse,TEXT("electrical room supplies the replacement fuse"));
   Check(!Chapter->FindNode(ELZChapterNode::Fuse),TEXT("archive and IT no longer bypass the electrical-room route"));break;
 case 20:UseNode(ELZChapterNode::Breaker);Check(Chapter->bFuseInstalled && Chapter->GetElevatorState()==ELZElevatorState::Ready,TEXT("replacement fuse alone restores power and enables lift"));break;
 case 21:UseNode(ELZChapterNode::Elevator);Check(Chapter->GetElevatorState()==ELZElevatorState::Arriving,TEXT("lift motor starts on call button"));View(FVector(3200,0,90),FVector(3500,0,150));Capture(TEXT("06_LiftApproach"));Next=Now+5.6f;break;
 case 22:
   Check(Chapter->IsJammed(),TEXT("arriving lift jams and starts sound finale"));
   {int32 Pursuers=0;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)if(It->IsActorTickEnabled() && It->HasHeardNoise())++Pursuers;
   Check(Pursuers>=2,TEXT("lift crash activates at least two live hearing pursuers"));}
   View(FVector(2900,0,90),FVector(3470,-180,135));Check(!Chapter->QTE(true,Player),TEXT("QTE cannot operate remotely"));
   View(FVector(3300,0,90),FVector(3470,-180,135));Player->ToggleInventory();Check(!Chapter->QTE(true,Player),TEXT("QTE cannot operate through backpack"));Player->ToggleInventory();
   for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){It->SetActorTickEnabled(false);It->GetCharacterMovement()->StopMovementImmediately();}
   View(FVector(3300,0,90),FVector(3470,-180,135));Check(!Chapter->QTE(false,Player) && Chapter->QTESteps==0,TEXT("wrong first QTE input cannot advance"));Capture(TEXT("07_JammedLift"));break;
 case 23:
   Check(Chapter->QTE(QTEIndex%2==0,Player),TEXT("alternating QTE input advances door"));
   Check(!Chapter->QTE(QTEIndex%2!=0,Player),TEXT("same-frame QTE spam cannot skip timing"));
   if(++QTEIndex<6)--Step;Next=Now+.4f;break;
 case 24:
   Check(Chapter->GetElevatorState()==ELZElevatorState::Open,TEXT("six QTE actions open lift without killing all enemies"));
   View(FVector(3650,0,90),FVector(4180,0,170));Next=Now+.8f;break;
 case 25:Check(Chapter->GetElevatorState()==ELZElevatorState::Descending,TEXT("physically entering lift closes doors and begins descent"));Capture(TEXT("08_InsideLift"));Next=Now+4.3f;break;
 case 26:
   Check(Chapter->Garage && Chapter->GetElevatorState()==ELZElevatorState::Basement,TEXT("elevator descent enters playable basement instead of ending the run"));
   Check(!GetWorld()->GetAuthGameMode<ALZGameMode>()->IsRunOver(),TEXT("basement begins with player control and preserved run"));
   Check(Player->GetActorLocation().Z < -2000,TEXT("player arrives on basement floor"));
   Capture(TEXT("09_BasementArrival"));break;
 case 27:UseNode(ELZChapterNode::Pistol);UseNode(ELZChapterNode::Ammo);Check(Player->HasFirearm() && Player->GetReserveAmmo()>=17,TEXT("garage security supplies usable pistol and ammunition"));
   View(Chapter->Garage->World(FVector(1000,0,90)),Chapter->Garage->Boss->GetActorLocation());Before=Chapter->Garage->Boss->GetActorLocation();Next=Now+1.2f;break;
 case 28:Check(Chapter->Garage->State==ELZGarageState::Pursuit && Chapter->Garage->Boss->bActive,TEXT("crossing garage aisle activates boss pursuit"));
   Check(FVector::Dist2D(Before,Chapter->Garage->Boss->GetActorLocation())>50,TEXT("active boss physically pursues player through garage aisle"));
   Chapter->Garage->Boss->SetActorTickEnabled(false);
   View(Chapter->Garage->Boss->GetActorLocation()-FVector(600,0,60),Chapter->Garage->Boss->GetActorLocation());BeforeHealth=Chapter->Garage->Boss->GetEnemyHealth();Player->QASelectFirearm();Next=Now+1;break;
 case 29:Player->QAFire();Check(Player->HasFirearm() && Player->GetAmmoInMagazine()==2 && Chapter->Garage->Boss->GetEnemyHealth()<BeforeHealth,TEXT("actual pistol shot damages and staggers boss"));Capture(TEXT("10_BossEncounter"));break;
 case 30:
   {auto* G=Chapter->Garage;UGameplayStatics::ApplyDamage(G->Boss,10000,Player->GetController(),Player,nullptr);Check(IsValid(G->Boss) && G->Boss->GetEnemyHealth()>=1,TEXT("small arms weaken boss but preserve vehicle finisher"));
   Check(!G->EnterVehicle(Player),TEXT("vehicle rejects remote boarding"));
   UseNode(ELZChapterNode::Vehicle);Check(G->State==ELZGarageState::Driving,TEXT("physical boarding switches to driving controls"));G->SetQAInput(1,0);}
   Next=Now+3;break;
 case 31:
   {auto* G=Chapter->Garage;const FVector P=G->Vehicle->GetActorLocation();
   FCollisionQueryParams Q(SCENE_QUERY_STAT(VehicleGroundQA),true,Player);Q.AddIgnoredActor(G->Vehicle);FHitResult H;
   if(GetWorld()->LineTraceSingleByChannel(H,P+FVector(0,0,250),P-FVector(0,0,1000),ECC_Visibility,Q))
   {UE_LOG(LogTemp,Display,TEXT("VEHICLE_GROUND_QA car=%s surface=%s clearance=%.2f"),*P.ToString(),*H.ImpactPoint.ToString(),P.Z-H.ImpactPoint.Z);
   Check(P.Z-H.ImpactPoint.Z>65 && P.Z-H.ImpactPoint.Z<110,TEXT("vehicle body maintains ground clearance on exit ramp"));}}
   Check(Chapter->Garage->bBossHit,TEXT("accelerating vehicle physically intersects and kills boss"));Capture(TEXT("11_VehicleImpact"));Next=Now+4.5f;break;
 case 32:Check(Chapter->Garage->State==ELZGarageState::CityReveal,TEXT("vehicle exits garage ramp into ruined-city reveal"));Check(Chapter->Garage->Vehicle->GetActorLocation().Z>-1500,TEXT("escape vehicle ascends to street elevation"));Capture(TEXT("12_RuinedCity"));Next=Now+7.5f;break;
 case 33:Check(GetWorld()->GetAuthGameMode<ALZGameMode>()->WasExtractionSuccessful(),TEXT("city panorama completes two-level escape"));Capture(TEXT("13_TwoLevelComplete"));break;
 case 40:
 {
  int32 Count=0;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){if(It->ActorHasTag(TEXT("LZDoorBCrowd")))++Count;It->SetActorTickEnabled(false);It->GetCharacterMovement()->StopMovementImmediately();}
  Check(Count==GetDefault<ULZStealthSettings>()->CrowdCount,TEXT("B uses the configured infected count"));
  View(FVector(565,730,90),FVector(565,864,135));Chapter->AdminDoor->Interact(Player);
  Check(Chapter->AdminDoor->IsClosed() && Chapter->GetFeedback().Contains(TEXT("需要门禁卡")),TEXT("B reader rejects exterior access without card"));break;
 }
 case 41:
  View(FVector(840,630,90),Chapter->AdminCard->GetActorLocation());Chapter->AdminCard->Interact(Player);
  Check(Chapter->HasAccess(TEXT("AdminAccess")),TEXT("security card grants reusable quest permission"));
  View(FVector(565,730,90),FVector(565,864,135));Chapter->AdminDoor->Interact(Player);Next=Now+1.5f;break;
 case 42:
  Check(Chapter->AdminDoor->IsOpen(),TEXT("B card opens complete animated portal"));View(FVector(300,1120,90),FVector(300,700,150));Capture(TEXT("15_DoorBAccess"));Step=10;break;
 case 46:
 {
   auto* C=Chapter->FindNode(ELZChapterNode::DoorC);View(FVector(3380,710,90),FVector(3380,900,150));if(C)C->Interact(Player);
   Check(C && !C->bOpen && !Chapter->bCUnlocked && Chapter->GetFeedback().Contains(TEXT("无法从这一侧打开")),TEXT("C rejects exterior opening with the requested Chinese feedback"));
   Capture(TEXT("16_DoorCLocked"));break;
 }
 case 47:
   UseNode(ELZChapterNode::DoorD);Check(Chapter->FindNode(ELZChapterNode::DoorD)->bOpen,TEXT("D opens normally from the admin corridor"));break;
 case 48:
 {
   FCollisionQueryParams Params(SCENE_QUERY_STAT(PowerRouteQA),false,Player);FHitResult Hit;
   for(auto R:TArray<TPair<FVector,FVector>>{{FVector(500,1150,90),FVector(2100,1150,90)},{FVector(2100,1150,90),FVector(2450,1150,90)}})
    Check(!GetWorld()->SweepSingleByChannel(Hit,R.Key,R.Value,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params),TEXT("archive corridor leads through D into electrical room"));
   View(FVector(2500,1350,90),FVector(2900,1720,88));Capture(TEXT("17_ElectricalFuse"));break;
 }
 case 49:
 {
   auto* C=Chapter->FindNode(ELZChapterNode::DoorC);View(FVector(3380,1110,90),FVector(3380,900,150));if(C)C->Interact(Player);
   Check(C && C->bOpen && Chapter->bCUnlocked,TEXT("C unlocks and opens from electrical-room side"));break;
 }
 case 50:
 {
   FCollisionQueryParams Params(SCENE_QUERY_STAT(PowerShortcutQA),false,Player);FHitResult Hit;
   Check(!GetWorld()->SweepSingleByChannel(Hit,FVector(3380,1110,90),FVector(3380,700,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params),TEXT("unlocked C is a physical shortcut back to elevator"));
   Capture(TEXT("18_DoorCShortcut"));Step=19;break;
 }
 default:Finish();break;
 }
#endif
}
void ALZChapterQA::Finish()
{
 const FString Summary=FString::Printf(TEXT("CHAPTER_QA SUMMARY %s assertions=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Assertions,Failures);Lines.Add(Summary);UE_LOG(LogTemp,Display,TEXT("%s"),*Summary);
 FFileHelper::SaveStringArrayToFile(Lines,*(FPaths::ProjectSavedDir()/TEXT("LZChapterQA_Report.txt")));SetActorTickEnabled(false);
 if(FParse::Param(FCommandLine::Get(),TEXT("LZQAExit")))FPlatformMisc::RequestExit(false);
}
