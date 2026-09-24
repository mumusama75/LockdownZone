#include "LZOpeningQA.h"
#include "LZChapter.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "LZOfficeOpening.h"
#include "LZOpeningSettings.h"
#include "LZEnemy.h"
#include "LZHearingAI.h"
#include "LZAccessDoor.h"
#include "LZIntercom.h"
#include "LZVentNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace {struct FOpeningContinuation {bool Restart=false;int Assertions=0,Failures=0;TArray<FString> Lines;} Run;}
ALZOpeningQA::ALZOpeningQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZOpeningQA::BeginPlay()
{
 Super::BeginPlay();C=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));Started=GetWorld()->GetTimeSeconds();Next=Started+6;
 if(Run.Restart){Assertions=Run.Assertions;Failures=Run.Failures;Lines=Run.Lines;Step=30;}
}
void ALZOpeningQA::Check(bool OK,const TCHAR* T){++Assertions;if(!OK)++Failures;Lines.Add(FString::Printf(TEXT("OPENING_QA %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),T));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());}
void ALZOpeningQA::Shot(const TCHAR* T){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Opening_")+T+TEXT(".png"),true,false);}
void ALZOpeningQA::Look(FVector Target){auto* Camera=P->FindComponentByClass<UCameraComponent>();const auto R=(Target-Camera->GetComponentLocation()).Rotation();Cast<APlayerController>(P->GetController())->SetControlRotation(R);Camera->SetWorldRotation(R);}
void ALZOpeningQA::Walk(TArray<FVector> Points){Path=Points;MoveDeadline=GetWorld()->GetTimeSeconds()+15;}
void ALZOpeningQA::Finish(){Lines.Add(FString::Printf(TEXT("OPENING_QA assertions=%d failures=%d"),Assertions,Failures));FFileHelper::SaveStringToFile(FString::Join(Lines,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("LZOpeningQA_Report.txt")));FPlatformMisc::RequestExit(false);SetActorTickEnabled(false);}
void ALZOpeningQA::Tick(float Delta)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(!C || !P || Now-Started>100){Check(false,TEXT("opening completes within timeout"));Finish();return;}
 if(Path.Num())
 {
  if(Now>MoveDeadline){Check(false,TEXT("physical walking path completed"));UE_LOG(LogTemp,Warning,TEXT("OPENING_BLOCK at=%s target=%s"),*P->GetActorLocation().ToString(),*Path[0].ToString());Path.Empty();}
  else{if(FVector::Dist2D(P->GetActorLocation(),Path[0])<25)Path.RemoveAt(0);if(Path.Num()){P->AddMovementInput((Path[0]-P->GetActorLocation()).GetSafeNormal2D());return;}P->GetCharacterMovement()->StopMovementImmediately();Next=Now+.2f;}
 }
 if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;Next=Now+.6f;
 switch(Step++)
 {
 case 0:
  Door=C->FindNode(ELZChapterNode::DoorLock);
  Check(C->Opening && Door && C->FindNode(ELZChapterNode::Crowbar),TEXT("one opening controller, jammed exit and direct crowbar pickup"));
  Check(!C->FindNode(ELZChapterNode::Desk) && !C->FindNode(ELZChapterNode::EmptyDesk) && !C->FindNode(ELZChapterNode::Cabinet),TEXT("opening has no key drawer or axe cabinet"));
  Check(!P->HasCrowbarTool() && P->GetUsedBagSlots()==0,TEXT("spawn owns no tool and has empty backpack"));
  for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)if(FVector::Dist2D(It->GetActorLocation(),GetDefault<ULZOpeningSettings>()->TeachingInfected)<100)E=*It;
  Check(E && !E->HasHeardNoise(),TEXT("existing teaching infected remains still before tool acquisition"));
  if(!E){Finish();return;}
  EnemyStart=E?E->GetActorLocation():FVector::ZeroVector;
  Shot(TEXT("01_Spawn"));Walk({FVector(-2880,-180,90),FVector(-2780,-590,90)});break;
 case 1:
  Check(FVector::Dist2D(P->GetActorLocation(),FVector(-2780,-590,90))<35,TEXT("spawn-to-exit has clear physical walking path"));
  Look(Door->GetActorLocation());break;
 case 2:
  Check(P->FindInteractable()==Door,TEXT("jammed exit identified by actual first-person trace"));Door->Interact(P);
  Check(!C->bDoorBreached && !C->Opening->IsPrying() && C->GetFeedback().Contains(TEXT("门框变形")),TEXT("no-tool check gives jam feedback without unlocking"));
  for(int I=0;I<8;++I)Door->Interact(P);
  Check(!C->Opening->IsPrying() && !E->HasHeardNoise(),TEXT("repeated checks do not start pry or alert infected"));
  Look(GetDefault<ULZOpeningSettings>()->RepairBench+FVector(0,0,9));break;
 case 3:Shot(TEXT("02_DoorToCrowbar"));break;
 case 4:
  Check(P->FindInteractable()==C->FindNode(ELZChapterNode::Crowbar),TEXT("small turn from door targets exposed crowbar on repair bench"));
  if(auto* Tool=P->FindInteractable())Tool->Interact(P);
  Check(P->HasCrowbarTool() && C->bCrowbar && P->GetSelectedWeapon()==EPlayerWeapon::Crowbar,TEXT("one direct pickup grants persistent crowbar and equips it"));
  Check(P->GetUsedBagSlots()==0 && !C->FindNode(ELZChapterNode::Crowbar),TEXT("crowbar consumes zero bag cells and removes world pickup"));
  Look(Door->GetActorLocation());Door->Interact(P);
  Check(C->Opening->IsPrying() && !C->bDoorBreached,TEXT("interaction starts timed pry and keeps exit closed"));
  P->ToggleInventory();Check(!P->IsInventoryOpen(),TEXT("pry cannot be interrupted by inventory input"));
  Step=50;Next=Now+GetDefault<ULZOpeningSettings>()->PrySeconds*.4f;break;
 case 50:
  Shot(TEXT("02b_Prying"));Step=5;Next=Now+GetDefault<ULZOpeningSettings>()->PrySeconds*.6f+.15f;break;
 case 5:
  Check(C->bDoorBreached && !C->Opening->IsPrying() && !P->IsTraversing(),TEXT("pry completes once and releases movement"));
  Check(P->GetHealth()==100,TEXT("no unavoidable hit during pry"));
  Check(E && E->HasHeardNoise(),TEXT("synchronous metal impact reaches teaching infected through hearing"));
  if(E){auto* AI=Cast<ALZHearingController>(E->GetController());Check(AI && AI->GetLastCategory()==TEXT("MetalImpact") && FVector::Dist2D(AI->GetLastSound(),Door->GetActorLocation())<1,TEXT("AI investigates door snapshot, not player position"));}
  {int B=0;bool Untouched=true;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)if(It->GetActorLocation().X>-500 && It->GetActorLocation().X<700 && It->GetActorLocation().Y>400){++B;auto* AI=Cast<ALZHearingController>(It->GetController());Untouched&=AI && AI->GetLastCategory()!=TEXT("MetalImpact");}Check(B==3 && Untouched,TEXT("door sound leaves all three B infected in their own encounter"));}
  Door->Interact(P);Check(C->Opening->SuccessfulPrys==1,TEXT("repeat interaction cannot reaward or replay strong sound"));
  Look(E->GetActorLocation()+FVector(0,0,60));Next=Now+1.0f;break;
 case 6:
  Check(FVector::Dist2D(EnemyStart,E->GetActorLocation())>30,TEXT("infected turns then physically approaches heard position"));
  Look(E->GetActorLocation()+FVector(0,0,45));
  UE_LOG(LogTemp,Display,TEXT("OPENING_VIEW player=%s enemy=%s camera=%s control=%s"),*P->GetActorLocation().ToString(),*E->GetActorLocation().ToString(),*P->FindComponentByClass<UCameraComponent>()->GetComponentLocation().ToString(),*P->GetControlRotation().ToString());
  {FCollisionQueryParams Q(SCENE_QUERY_STAT(OpeningPassage),false,P);FHitResult H;Check(!GetWorld()->SweepSingleByChannel(H,FVector(-2780,-610,90),FVector(-2360,-610,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("open doorway passes full standing capsule"));}
  Step=60;Next=Now+.1f;break;
 case 60:Shot(TEXT("03_AfterPryEnemy"));Step=61;Next=Now+.1f;break;
 case 61:Walk({FVector(-2380,-600,90),FVector(-2320,-180,90),FVector(-2240,0,90)});Step=7;break;
 case 7:
  Check(P->GetActorLocation().X>-2400 && P->GetActorLocation().Y>-240,TEXT("player can leave office without killing teaching infected"));
  Check(IsValid(E),TEXT("teaching infected persists; no kill gate or forced deletion"));
  {auto* Nav=UNavigationSystemV1::GetCurrent(GetWorld());auto* Route=Nav?UNavigationSystemV1::FindPathToLocationSynchronously(this,FVector(-2360,-610,90),FVector(-2780,-610,90)):nullptr;Check(Route && Route->IsValid() && !Route->IsPartial(),TEXT("updated navigation connects both sides of opened office door"));}
  P->AcquireWeapon(EPlayerWeapon::Firearm);P->AcquireFlashlight();P->ToggleFlashlight();P->SyncEquippedGear();P->QASelectMelee();
  Check(P->HasCrowbarTool() && P->GetSelectedWeapon()==EPlayerWeapon::Crowbar,TEXT("weapon/flashlight switching retains permanent tool and melee slot"));
  Check(C->AdminDoor && C->VentNetwork->Accesses.Num()==2 && C->FindNode(ELZChapterNode::Fuse)->GetActorLocation().Y>1500,TEXT("B, 10-to-06 ventilation and electrical fuse remain present"));
  // Isolate contact geometry only after the live escape test; use the actual weapon trace.
  E->SetActorTickEnabled(false);E->GetCharacterMovement()->StopMovementImmediately();E->SetActorLocation(FVector(1000,0,90));P->SetActorLocation(FVector(800,0,90));Look(E->GetActorLocation()+FVector(0,0,25));
  {const float Health=E->GetEnemyHealth();P->QAFire();Check(FMath::IsNearlyEqual(E->GetEnemyHealth(),Health-18),TEXT("equipped crowbar actual attack trace applies existing 18 damage"));}
  UGameplayStatics::ApplyDamage(P,1000,nullptr,this,nullptr);Next=Now+.5f;break;
 case 8:
  Check(GM->IsRunOver(),TEXT("death ends active run"));Run={true,Assertions,Failures,Lines};GM->RestartRun();SetActorTickEnabled(false);break;
 case 30:
  Door=C->FindNode(ELZChapterNode::DoorLock);
  Check(!C->bDoorBreached && !C->bCrowbar && !P->HasCrowbarTool() && C->Opening->SuccessfulPrys==0,TEXT("real level restart resets jammed exit and permanent tool permission"));
  {int Tools=0,Enemies=0;for(TActorIterator<ALZChapterNode> It(GetWorld());It;++It)if(It->Kind==ELZChapterNode::Crowbar)++Tools;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)++Enemies;Check(Tools==1 && Enemies==6,TEXT("restart creates one tool and original six infected without duplication"));}
  Walk({FVector(-2880,-180,90),FVector(-2780,-590,90)});break;
 case 31:
  for(int I=0;I<36;++I)P->TryStoreItem(ELZInventoryItemType::Ammo,30);
  Check(P->GetUsedBagSlots()==36,TEXT("early-pickup fixture fills all ordinary inventory cells"));
  Look(C->FindNode(ELZChapterNode::Crowbar)->GetActorLocation());break;
 case 32:
  if(auto* Tool=P->FindInteractable())Tool->Interact(P);
  Check(P->HasCrowbarTool(),TEXT("early pickup works without checking door first"));
  Check(P->GetUsedBagSlots()==36,TEXT("full backpack neither prevents pickup nor loses permanent tool"));
  Look(Door->GetActorLocation());Door->Interact(P);Next=Now+GetDefault<ULZOpeningSettings>()->PrySeconds+.2f;break;
 case 33:Check(C->bDoorBreached && !C->bKey && !C->bAxe,TEXT("early pickup opens exit without key or axe permission"));Finish();break;
 default:Finish();break;
 }
#endif
}
