#include "LZGarageEscapeQA.h"
#include "LZGarageSlice.h"
#include "LZPickupTruck.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZHearingAI.h"
#include "LZAcoustics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Engine/StaticMeshActor.h"
namespace {bool Retried=false;TArray<FString> PreviousLines;int PreviousCount=0,PreviousFailures=0;}
ALZGarageEscapeQA::ALZGarageEscapeQA(){PrimaryActorTick.bCanEverTick=true;bMechanics=FParse::Param(FCommandLine::Get(),TEXT("GarageMechanicsQA"));if(bMechanics)Step=100;if(FParse::Param(FCommandLine::Get(),TEXT("GarageGuidanceQA")))Step=200;}
void ALZGarageEscapeQA::Check(bool Good,const TCHAR* T){++Assertions;if(!Good)++Failures;Lines.Add(FString::Printf(TEXT("GARAGE_QA %s %s"),Good?TEXT("PASS"):TEXT("FAIL"),T));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());}
void ALZGarageEscapeQA::Shot(const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Garage_")+N+TEXT(".png"),true,false);}
void ALZGarageEscapeQA::Finish(){Lines.Add(FString::Printf(TEXT("GARAGE_QA assertions=%d failures=%d"),Assertions,Failures));FFileHelper::SaveStringToFile(FString::Join(Lines,TEXT("\n")),*(FPaths::ProjectSavedDir()/(bMechanics?TEXT("GarageMechanicsQA.txt"):TEXT("GarageEscapeQA.txt"))));if(Trace.Num())FFileHelper::SaveStringToFile(FString::Join(Trace,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("GarageDriveTrace.csv")));FPlatformMisc::RequestExit(false);SetActorTickEnabled(false);}
bool ALZGarageEscapeQA::Follow(float Delta){
 auto* T=G->Truck;const FVector Pos=T->GetActorLocation();
 if(Point>=Route.Num()){T->SetTestInput(0,0,true);return true;}
 FVector Target=Route[Point];Target.Z=Pos.Z;
 if(FVector::Dist2D(Pos,Target)<390){++Point;return false;}
 const float Error=FMath::FindDeltaAngleDegrees(T->GetActorRotation().Yaw,(Target-Pos).Rotation().Yaw);
 const float SpeedTarget=FMath::Abs(Error)>45?330:Pos.X>7600?1250:650;
 const float Input=T->Speed()>SpeedTarget+80?-.35f:T->Speed()<SpeedTarget?1.f:.2f;
 T->SetTestInput(Input,FMath::Clamp(Error/32.f,-1.f,1.f));
 if(Trace.Num()%120==0){UE_LOG(LogTemp,Display,TEXT("DRIVE step=%d node=%d pos=%s speed=%.1f yaw=%.1f"),Step,Point,*Pos.ToCompactString(),T->Speed(),T->GetActorRotation().Yaw);FFileHelper::SaveStringToFile(FString::Join(Trace,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("GarageDriveTrace.csv")));}
 Trace.Add(FString::Printf(TEXT("%.2f,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%d"),GetWorld()->GetTimeSeconds(),Step,Pos.X,Pos.Y,Pos.Z,T->Speed(),T->GetActorRotation().Yaw,T->GroundWheels));
 return false;
}
void ALZGarageEscapeQA::Tick(float Delta){
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();if(!G)G=GetWorld()->GetAuthGameMode<ALZGameMode>()->GarageSlice;if(!G || !G->Player || Now<Next)return;
 auto* T=G->Truck;auto* P=G->Player;auto* PC=Cast<APlayerController>(P->GetController());
 if(Step>=200){GuidanceTick(Delta);return;}
 if(Now>360){Check(false,TEXT("driving scenario timeout"));Finish();return;}
 if(Step==0 && Retried){Lines=PreviousLines;Assertions=PreviousCount;Failures=PreviousFailures;Step=90;}
 switch(Step){
 case 0:
  Check(ALZGarageSlice::IsMap(this),TEXT("independent map active"));
  {FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(GarageGapTest),false,P);
  Check(GetWorld()->SweepSingleByChannel(Hit,FVector(4640,850,48),FVector(4950,850,48),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,48),Q),TEXT("initial shutter gap blocks actual crouched capsule"));
  Check(GetWorld()->SweepSingleByChannel(Hit,FVector(4400,850,110),FVector(5100,850,110),FQuat::Identity,ECC_Vehicle,FCollisionShape::MakeBox(FVector(235,102,35)),Q),TEXT("initial shutter gap blocks vehicle chassis"));
  Check(FMath::IsNearlyEqual(G->Shutter->GetActorLocation().Z-200,28,.1),TEXT("shutter visible gap matches collider transform"));}
Check(G->Phase==ELZGaragePhase::Preparation && !G->Boss->Active,TEXT("arrival begins with dormant boss"));
  Check(T->GroundWheels==4 && T->GetActorUpVector().Z>.95f,TEXT("four-wheel suspension settles upright"));Shot(TEXT("01_Arrival"));Step=1;Next=Now+.5f;break;
 case 1:{
  P->SetActorLocation(T->DoorPosition()+FVector(-100,-120,0));const auto Aim=(T->DoorPosition()-P->FindComponentByClass<UCameraComponent>()->GetComponentLocation()).Rotation();PC->SetControlRotation(Aim);P->FindComponentByClass<UCameraComponent>()->SetWorldRotation(Aim);
  Check(P->FindInteractable()==T,TEXT("normal interaction trace identifies half-open driver door"));
  Check(T->DoorAngle>34,TEXT("initial driver door half open"));Check(T->Enter(P),TEXT("enter pickup before encounter"));Step=2;Next=Now+1;break;}
 case 2:
  {const float YawBefore=T->GetActorRotation().Yaw;T->LookYaw(1000);T->LookPitch(-1000);
  auto R=T->DriveCamera->GetRootComponent()->GetRelativeRotation();Check(FMath::IsNearlyEqual(R.Yaw,85,1)&&FMath::IsNearlyEqual(R.Pitch,27,1),TEXT("driver look clamps right and up"));
  Check(FMath::IsNearlyEqual(YawBefore,T->GetActorRotation().Yaw,.01f),TEXT("driver looking does not steer chassis"));
  T->LookYaw(-1000);T->LookPitch(1000);R=T->DriveCamera->GetRootComponent()->GetRelativeRotation();Check(FMath::IsNearlyEqual(R.Yaw,-85,1)&&FMath::IsNearlyEqual(R.Pitch,-28,1),TEXT("driver look clamps left and down"));
  T->LookPitch(-25/1.5f);Shot(TEXT("08_DriverLookLeft"));}Step=20;Next=Now+.5f;break;
 case 20:
  T->LookYaw(85/1.5f);
  Check(T->Driving && T->DoorAngle<1,TEXT("door closes around hinge before throttle unlock"));Shot(TEXT("02_Driver"));
  Route={FVector(1100,-430,0),FVector(1800,-550,0),FVector(3300,-550,0),FVector(3980,-150,0),FVector(4090,650,0),FVector(3300,900,0),FVector(1700,900,0),FVector(1000,500,0)};Point=0;StageAt=Now;Step=3;break;
 case 3:
  if(Follow(Delta)){Check(Now-StageAt<75,TEXT("physical vehicle completes clockwise outer loop"));Step=4;Next=Now+2;}
  else if(Now-StageAt>75){Check(false,TEXT("clockwise loop blocked"));T->SetTestInput(0,0,true);Shot(TEXT("LoopBlocked"));Step=4;Next=Now+2;}break;
 case 4:
  Check(!G->Boss->Active,TEXT("test driving engine does not activate encounter"));Parked=T->GetActorLocation();Check(T->Exit(),TEXT("stationary vehicle finds safe exit"));Step=5;Next=Now+.5f;break;
 case 5:
  Check(!T->Driving && P->GetActorEnableCollision() && PC->GetViewTarget()==P,TEXT("exit restores player collision and camera"));
  P->SetActorLocation(T->DoorPosition()+FVector(0,0,5));Check(T->Enter(P),TEXT("reenter at moved vehicle location"));
  Check(T->DriveCamera->GetRootComponent()->GetRelativeRotation().Equals(FRotator(-3,0,0),.01f),TEXT("reenter resets driver view forward"));
  // Independent opposite-direction fixture: setup only; route itself uses wheel forces and controls.
  T->SetActorLocationAndRotation(FVector(1250,850,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);T->Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);T->Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
  Route={FVector(1400,880,0),FVector(3100,880,0),FVector(4150,500,0),FVector(4000,-450,0),FVector(3200,-620,0),FVector(1600,-620,0),FVector(950,-150,0),FVector(1000,500,0),FVector(1700,850,0),FVector(2200,800,0),FVector(2450,200,0),FVector(2650,-400,0),FVector(3400,-550,0),FVector(4000,-150,0),FVector(4250,450,0),FVector(4600,850,0)};Point=0;StageAt=Now;Step=6;break;
 case 6:
  if(Follow(Delta)){Check(true,TEXT("counterclockwise full loop and central 7 m passage driven"));Step=7;Next=Now+2;}
  else if(Now-StageAt>65){Check(false,TEXT("counterclockwise or central passage blocked"));T->SetTestInput(0,0,true);Step=7;Next=Now+2;}break;
 case 7:
  Check(T->Exit(),TEXT("second safe exit"));Parked=T->GetActorLocation();P->SetActorLocation(FVector(4650,90,100));G->DutyDoor->Interact(P);Step=70;Next=Now+2;break;
 case 70:
  Check(G->DutyOpen && !P->IsPrying(),TEXT("booth crowbar action clears chair and returns control"));P->SetActorLocation(FVector(5220,270,100));G->Button->Interact(P);G->Button->Interact(P);Step=8;Next=Now+5.2f;break;
 case 8:
  Check(G->GateActivations==1 && G->Phase==ELZGaragePhase::Encounter,TEXT("exit activation is one-shot"));Check(FVector::Dist(T->GetActorLocation(),Parked)<30,TEXT("opening exit does not reset moved vehicle"));Check(G->Boss->Active && !G->Shutter->GetActorEnableCollision(),TEXT("boss active and shutter fully clear"));
  Shot(TEXT("03_Gate"));
  P->SetActorLocation(T->DoorPosition()+FVector(0,0,10));Check(T->Enter(P),TEXT("return to actual parked truck"));
  Route={FVector(5350,850,0),FVector(6500,850,0),FVector(7800,850,0),FVector(12000,1100,0),FVector(24000,1550,0),FVector(29500,1550,0),FVector(48300,850,0)};Point=0;StageAt=Now;Step=9;break;
 case 9:
  if(Point==2 && !bRampShot){Shot(TEXT("04_Ramp"));bRampShot=true;}
  if(Follow(Delta)){Check(G->Phase==ELZGaragePhase::POI,TEXT("no gun and no mandatory boss kill escape reaches POI"));Shot(TEXT("05_POI"));Step=10;Next=Now+.5f;}
  else if(Now-StageAt>95){Check(false,TEXT("ramp and street drive timeout"));Shot(TEXT("EscapeBlocked"));Step=10;Next=Now+.5f;}break;
 case 10:
  Retried=true;PreviousLines=Lines;PreviousCount=Assertions;PreviousFailures=Failures;
  if(Trace.Num())FFileHelper::SaveStringToFile(FString::Join(Trace,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("GarageDriveTrace.csv")));G->Retry();SetActorTickEnabled(false);break;
 case 100:
  Check(!G->Boss->Active && !T->Driving,TEXT("direct-to-duty route requires no prior vehicle entry"));
  LZAcoustics::Emit(T,T->GetActorLocation(),8000,TEXT("Motor"),TEXT("Engine"));Step=101;Next=Now+.5f;break;
 case 101:
  Check(!G->Boss->Active && !G->Boss->HasHeardNoise(),TEXT("dormant boss rejects pre-encounter engine sound"));
  P->SetActorLocation(FVector(4650,90,100));G->DutyDoor->Interact(P);Step=120;Next=Now+2;break;
 case 120:P->SetActorLocation(FVector(5220,270,100));G->Button->Interact(P);Step=102;Next=Now+5.2f;break;
 case 102:
  Check(G->Phase==ELZGaragePhase::Encounter && !T->Driving,TEXT("walking straight to button activates open escape route"));
  G->Boss->SetActorLocation(FVector(2005,-620,130));G->Boss->SetActorRotation(FRotator(0,90,0));G->Boss->NextCharge=Now+100;G->Boss->BeginCharge(FVector(2005,850,130));Step=103;Next=Now+2.2f;break;
 case 103:
  Check(G->Boss->ColumnStuns==1 && G->Boss->Action==3,TEXT("charge hits designated column and enters recovery"));
  Check(G->Boss->GetActorLocation().Y<40,TEXT("charging capsule cannot tunnel through column"));Shot(TEXT("06_ColumnStun"));Step=104;Next=Now+5;break;
 case 104:
  Check(G->Boss->Action==0,TEXT("column recovery returns control to hearing AI"));
  G->Boss->SetActorLocation(FVector(2500,700,130));G->Boss->SetActorRotation(FRotator::ZeroRotator);P->SetActorLocation(FVector(3100,700,100));SavedHealth=P->GetHealth();G->Boss->BeginCharge(P->GetActorLocation());Step=105;Next=Now+2.1f;break;
 case 105:
  Check(G->Boss->ChargeHits==1 && FMath::IsNearlyEqual(SavedHealth-P->GetHealth(),30,1),TEXT("one charge applies one damage event"));
  P->SetActorLocation(FVector(5220,220,100));Step=106;Next=Now+5;break;
 case 106:
  G->Boss->SetActorLocation(FVector(2500,700,130));G->Boss->SetActorRotation(FRotator::ZeroRotator);P->SetActorLocation(FVector(3100,700,100));SavedHealth=P->GetHealth();G->Boss->BeginCharge(P->GetActorLocation());Step=107;Next=Now+1.5f;break;
 case 107:
  P->SetActorLocation(FVector(3100,1100,100));Step=108;Next=Now+2;break;
 case 108:
  Check(FMath::IsNearlyEqual(P->GetHealth(),SavedHealth),TEXT("lateral dodge after windup avoids locked-direction charge"));
  G->Boss->Active=false;G->Boss->Action=0;G->Boss->GetCharacterMovement()->SetMovementMode(MOVE_Walking);G->Boss->GetCharacterMovement()->StopMovementImmediately();
  G->Boss->SetActorLocation(FVector(3500,800,130));T->Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);T->Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);T->SetActorLocationAndRotation(FVector(3000,800,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
  P->SetActorLocation(T->DoorPosition());Check(T->Enter(P),TEXT("ram fixture enters physical truck"));Step=109;Next=Now+1;break;
 case 109:
  T->SetTestInput(.25f,0);SavedHealth=G->Boss->GetEnemyHealth();SavedImpacts=T->Impacts;Step=110;Next=Now+2;break;
 case 110:
  Check(IsValid(G->Boss) && G->Boss->GetEnemyHealth()==SavedHealth && T->Impacts==SavedImpacts,TEXT("low-speed bumper contact cannot execute boss"));
  T->SetTestInput(0,0);T->SetActorLocationAndRotation(FVector(2750,800,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);T->Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);T->Chassis->SetPhysicsLinearVelocity(FVector(1250,0,0));Step=111;Next=Now+1;break;
 case 111:
  Check(!IsValid(G->Boss) && T->Impacts==SavedImpacts+1,TEXT("high relative-speed leading bumper kills boss once"));
  Check(T->GetActorUpVector().Z>.85f && T->Chassis->GetPhysicsLinearVelocity().Size()<2200,TEXT("impact leaves truck upright without explosive impulse"));Shot(TEXT("07_Ram"));T->SetTestInput(0,0,true);Step=112;Next=Now+2;break;
 case 112:
  Check(T->Exit(),TEXT("exit remains usable after ram"));
  P->SetActorLocation(T->DoorPosition());Check(T->Enter(P),TEXT("driver can reenter after ram"));Step=119;Next=Now+1;break;
 case 119:
  for(FVector Local:{FVector(0,-210,0),FVector(-70,210,0),FVector(-340,0,0),FVector(340,0,0)}){FVector V=T->GetActorTransform().TransformPosition(Local);V.Z=130;ExitBlocks.Add(GetWorld()->GetAuthGameMode<ALZGameMode>()->SpawnBlock(TEXT("QAExitBlock"),V,FVector(170,170,260)));}
  Check(!T->Exit() && T->Driving,TEXT("no safe candidate keeps player in vehicle"));for(auto* A:ExitBlocks)A->Destroy();Step=113;Next=Now+1;break;
 case 113:
  Check(T->Exit(),TEXT("clearing exit candidates restores safe dismount"));P->SetActorLocation(T->DoorPosition());T->Enter(P);T->SetTestInput(-.65f,0);Step=114;Next=Now+2;break;
 case 114:
  Check(T->Speed()<-60,TEXT("reverse uses wheel forces and produces backward motion"));Check(!T->Exit(),TEXT("moving vehicle refuses unsafe dismount"));T->SetTestInput(0,0,true);Step=115;Next=Now+2;break;
 case 115:
  Check(FMath::Abs(T->Speed())<35 && T->Exit(),TEXT("brake stops vehicle and enables safe exit"));
  P->SetActorLocation(FVector(4300,900,100));G->Boss=GetWorld()->SpawnActor<ALZGarageCharger>(FVector(2400,-520,130),FRotator::ZeroRotator);G->Boss->Active=true;G->Boss->NextCharge=Now+100;G->Boss->Slice=G;
  Check(LZAcoustics::Emit(G,FVector(2900,-520,130),3200,TEXT("BottleBreak"),TEXT("Bottle")),TEXT("bottle test uses shared audible event"));Step=116;Next=Now+1;break;
 case 116:
  Check(G->Boss->HasHeardNoise() && FVector::Dist(G->Boss->GetHeardLocation(),FVector(2900,-520,130))<30,TEXT("boss hears actual bottle position"));P->SetActorLocation(FVector(4200,500,100));Step=117;Next=Now+4;break;
 case 117:
  Check(FVector::Dist(G->Boss->GetHeardLocation(),FVector(2900,-520,130))<30,TEXT("silent relocated player does not update heard snapshot"));
  Check(FVector::Dist2D(G->Boss->GetActorLocation(),FVector(2900,-520,130))<170,TEXT("hearing navigation reaches sound through garage"));Step=118;Next=Now+22;break;
 case 118:Check(!G->Boss->HasHeardNoise(),TEXT("finite search forgets old sound"));Finish();break;
 case 90:
  Check(G->Phase==ELZGaragePhase::Preparation && !G->Boss->Active && !G->DutyOpen && G->GateActivations==0 && !G->bRadioPlayed && FVector::Dist(G->BlockingChair->GetActorLocation(),G->ChairClosed)<1,TEXT("explicit retry resets encounter and shutter"));
  Check(FVector::Dist2D(T->GetActorLocation(),FVector(960,510,0))<30 && T->DoorAngle>34 && !T->Driving,TEXT("retry restores initial angled pickup and open door"));
  Check(P->HasCrowbarTool() && P->GetThrowableCount()==4 && FVector::Dist2D(P->GetActorLocation(),FVector(310,1170,0))<30,TEXT("retry supplies tools and arrival checkpoint"));Finish();break;
 }
}

