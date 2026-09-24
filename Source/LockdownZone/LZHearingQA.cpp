#include "LZHearingQA.h"
#include "LZIntercom.h"
#include "LZHearingAI.h"
#include "LZChapter.h"
#include "LZGarage.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
ALZHearingQA::ALZHearingQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZHearingQA::BeginPlay()
{Super::BeginPlay();Chapter=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();Player=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));Started=GetWorld()->GetTimeSeconds();Next=Started+4;}
void ALZHearingQA::Check(bool Value,const TCHAR* Text)
{++Assertions;if(!Value)++Failures;FString Line=FString::Printf(TEXT("HEARING_QA %s %s"),Value?TEXT("PASS"):TEXT("FAIL"),Text);Lines.Add(Line);UE_LOG(LogTemp,Display,TEXT("%s"),*Line);}
void ALZHearingQA::View(FVector Position,FVector Target)
{Player->GetCharacterMovement()->StopMovementImmediately();Player->SetActorLocation(Position);Cast<APlayerController>(Player->GetController())->SetControlRotation((Target-Position-FVector(0,0,64)).Rotation());}
void ALZHearingQA::Capture(const TCHAR* Name)
{FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Hearing_")+Name+TEXT(".png"),true,false);}
void ALZHearingQA::Tick(float Delta)
{
 Super::Tick(Delta);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
 const float Now=GetWorld()->GetTimeSeconds();if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;
 if(Now-Started>90 || !Chapter || !Player){Check(false,TEXT("hearing QA runtime exists within timeout"));Finish();return;}
 Next=Now+.8f;
 auto* Brain=Enemy?Cast<ALZHearingController>(Enemy->GetController()):nullptr;
 switch(Step++)
 {
 case 0:
   Chapter->AdminRadio->NextCallAt=BIG_NUMBER;
   for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)It->Destroy();
   Chapter->bDoorBreached=true;View(FVector(-800,300,90),FVector(0,0,90));
   Enemy=GetWorld()->SpawnActor<ALZEnemy>(FVector(0,0,90),FRotator::ZeroRotator);Before=Enemy->GetActorLocation();Next=Now+1;break;
 case 1:
   Check(Brain && Brain->GetBrainComponent() && Brain->GetBrainComponent()->IsRunning(),TEXT("saved behavior tree is running on hearing controller"));
   Check(Brain && Brain->GetBlackboardComponent(),TEXT("saved blackboard is attached"));
   Check(Brain && !Brain->Hearing->GetSenseConfig(UAISense::GetSenseID<UAISense_Sight>()),TEXT("no sight sense configured"));
   Check(!Enemy->HasHeardNoise() && FVector::Dist2D(Before,Enemy->GetActorLocation())<2,TEXT("visible silent player does not wake infected"));
   Chapter->Noise(FVector(650,0,90),4500,TEXT("Clang"),TEXT("Gunshot"));Next=Now+1.3f;break;
 case 2:
   Check(Enemy->HasHeardNoise() && FVector::Dist(Enemy->GetHeardLocation(),FVector(650,0,90))<2,TEXT("ReportNoiseEvent reaches hearing perception with exact event location"));
   Check(Enemy->GetActorLocation().X>Before.X+25,TEXT("behavior tree moves toward heard event"));
   Remembered=Enemy->GetHeardLocation();View(FVector(-1000,-500,90),Enemy->GetActorLocation());
   Chapter->Noise(Enemy->GetActorLocation()+FVector(0,400,0),800,TEXT("Clang"));Next=Now+.6f;break;
 case 3:
   Check(FVector::Dist(Enemy->GetHeardLocation(),Remembered)<2,TEXT("nearby weak distraction cannot overwrite fresh gunshot memory"));
   Check(Enemy->GetHeardLocation().X>Player->GetActorLocation().X+1000,TEXT("silent player relocation does not update target position"));
   DrawDebugSphere(GetWorld(),Remembered,70,16,FColor::Cyan,false,3);Capture(TEXT("01_LastKnownSound"));
   Brain->SetState(ELZHearingState::Idle);Enemy->SetActorLocation(FVector(0,0,90));
   Wall=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-350,0,150),FRotator::ZeroRotator);Wall->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Wall->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Wall->SetActorScale3D(FVector(.2f,6,3));Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
   Check(Brain->SoundTransmission(FVector(-650,0,90))<=.5f,TEXT("closed partition attenuates sound"));
   Chapter->Noise(FVector(-650,0,90),900);Next=Now+.6f;break;
 case 4:
   Check(!Enemy->HasHeardNoise(),TEXT("quiet sound behind closed partition is below hearing threshold"));Wall->Destroy();Chapter->Noise(FVector(-650,0,90),900);Next=Now+.8f;break;
 case 5:
   Check(Enemy->HasHeardNoise(),TEXT("same sound becomes audible when partition opens"));
   Partner=GetWorld()->SpawnActor<ALZEnemy>(FVector(-100,400,90),FRotator::ZeroRotator);
   Chapter->Noise(FVector(-650,0,90),4500);Next=Now+.5f;break;
 case 6:
   Enemy->SearchDuration=5;Partner->SearchDuration=5;Brain->SetState(ELZHearingState::Search);Cast<ALZHearingController>(Partner->GetController())->SetState(ELZHearingState::Search);Next=Now+2;break;
 case 7:
   {auto* Other=Cast<ALZHearingController>(Partner->GetController());
   Check(Brain->SearchQueries>0 && Other->SearchQueries>0,TEXT("both infected execute EQS sound-search queries"));
   Check(!Brain->GetSearchPoint().IsNearlyZero() && !Other->GetSearchPoint().IsNearlyZero(),TEXT("EQS finds reachable investigation points"));
   Check(FVector::Dist2D(Brain->GetSearchPoint(),Other->GetSearchPoint())>150,TEXT("group search reserves separated points instead of stacking"));
   DrawDebugSphere(GetWorld(),Brain->GetSearchPoint(),65,16,FColor::Green,false,3);DrawDebugSphere(GetWorld(),Other->GetSearchPoint(),65,16,FColor::Yellow,false,3);View(FVector(600,-450,160),FVector(-650,0,90));Capture(TEXT("02_DistributedSearch"));Next=Now+4;}
   break;
 case 8:
   Check(Brain->GetHearingState()==ELZHearingState::Idle && !Enemy->HasHeardNoise(),TEXT("unsuccessful search expires and releases target memory"));
   Enemy->Destroy();Partner->Destroy();View(FVector(-1000,0,90),FVector(1200,0,90));
   Boss=GetWorld()->SpawnActor<ALZGarageBoss>(FVector(1200,0,90),FRotator(0,180,0));Boss->bActive=true;Before=Boss->GetActorLocation();Next=Now+1;break;
 case 9:
   Check(FVector::Dist2D(Before,Boss->GetActorLocation())<2 && !Boss->HasHeardNoise(),TEXT("active boss cannot track a silent visible player"));
   // Boss initially faces 180 degrees away: include the hearing turn, navigation
   // request and acceleration in this bounded response window.
   Chapter->Noise(FVector(1800,0,90),3000);Next=Now+2.5f;break;
 case 10:
   Check(Boss->GetActorLocation().X>Before.X+30,TEXT("boss follows sound away from the player instead of live coordinates"));
   Check(FVector::Dist(Boss->GetHeardLocation(),FVector(1800,0,90))<2,TEXT("boss retains sound-event position only"));Capture(TEXT("03_BlindBoss"));break;
 case 11:
   Cast<ALZHearingController>(Boss->GetController())->SetState(ELZHearingState::Idle);
   Partner=GetWorld()->SpawnActor<ALZEnemy>(Boss->GetActorLocation()-FVector(350,0,0),FRotator::ZeroRotator);Next=Now+.4f;break;
 case 12:
   Remembered=Boss->GetActorLocation();UAISense_Hearing::ReportNoiseEvent(this,Remembered,.8f,Boss,0,TEXT("Howl"));Next=Now+.6f;break;
 case 13:
   Check(!Boss->HasHeardNoise(),TEXT("caller ignores its own alert vocalization"));
   Check(Partner->HasHeardNoise() && FVector::Dist(Partner->GetHeardLocation(),Remembered)<2,TEXT("ally hears caller location without receiving player coordinates"));
   Next=Now+1;break;
 case 14:
   Check(!Boss->HasHeardNoise(),TEXT("ally alert does not recursively relay back through the group"));break;
 default:Finish();break;
 }
#endif
}
void ALZHearingQA::Finish()
{
 const FString Summary=FString::Printf(TEXT("HEARING_QA SUMMARY %s assertions=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Assertions,Failures);Lines.Add(Summary);UE_LOG(LogTemp,Display,TEXT("%s"),*Summary);FFileHelper::SaveStringArrayToFile(Lines,*(FPaths::ProjectSavedDir()/TEXT("LZHearingQA_Report.txt")));SetActorTickEnabled(false);
 if(FParse::Param(FCommandLine::Get(),TEXT("LZQAExit")))FPlatformMisc::RequestExit(false);
}
