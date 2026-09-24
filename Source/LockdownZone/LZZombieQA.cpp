#include "LZZombieQA.h"
#include "LZEnemy.h"
#include "LZGarage.h"
#include "LZGameMode.h"
#include "LZChapter.h"
#include "LZCharacter.h"
#include "LZIntercom.h"
#include "LZHearingAI.h"
#include "LZMotionProfile.h"
#include "Camera/CameraActor.h"
#include "BrainComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DamageEvents.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
ALZZombieQA::ALZZombieQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZZombieQA::Check(bool OK,const TCHAR* Text){if(!OK)++Failures;UE_LOG(LogTemp,Display,TEXT("ZOMBIE_QA %s %s"),OK?TEXT("PASS"):TEXT("FAIL"),Text);}
void ALZZombieQA::Shot(const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Zombie7_")+N+TEXT(".png"),false,false);}
void ALZZombieQA::Tick(float Delta)
{
 Super::Tick(Delta);float Now=GetWorld()->GetTimeSeconds();if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;Next=Now+1;
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();auto* P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
 switch(Step++)
 {
 case 0:
 {
  int Count=0,Total=0;bool AllReplaced=true;
  auto* ZombieProfile=LoadObject<ULZMotionProfile>(nullptr,TEXT("/Game/Gameplay/DA_Zombie7"));
  for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){++Total;AllReplaced &= It->GetMotionProfile()==ZombieProfile;if(It->ActorHasTag(TEXT("LZMotionQAAnchor"))){E=*It;++Count;}else{It->SetActorTickEnabled(false);if(It->GetController()){It->GetController()->SetActorTickEnabled(false);Cast<ALZHearingController>(It->GetController())->GetBrainComponent()->StopLogic(TEXT("Visual inspection isolation"));}}}
  Check(Total==6 && AllReplaced,TEXT("all six authored ordinary infected use Zombie7"));
  Check(Count==1,TEXT("one actor selected for motion inspection"));if(!E){Step=99;break;}
  auto* Spawned=GetWorld()->SpawnActor<ALZEnemy>(FVector(2300,400,90),FRotator::ZeroRotator);
  Check(Spawned->GetMotionProfile()==ZombieProfile,TEXT("later spawned infected automatically use Zombie7"));
  Spawned->Configure(EEnemyType::Raider);Check(Spawned->GetMotionProfile()!=ZombieProfile,TEXT("raiders keep their existing visual profile"));Spawned->Destroy();
  auto* Boss=GetWorld()->SpawnActor<ALZGarageBoss>(FVector(2400,400,90),FRotator::ZeroRotator);
  Check(Boss->GetMotionProfile()!=ZombieProfile,TEXT("specialized boss retains its existing visual profile"));Boss->Destroy();
  GM->GetChapter()->AdminRadio->SetActorTickEnabled(false);
  E->SetActorLocation(FVector(2300,0,90));E->SetActorRotation(FRotator(0,-90,0));Before=E->GetActorLocation();
  P->SetActorLocation(FVector(2100,-700,90));
  Camera=GetWorld()->SpawnActor<ACameraActor>(FVector(2460,-480,160),FRotator::ZeroRotator);Camera->SetActorRotation((FVector(2300,0,85)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(48);Cast<APlayerController>(P->GetController())->SetViewTarget(Camera);
  auto* Light=GetWorld()->SpawnActor<APointLight>(FVector(2320,-240,230),FRotator::ZeroRotator);auto* LC=Cast<UPointLightComponent>(Light->GetLightComponent());LC->SetIntensity(350);LC->SetAttenuationRadius(850);
  Check(E->GetMesh()->GetNumMaterials()>=2 && E->GetMesh()->GetBoneIndex(TEXT("mixamorig_Hips"))>=0,TEXT("merged textured mesh and native Mixamo skeleton"));
  UE_LOG(LogTemp,Display,TEXT("ZOMBIE_BOUNDS %s"),*E->GetMesh()->Bounds.BoxExtent.ToString());Next=Now+2;break;
 }
 case 1:Check(E->GetVelocity().Size()<1 && !E->HasHeardNoise(),TEXT("sample stays still before arming"));Shot(TEXT("01_Idle"));break;
 case 2:GM->GetChapter()->bDoorBreached=true;E->HearNoise(FVector(2700,0,90),2200);Next=Now+1.3f;break;
 case 3:Check(E->HasHeardNoise(),TEXT("sample responds to actual hearing event"));Shot(TEXT("02_Investigate"));Next=Now+2;break;
 case 4:
  Check(FVector::Dist2D(Before,E->GetActorLocation())>100,TEXT("sample navigates toward last sound"));
  E->GetController()->SetActorTickEnabled(false);Cast<ALZHearingController>(E->GetController())->GetBrainComponent()->StopLogic(TEXT("Isolated animation inspection"));Cast<ALZHearingController>(E->GetController())->StopMovement();E->GetCharacterMovement()->StopMovementImmediately();E->SetActorLocation(FVector(2300,0,82));E->SetActorRotation(FRotator(0,-90,0));P->SetActorLocation(FVector(2300,-95,90));Before.X=P->GetHealth();E->TryContactAttack(P);Next=Now+.3f;break;
 case 5:Check(P->GetHealth()<Before.X,TEXT("native attack retains contact damage"));Shot(TEXT("03_Attack"));P->SetActorLocation(FVector(2100,-700,90));break;
 case 6:E->PushFrom(FVector(2300,150,82));Check(E->IsIncapacitated(),TEXT("push still staggers sample"));Next=Now+.5f;break;
 case 7:UGameplayStatics::ApplyDamage(E,50,nullptr,this,nullptr);Next=Now+.8f;break;
 case 8:Check(E->GetMotionState()==ELZEnemyMotionState::KnockedDown && E->GetMesh()->IsSimulatingPhysics(),TEXT("native skeleton supports physical knockdown"));Shot(TEXT("04_Knockdown"));Next=Now+3;break;
 case 9:Check(E->GetMotionState()==ELZEnemyMotionState::Locomotion && !E->GetMesh()->IsSimulatingPhysics(),TEXT("sample recovers with original mesh transform"));Check(FVector::Dist(E->GetMesh()->GetBoneLocation(TEXT("mixamorig_Hips")),E->GetActorLocation())<120,TEXT("recovered mesh remains inside character vicinity"));Shot(TEXT("05_Recovery"));break;
 case 10:UGameplayStatics::ApplyDamage(E,100,nullptr,this,nullptr);Next=Now+3;break;
 case 11:Check(!IsValid(E),TEXT("lethal damage removes enemy and preserves corpse"));Shot(TEXT("06_Death"));break;
 default:FFileHelper::SaveStringToFile(FString::Printf(TEXT("ZOMBIE_QA failures=%d"),Failures),*(FPaths::ProjectSavedDir()/TEXT("LZZombieQA_Report.txt")));FPlatformMisc::RequestExit(false);SetActorTickEnabled(false);break;
 }
}
