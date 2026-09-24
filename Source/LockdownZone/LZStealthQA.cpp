#include "LZStealthQA.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "LZHearingAI.h"
#include "LZAccessDoor.h"
#include "LZIntercom.h"
#include "LZStealthSettings.h"
#include "LZNoiseProjectile.h"
#include "LZVentNetwork.h"
#include "LZFlashlightPickup.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DamageEvents.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AIPerceptionComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
namespace {struct FContinuation {bool Restart=false;int32 Assertions=0,Failures=0;TArray<FString> Lines;} Run;}
ALZStealthQA::ALZStealthQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZStealthQA::BeginPlay()
{
 Super::BeginPlay();C=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));Started=GetWorld()->GetTimeSeconds();Next=Started+5;
 if(Run.Restart){bRestarted=true;Assertions=Run.Assertions;Failures=Run.Failures;Lines=Run.Lines;Step=90;}
}
void ALZStealthQA::Check(bool OK,const TCHAR* M){++Assertions;if(!OK)++Failures;Lines.Add(FString::Printf(TEXT("STEALTH_QA %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),M));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());}
void ALZStealthQA::View(FVector Position,FVector Target)
{
 P->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(Position);
 auto* Camera=P->FindComponentByClass<UCameraComponent>();const FRotator Aim=(Target-Camera->GetComponentLocation()).Rotation();Cast<APlayerController>(P->GetController())->SetControlRotation(Aim);Camera->SetWorldRotation(Aim);
}
void ALZStealthQA::Capture(const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Stealth_")+N+TEXT(".png"),true,false);}
void ALZStealthQA::Walk(TArray<FVector> Points,float Timeout,bool Crouch){Waypoints=Points;MoveDeadline=GetWorld()->GetTimeSeconds()+Timeout;if(Crouch)P->StartCrouch();else P->StopCrouch();}
void ALZStealthQA::Tick(float Delta)
{
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();
 if(!C || !P || Now-Started>230){Check(false,TEXT("stealth runtime within bounded test time"));Finish();return;}
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();auto* D=C->AdminDoor;auto* R=C->AdminRadio;
 if(Waypoints.Num())
 {
  if(Now>MoveDeadline){Check(false,TEXT("physical crouched route completed before timeout"));UE_LOG(LogTemp,Warning,TEXT("STEALTH_MOVE_BLOCK position=%s target=%s"),*P->GetActorLocation().ToString(),*Waypoints[0].ToString());Waypoints.Empty();Next=Now;}
  else {if(FVector::Dist2D(P->GetActorLocation(),Waypoints[0])<30)Waypoints.RemoveAt(0);if(Waypoints.Num()){P->AddMovementInput((Waypoints[0]-P->GetActorLocation()).GetSafeNormal2D());return;}P->GetCharacterMovement()->StopMovementImmediately();Next=Now+.1f;}
 }
 if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;Next=Now+.65f;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(StealthQA),false,P);FHitResult H;
 auto Brain=[](ALZEnemy* E){return E?Cast<ALZHearingController>(E->GetController()):nullptr;};
 switch(Step++)
 {
 case 0:
 {
  Check(D && R && C->AdminCard,TEXT("authored B door radio and guard card exist"));
  Check(D->IsClosed() && !C->HasAccess(TEXT("AdminAccess")) && R->IsPowered() && R->EmissionCount==0,TEXT("initial state is locked without permission and safe before arming"));
  for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)
  {
   if(It->ActorHasTag(TEXT("LZDoorBCrowd")))Crowd.Add(*It);
   else {It->SetActorTickEnabled(false);It->GetCharacterMovement()->StopMovementImmediately();}
  }
  Check(Crowd.Num()==3,TEXT("three infected validate the ground route"));
  for(auto* E:Crowd)Check(!E->HasHeardNoise() && E->GetVelocity().Size2D()<1,TEXT("unarmed opening crowd stays still"));
  View(FVector(565,730,90),FVector(565,864,135));D->Interact(P);
  Check(D->IsClosed() && C->GetFeedback()==TEXT("需要门禁卡"),TEXT("exterior B refuses no-card player with exact feedback"));
  View(FVector(650,-20,90),FVector(350,710,125));
  Check(GetWorld()->LineTraceSingleByChannel(H,P->GetActorLocation()+FVector(0,0,64),C->AdminCard->GetActorLocation(),ECC_Visibility,Q) && H.GetActor()==C->AdminCard,TEXT("safe observation has direct unobstructed sightline to guard card"));
  Check(!GetWorld()->SweepSingleByChannel(H,FVector(850,100,90),FVector(850,590,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("east approach to guard does not cross the northwest lure route"));
  C->bDoorBreached=true;R->NextCallAt=Now+.2f;Capture(TEXT("01_ObserveB"));Next=Now+3;break;
 }
 case 1:
 {
  Check(R->EmissionCount>0,TEXT("radio plays scheduled audible call and emits hearing event"));
  for(auto* E:Crowd){Check(Brain(E) && Brain(E)->GetLastCategory()==TEXT("Radio"),TEXT("initial gathering responds to actual radio event"));Check(!Brain(E)->Hearing->GetSenseConfig(UAISense::GetSenseID<UAISense_Sight>()),TEXT("crowd has no visual perception"));}
  View(FVector(650,-20,90),FVector(300,900,130));Next=Now+3.5f;break;
 }
 case 2:
 {
  bool Outside=true;for(auto* E:Crowd){Outside &= E->GetActorLocation().Y<890;Before.Add(E->GetActorLocation());}
  Check(Outside && D->KnockCount>0,TEXT("closed B holds infected outside with audible door taps"));
  Check(GetWorld()->SweepSingleByChannel(H,FVector(300,700,90),FVector(300,1100,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("closed B has real pawn collision"));
  P->TryStoreItem(ELZInventoryItemType::Scrap,2);CountBefore=P->GetThrowableCount();
  // Select a real ballistic throw, including scene collision, from the safe observation position.
  const FVector Eye=P->FindComponentByClass<UCameraComponent>()->GetComponentLocation(),Target(-600,0,5);const float Yaw=(Target-Eye).Rotation().Yaw;
  float Best=BIG_NUMBER;FRotator Aim=FRotator::ZeroRotator;
  for(float Pitch=5;Pitch<=65;Pitch+=1)
  {
   const FVector F=FRotator(Pitch,Yaw,0).Vector();FPredictProjectilePathParams Params;
   Params.StartLocation=Eye+F*55;Params.LaunchVelocity=F*1050+FVector(0,0,100);Params.ProjectileRadius=6;Params.MaxSimTime=3;Params.SimFrequency=30;Params.bTraceWithCollision=true;Params.bTraceWithChannel=true;Params.TraceChannel=ECC_Visibility;Params.ActorsToIgnore.Add(P);
   FPredictProjectilePathResult Result;
   if(UGameplayStatics::PredictProjectilePath(this,Params,Result) && Result.HitResult.ImpactPoint.Z<25)
   {const float Score=FVector::Dist2D(Target,Result.HitResult.ImpactPoint);if(Score<Best){Best=Score;Aim=FRotator(Pitch,Yaw,0);Remembered=Result.HitResult.ImpactPoint;}}
  }
  Check(Best<220,TEXT("northwest workstation landing is physically throwable from observation"));
  UE_LOG(LogTemp,Display,TEXT("STEALTH_THROW pitch=%.1f predicted=%s miss=%.1f"),Aim.Pitch,*Remembered.ToString(),Best);
  Cast<APlayerController>(P->GetController())->SetControlRotation(Aim);P->FindComponentByClass<UCameraComponent>()->SetWorldRotation(Aim);P->ThrowNoiseItem();
  for(TActorIterator<ALZNoiseProjectile> It(GetWorld());It;++It)Bottle=*It;
  Check(Bottle && P->GetThrowableCount()==CountBefore-1,TEXT("throw input consumes a real backup bottle and launches projectile"));Next=Now+2.4f;break;
 }
 case 3:
 {
  Check(Bottle && Bottle->Impacts>0 && FVector::Dist2D(Bottle->FirstImpact,FVector(-600,0,5))<240,TEXT("real bottle lands on reachable northwest office floor"));ImpactAt=Now-1;
  for(auto* E:Crowd)Check(Brain(E)->GetLastCategory()==TEXT("Bottle"),TEXT("bottle redirects every B infected to its impact location"));
  R->NextCallAt=Now+.1f; // stress-test a weak repeat during strong investigation commitment
  Walk({FVector(850,270,58),FVector(850,590,58),FVector(810,625,58)},6);break;
 }
 case 4:
 {
  bool Clear=true;for(int I=0;I<Crowd.Num();++I){auto* E=Crowd[I];Clear &= FVector::Dist2D(E->GetActorLocation(),D->GetActorLocation())>450;Check(FVector::Dist2D(Before[I],E->GetActorLocation())>250,TEXT("bottle makes infected relocate instead of merely turning"));Check(Brain(E)->GetLastCategory()==TEXT("Bottle"),TEXT("weak radio repeat does not cancel stronger bottle investigation"));}
  Check(Clear,TEXT("actual distraction clears B entrance and guard approach"));
  View(P->GetActorLocation(),C->AdminCard->GetActorLocation());
  Check(P->FindInteractable()==C->AdminCard,TEXT("normal player interaction ray can acquire corpse card"));
  CountBefore=P->GetUsedBagSlots();C->AdminCard->Interact(P);
  Check(C->HasAccess(TEXT("AdminAccess")) && !IsValid(C->AdminCard) && P->GetUsedBagSlots()==CountBefore,TEXT("single corpse interaction grants permanent permission without inventory slots"));
  Walk({FVector(590,725,58)},3);break;
 }
 case 5:
  View(P->GetActorLocation(),FVector(565,864,135));Check(P->FindInteractable()==D,TEXT("normal player interaction ray reaches external reader"));
  D->Interact(P);Check(D->State==ELZAccessDoorState::Opening,TEXT("exterior reader automatically uses card without equipping it"));Next=Now+1.3f;break;
 case 6:
  Check(D->IsOpen() && FMath::Abs(Now-D->GetOpenedAt())<.6f,TEXT("seven-second hold starts after complete animation"));
  Check(!GetWorld()->SweepSingleByChannel(H,FVector(300,700,90),FVector(300,1100,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("fully open B allows a full standing capsule"));
  Walk({FVector(350,810,58),FVector(350,1120,58)},4);break;
 case 7:
  Check(P->GetActorLocation().Y>1000 && P->GetHealth()==100,TEXT("live crouched card-swipe-entry route succeeds without taking damage"));
  UE_LOG(LogTemp,Display,TEXT("STEALTH_ENTRY_WINDOW seconds=%.2f"),Now-ImpactAt);
  View(P->GetActorLocation(),R->GetActorLocation());Capture(TEXT("02_InsideB"));Walk({FVector(1100,1120,58)},5);break;
 case 8:
  Check(!P->HasFlashlight() && !P->HasFirearm(),TEXT("B ground route does not require flashlight or pantry reward"));
  Next=Now+6;break;
 case 9:
  Check(D->IsClosed(),TEXT("unoccupied door automatically returns to closed state"));
  // Let the bottle search finish, then allow another ordinary call.
  R->NextCallAt=Now+9;Next=Now+13;break;
 case 10:
 {
  int Returning=0;for(auto* E:Crowd)if(Brain(E)->GetLastCategory()==TEXT("Radio"))++Returning;
  Check(Returning==Crowd.Num(),TEXT("powered radio re-attracts infected after bottle investigation"));
  C->AccessPermissions.Empty();View(FVector(565,1110,90),FVector(565,936,135));D->Interact(P);Next=Now+1.4f;break;
 }
 case 11:
  Check(D->IsOpen() && !C->HasAccess(TEXT("AdminAccess")),TEXT("inside release works without any card permission"));
  View(FVector(1100,1120,90),FVector(700,1240,95));R->NextCallAt=Now+.1f;Next=Now+4.5f;break;
 case 12:
 {
  bool Inside=false;for(auto* E:Crowd)Inside |= E->GetActorLocation().Y>980;
  Check(Inside,TEXT("open B permits actual AI navigation into administrative corridor"));
  // Isolate close-door safety from combat while leaving every actor physically present.
  for(auto* E:Crowd){E->SetActorTickEnabled(false);E->GetCharacterMovement()->StopMovementImmediately();}
  C->GrantAccess(TEXT("AdminAccess"));View(FVector(300,905,90),FVector(700,1240,95));D->Interact(P);Next=Now+8.6f;break;
 }
 case 13:
  Check(D->IsOpen() && P->GetVelocity().Size()<2,TEXT("player in doorway pauses auto-close without displacement"));
  View(FVector(1100,1100,90),FVector(300,900,140));Probe=Crowd[0];Probe->SetActorLocation(FVector(300,900,90));Next=Now+2;break;
 case 14:
  Check(D->IsOpen() && FVector::Dist2D(Probe->GetActorLocation(),FVector(300,900,90))<2,TEXT("infected in doorway also pauses close without being pushed"));
  for(auto* E:Crowd)E->SetActorLocation(FVector(1400+Crowd.IndexOfByKey(E)*150,1100,90));
  Next=Now+.6f;break;
 case 15:
  Check(D->IsOpen(),TEXT("cleared doorway waits debounce delay instead of jitter-closing"));Next=Now+2.1f;break;
 case 16:
  Check(D->IsClosed(),TEXT("door closes once safety area stays clear"));
  for(auto* E:Crowd)Check(E->GetActorLocation().Y>1000 && IsValid(E),TEXT("closing B never teleports or removes followers already inside"));
  View(FVector(565,730,90),FVector(565,864,135));D->Interact(P);Next=Now+1.4f;break;
 case 17:
 {
  Check(D->IsOpen(),TEXT("collected card remains reusable after automatic close"));
  View(FVector(690,1090,90),R->GetActorLocation());
  Check(P->FindInteractable()==R,TEXT("normal player interaction ray reaches the desk radio"));
  Capture(TEXT("05_RadioCloseup"));Remembered=Brain(Crowd[0])->GetLastSound();const bool HadMemory=Brain(Crowd[0])->HasSoundMemory();
  CallsBefore=R->EmissionCount;R->Interact(P);
  Check(!R->IsPowered() && !R->IsPlaying(),TEXT("nearby direct interaction silences radio audio immediately"));
  Check(Brain(Crowd[0])->HasSoundMemory()==HadMemory && Brain(Crowd[0])->GetLastSound()==Remembered,TEXT("turning off radio preserves existing AI memory and search state"));
  R->NextCallAt=Now+.1f;Next=Now+1;break;
 }
 case 18:
  Check(R->EmissionCount==CallsBefore,TEXT("powered-off radio emits no subsequent hearing event"));
  // Ground branch reaches 06, D, fuse and C without picking up flashlight or optional gun.
  View(FVector(500,1150,90),FVector(500,1350,150));C->FindNode(ELZChapterNode::ArchiveDoor)->Interact(P);
  Check(C->FindNode(ELZChapterNode::ArchiveDoor)->bOpen && IsValid(C->ArchiveFlashlight),TEXT("ground player can open ordinary 06 door and find existing flashlight"));
  Walk({FVector(1800,1150,58),FVector(2040,1150,58)},10);break;
 case 19:
  C->FindNode(ELZChapterNode::DoorD)->Interact(P);Check(C->FindNode(ELZChapterNode::DoorD)->bOpen,TEXT("D opens normally from ground administrative route"));Walk({FVector(2420,1150,58),FVector(2740,1650,58)},7);break;
 case 20:
 {
  int Enemies=0;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)++Enemies;
  auto* Fuse=C->FindNode(ELZChapterNode::Fuse);Fuse->Interact(P);Check(C->bFuse && !P->HasFlashlight() && !P->HasFirearm(),TEXT("main fuse acquired with neither flashlight nor pantry reward"));
  int After=0;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)++After;Check(Enemies==After,TEXT("taking fuse neither spawns enemies nor triggers pursuit"));
  Walk({FVector(3240,1600,58),FVector(3380,1100,58)},8);break;
 }
 case 21:
  C->FindNode(ELZChapterNode::DoorC)->Interact(P);Check(C->bCUnlocked && C->FindNode(ELZChapterNode::DoorC)->bOpen,TEXT("C unlocks only from interior as a return shortcut"));Walk({FVector(3380,650,58),FVector(3300,230,58)},6);break;
 case 22:
  C->FindNode(ELZChapterNode::Breaker)->Interact(P);Check(C->bFuseInstalled,TEXT("ground route returns to original spark panel and restores elevator"));
  Check(C->VentNetwork && C->VentNetwork->Accesses.Num()==2 && ALZVentNetwork::Outlets().Num()==1,TEXT("meeting-to-archive alternative remains intact"));
  View(P->GetActorLocation(),FVector(3470,230,160));Capture(TEXT("03_GroundRouteComplete"));
  // Isolated contact / obstruction checks use one still living actor, never a fake attack event.
  Probe=Crowd[0];Probe->SetActorTickEnabled(true);Probe->SetActorLocation(FVector(300,860,90));Brain(Probe)->SetState(ELZHearingState::Idle);
  for(int I=1;I<Crowd.Num();++I)Crowd[I]->SetActorTickEnabled(false);
  P->StopCrouch();View(FVector(300,950,90),FVector(300,860,90));BeforeHealth=P->GetHealth();Next=Now+1.5f;break;
 case 23:
  Check(D->IsClosed() && P->GetHealth()==BeforeHealth,TEXT("near contact cannot attack player through closed B"));
  View(FVector(300,750,90),Probe->GetActorLocation());Next=Now+1.5f;break;
 case 24:
  Check(P->GetHealth()<BeforeHealth,TEXT("silent close body contact remains dangerous without sight"));
  View(FVector(0,-100,90),Probe->GetActorLocation());Brain(Probe)->SetState(ELZHearingState::Idle);Before={Probe->GetActorLocation()};Next=Now+1.5f;break;
 case 25:
  Check(!Probe->HasHeardNoise() && FVector::Dist2D(Before[0],Probe->GetActorLocation())<2,TEXT("quiet visible player alone does not create pursuit target"));
  C->Noise(FVector(-650,0,90),3200,TEXT("BottleBreak"),TEXT("Bottle"));Next=Now+1.3f;break;
 case 26:
  Remembered=Probe->GetHeardLocation();View(FVector(1000,-300,90),Probe->GetActorLocation());Next=Now+1.5f;break;
 case 27:
  Check(Probe->GetHeardLocation()==Remembered && FVector::Dist2D(Remembered,P->GetActorLocation())>1000,TEXT("one sound never becomes permanent live player tracking"));
  Probe->SearchDuration=1;Brain(Probe)->SetState(ELZHearingState::Search);Next=Now+1.8f;break;
 case 28:
  Check(!Probe->HasHeardNoise(),TEXT("finite search forgets old position and releases alert"));
  Probe->SetActorTickEnabled(false);Probe->GetCharacterMovement()->StopMovementImmediately();Probe->SetActorLocation(FVector(2200,200,90));Brain(Probe)->SetState(ELZHearingState::Idle);
  View(FVector(1800,200,90),FVector(1000,200,100));Next=Now+1.2f;break;
 case 29:
  Check(!Probe->HasHeardNoise(),TEXT("stationary view rotation produces no footstep stimulus"));
  Walk({FVector(1800,0,58)},3);break;
 case 30:
  Check(!Probe->HasHeardNoise(),TEXT("actual crouched movement stays unheard at four metres"));
  Walk({FVector(1800,200,90)},3,false);break;
 case 31:
  Check(Brain(Probe)->GetLastCategory()==TEXT("Walk"),TEXT("actual ordinary movement is heard at nearby range"));
  Brain(Probe)->SetState(ELZHearingState::Idle);View(FVector(1300,200,90),FVector(1800,200,100));P->StartSprint();Walk({FVector(1600,200,90)},3,false);break;
 case 32:
  Check(Brain(Probe)->GetLastCategory()==TEXT("Sprint"),TEXT("actual sprint is heard beyond normal walking radius"));P->StopSprint();
  UGameplayStatics::ApplyDamage(P,10000,nullptr,this,nullptr);Next=Now+.8f;break;
 case 33:
  Check(GM->IsRunOver() && !R->IsPlaying() && Probe->GetVelocity().Size()<1,TEXT("death halts radio playback and AI movement"));
  Run.Restart=true;Run.Assertions=Assertions;Run.Failures=Failures;Run.Lines=Lines;GM->RestartRun();SetActorTickEnabled(false);break;
 case 90:
 {
  Check(!GM->IsRunOver() && P->GetHealth()==100 && !C->HasAccess(TEXT("AdminAccess")),TEXT("real level restart clears death and task-card permission"));
  Check(D->IsClosed() && IsValid(C->AdminCard) && R->IsPowered() && R->EmissionCount==0,TEXT("restart restores closed door corpse card and powered silent opening radio"));
  int N=0;bool Reset=true;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){if(It->ActorHasTag(TEXT("LZDoorBCrowd")))++N;Reset &= !It->HasHeardNoise() && It->GetVelocity().Size()<1;}
  Check(N==3 && Reset && !GM->IsCombatUnlocked(),TEXT("restart restores authored crowd and unarmed safe state"));Capture(TEXT("04_Restarted"));break;
 }
 default:Finish();break;
 }
}
void ALZStealthQA::Finish()
{
 Lines.Add(FString::Printf(TEXT("STEALTH_QA SUMMARY %s assertions=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Assertions,Failures));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());
 FFileHelper::SaveStringArrayToFile(Lines,*(FPaths::ProjectSavedDir()/TEXT("LZStealthQA_Report.txt")));SetActorTickEnabled(false);FPlatformMisc::RequestExit(false);
}