void ALZGarageEscapeQA::GuidanceTick(float Delta){
 const float Now=GetWorld()->GetTimeSeconds();auto* P=G->Player;auto* T=G->Truck;auto* PC=Cast<APlayerController>(P->GetController());
 const bool Before=FParse::Param(FCommandLine::Get(),TEXT("GarageBefore"));
 auto View=[&](FVector Pos,FVector Target){P->SetActorLocation(Pos);P->GetCharacterMovement()->StopMovementImmediately();auto* Cam=P->FindComponentByClass<UCameraComponent>();auto Aim=(Target-Cam->GetComponentLocation()).Rotation();PC->SetControlRotation(Aim);Cam->SetWorldRotation(Aim);};
 auto Capture=[&](const TCHAR* Name){Shot(*(FString(Before?TEXT("Before_"):FParse::Param(FCommandLine::Get(),TEXT("GarageLightingBaseline"))?TEXT("LightingBefore_"):TEXT("After_"))+Name));};
 switch(Step){
 case 200:
  if(!Before){const bool Preset=FParse::Param(FCommandLine::Get(),TEXT("GarageBeforeGate"));Check(G->DutyOpen==Preset && !G->Boss->Active && G->GateActivations==0,TEXT("requested arrival or before-gate preset initialized"));}
  View(FVector(310,1170,100),FVector(2300,520,160));Step=201;Next=Now+3;break;
 case 201:Capture(TEXT("01_Arrival"));Step=202;Next=Now+.5f;break;
 case 202:View(Before?FVector(4260,-640,100):FVector(4510,445,100),Before?FVector(4390,-1280,155):FVector(5100,300,130));Step=203;Next=Now+3;break;
 case 203:Capture(TEXT("02_Booth"));Step=204;Next=Now+.5f;break;
 case 204:if(Before){G->DutyOpen=true;G->DutyDoor->SetActorEnableCollision(false);G->DutyDoor->SetActorHiddenInGame(true);}else G->SetBoothOpen();View(Before?FVector(4470,-1220,100):FVector(5220,270,100),FVector(4220,920,150));G->OpenExit();Step=212;Next=Now+.5f;break;
 case 212:Capture(TEXT("Opening_05"));Step=205;Next=Now+1;break;
 case 205:Capture(TEXT("03_ButtonOpening"));Step=213;Next=Now+1;break;
 case 213:Capture(TEXT("Opening_25"));Step=214;Next=Now+1;break;
 case 214:Capture(TEXT("Opening_35"));if(!Before){Check(G->Boss->Active && FVector::Dist2D(G->Boss->GetHeardLocation(),FVector(4650,850,110))<80,TEXT("boss immediately investigates shutter noise instead of player"));Check(!G->Shutter->GetActorEnableCollision(),TEXT("shutter finishes rising while boss approaches"));}Step=215;Next=Now+1.6f;break;
 case 215:View(Before?FVector(4470,-1220,100):FVector(5220,270,100),FVector(2500,-960,160));Step=216;Next=Now+.4f;break;
 case 216:Capture(TEXT("07_ButtonBossDirection"));Step=206;Next=Now+.5f;break;
 case 206:View(FVector(4240,780,100),FVector(2520,-650,150));Step=207;Next=Now+4;break;
 case 207:Capture(TEXT("05_OpenGarage"));Step=219;Next=Now+.5f;break;
 case 219:
 if(FVector::Dist2D(G->Boss->GetActorLocation(),FVector(4650,850,110))>480 && Now-G->OpenAt<28){Next=Now+.2f;break;}
 Check(FVector::Dist2D(G->Boss->GetActorLocation(),FVector(4650,850,110))<480,TEXT("boss navigates from repair bay to shutter entrance"));Capture(TEXT("08_BossAtExit"));Step=220;Next=Now+.5f;break;
 case 220:if(IsValid(G->Boss))G->Boss->Active=false;P->SetActorLocation(T->DoorPosition());T->Enter(P);T->SetActorLocationAndRotation(FVector(3780,650,105),FRotator(0,-130,0),false,nullptr,ETeleportType::TeleportPhysics);T->SetTestInput(0,0,true);Step=208;Next=Now+3;break;
 case 208:Capture(TEXT("04_DriverIslands"));Step=209;Next=Now+.6f;break;
 case 209:T->SetActorRotation(FRotator(0,5,0),ETeleportType::TeleportPhysics);Step=210;Next=Now+2;break;
 case 210:Capture(TEXT("06_DriverRamp"));Step=211;Next=Now+.5f;break;
 case 211:FFileHelper::SaveStringToFile(FString::Join(Lines,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("GarageGuidanceQA.txt")));FPlatformMisc::RequestExit(false);SetActorTickEnabled(false);break;
 }
}
