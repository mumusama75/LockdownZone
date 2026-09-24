#include "LZActionSampleQA.h"
#include "LZActionSamples.h"
#include "LZCharacter.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "LZEnemy.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
namespace {int RestartStep=0;int SavedFailures=0,SavedAssertions=0;TArray<FString> SavedLines;}
ALZActionSampleQA::ALZActionSampleQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZActionSampleQA::BeginPlay(){Super::BeginPlay();C=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));Start=GetWorld()->GetTimeSeconds();Next=Start+6;if(RestartStep){Step=RestartStep;Failures=SavedFailures;Assertions=SavedAssertions;Lines=SavedLines;}}
void ALZActionSampleQA::Check(bool Good,const TCHAR* M){++Assertions;if(!Good)++Failures;Lines.Add(FString::Printf(TEXT("ACTION_QA %s %s"),Good?TEXT("PASS"):TEXT("FAIL"),M));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());}
void ALZActionSampleQA::Aim(FVector Target){const auto R=(Target-P->FindComponentByClass<UCameraComponent>()->GetComponentLocation()).Rotation();Cast<APlayerController>(P->GetController())->SetControlRotation(R);P->FindComponentByClass<UCameraComponent>()->SetWorldRotation(R);}
void ALZActionSampleQA::Shot(const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Action_")+N+TEXT(".png"),true,false);}
void ALZActionSampleQA::Finish(){Lines.Add(FString::Printf(TEXT("ACTION_QA assertions=%d failures=%d"),Assertions,Failures));FFileHelper::SaveStringToFile(FString::Join(Lines,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("LZActionQA_Report.txt")));FPlatformMisc::RequestExit(false);SetActorTickEnabled(false);}
void ALZActionSampleQA::Tick(float Delta)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;Next=Now+.3f;
 if(!C || !P || Now-Start>100){Check(false,TEXT("sample fixtures available within timeout"));Finish();return;}
 switch(Step++)
 {
 case 0:
  Check(LZActionSamples::Enabled(this),TEXT("pickup sample enabled in current map"));
  P->SetActorLocation(FVector(-2780,-560,90));Aim(C->FindNode(ELZChapterNode::Crowbar)->GetActorLocation());Next=Now+.2f;break;
 case 1:
  Check(P->FindInteractable()==C->FindNode(ELZChapterNode::Crowbar),TEXT("actual pickup ray reaches crowbar"));
  C->FindNode(ELZChapterNode::Crowbar)->Interact(P);
  Check(P->HasCrowbarTool() && P->IsPresentingCrowbar() && !C->FindNode(ELZChapterNode::Crowbar),TEXT("single item grant hands scene tool to presentation"));Next=Now+.08f;break;
 case 2:Shot(TEXT("01_LiftFromBench"));Next=Now+.53f;break;
 case 3:
  Shot(TEXT("02_PickupDisplay"));Check(P->GetCharacterMovement()->MovementMode!=MOVE_None,TEXT("pickup leaves movement and camera available"));Next=Now+.8f;break;
 case 4:
  Check(!P->IsPresentingCrowbar() && P->GetUsedBagSlots()==0,TEXT("presentation settles into grip without bag cost"));
  P->PresentCrowbarPickup(FTransform::Identity);Check(!P->IsPresentingCrowbar(),TEXT("repeat acquisition does not restart long display"));
  Check(!C->bDoorBreached,TEXT("pickup leaves opening progression unchanged"));Shot(TEXT("03_NormalGrip"));break;
 case 5:
  RestartStep=20;SavedFailures=Failures;SavedAssertions=Assertions;SavedLines=Lines;GetWorld()->GetAuthGameMode<ALZGameMode>()->RestartRun();SetActorTickEnabled(false);break;
 case 20:
  Check(!P->HasCrowbarTool() && !P->IsPresentingCrowbar() && C->FindNode(ELZChapterNode::Crowbar),TEXT("reload restores uncollected tool and presentation state"));
  P->SetActorLocation(FVector(-2780,-560,90));Aim(C->FindNode(ELZChapterNode::Crowbar)->GetActorLocation());C->FindNode(ELZChapterNode::Crowbar)->Interact(P);Next=Now+.25f;break;
 case 21:
  Check(P->IsPresentingCrowbar(),TEXT("fresh run can play pickup again"));UGameplayStatics::ApplyDamage(P,1000,nullptr,this,nullptr);Next=Now+.2f;break;
 case 22:
  Check(!P->IsPresentingCrowbar() && GetWorld()->GetAuthGameMode<ALZGameMode>()->IsRunOver(),TEXT("death interrupts pickup safely"));
  RestartStep=30;SavedFailures=Failures;SavedAssertions=Assertions;SavedLines=Lines;GetWorld()->GetAuthGameMode<ALZGameMode>()->RestartRun();SetActorTickEnabled(false);break;
 case 30:
  Check(!P->HasCrowbarTool() && !P->IsPresentingCrowbar(),TEXT("restart after death clears tool and action"));
  P->SetActorLocation(FVector(-2780,-560,90));Aim(C->FindNode(ELZChapterNode::Crowbar)->GetActorLocation());C->FindNode(ELZChapterNode::Crowbar)->Interact(P);Next=Now+.25f;break;
 case 31:
  P->AcquireWeapon(EPlayerWeapon::Firearm);P->QASelectMelee();
  Check(!P->IsPresentingCrowbar() && P->HasCrowbarTool() && P->GetCharacterMovement()->MovementMode!=MOVE_None,TEXT("weapon switch cancels display and preserves tool and movement"));break;
 default:Finish();break;
 }
#endif
}
