#include "LZHearingAI.h"
#include "LZAccessDoor.h"
#include "LZStealthSettings.h"
#include "LZEnemy.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_Donut.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
namespace {const FName StateKey(TEXT("HearingState")),SoundKey(TEXT("LastSoundLocation")),SearchKey(TEXT("SearchLocation")),SuspicionKey(TEXT("Suspicion"));}
ALZHearingController::ALZHearingController()
{
 Hearing=CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("HearingOnly"));
 HearingConfig=CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
 HearingConfig->HearingRange=1500;HearingConfig->SetMaxAge(18);
 HearingConfig->DetectionByAffiliation.bDetectEnemies=true;HearingConfig->DetectionByAffiliation.bDetectNeutrals=true;HearingConfig->DetectionByAffiliation.bDetectFriendlies=true;
 Hearing->ConfigureSense(*HearingConfig);Hearing->SetDominantSense(UAISense_Hearing::StaticClass());
 Hearing->OnTargetPerceptionUpdated.AddDynamic(this,&ALZHearingController::Perceived);
}
void ALZHearingController::OnPossess(APawn* P)
{
 Super::OnPossess(P);
 VoiceAttenuation=NewObject<USoundAttenuation>(this);VoiceAttenuation->Attenuation.bAttenuate=true;VoiceAttenuation->Attenuation.AttenuationShapeExtents=FVector(100);VoiceAttenuation->Attenuation.FalloffDistance=1600;
 SearchQuery=LoadObject<UEnvQuery>(nullptr,TEXT("/Game/Gameplay/AI/EQS_LZSoundSearch.EQS_LZSoundSearch"));
 auto* Tree=LoadObject<UBehaviorTree>(nullptr,TEXT("/Game/Gameplay/AI/BT_LZHearing.BT_LZHearing"));
 if(Tree)RunBehaviorTree(Tree);
 SetState(ELZHearingState::Idle);
}
void ALZHearingController::OnUnPossess()
{
 ++Generation;StopMovement();if(BrainComponent)BrainComponent->StopLogic(TEXT("Unpossessed"));Super::OnUnPossess();
}
void ALZHearingController::Perceived(AActor* Source,FAIStimulus S)
{
 if(Source==GetPawn())return;
 if(S.WasSuccessfullySensed() && S.Type==UAISense::GetSenseID<UAISense_Hearing>())ReceiveSound(S.StimulusLocation,S.Strength*1000,S.Tag,Source);
}
float ALZHearingController::SoundTransmission(FVector Location,AActor* Source) const
{
 const APawn* P=GetPawn();if(!P)return 0;
 float Gain=1;FCollisionQueryParams Q(SCENE_QUERY_STAT(LZSoundWalls),false,P);Q.AddIgnoredActor(this);if(Source)Q.AddIgnoredActor(Source);
 FVector From=P->GetActorLocation()+FVector(0,0,35);
 for(int32 I=0;I<3;++I)
 {
   FHitResult Hit;if(!GetWorld()->LineTraceSingleByChannel(Hit,From,Location,ECC_Visibility,Q))break;
   if(FVector::Dist(Hit.ImpactPoint,Location)<35)break;
   if(Cast<APawn>(Hit.GetActor())){Q.AddIgnoredActor(Hit.GetActor());continue;}
   Gain*=.5f;Q.AddIgnoredActor(Hit.GetActor());
 }
 if(FMath::Abs(Location.Z-P->GetActorLocation().Z)>350)Gain*=.3f;
 return Gain;
}
void ALZHearingController::ReceiveSound(FVector Location,float Radius,FName Category,AActor* Source)
{
 auto* E=Cast<ALZEnemy>(GetPawn());auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(GM && !GM->UsesHearingAI())return;
 if(!E || !GM || GM->IsRunOver() || !GM->IsCombatUnlocked() || E->IsDead() || !E->IsHearingEnabled() || Radius<=0)return;
 const float Distance=FVector::Dist(E->GetActorLocation(),Location);
 const float Effective=Radius*E->HearingSensitivity*SoundTransmission(Location,Source);
 if(Distance>Effective)return;
 const float Now=GetWorld()->GetTimeSeconds();
 float Score=Effective/FMath::Max(200.f,Distance);
 if(Category==TEXT("Gunshot") || Category==TEXT("Crash"))Score*=1.5f;
 if(Category==TEXT("Howl"))Score*=.35f;
 const float Priority=Category==TEXT("Gunshot") || Category==TEXT("Crash")?3.f:Category==TEXT("Bottle")?2.f:Category==TEXT("Sprint")?1.5f:1.f;
 if(bHasSound && Now<CommitUntil && Priority<CommittedPriority)return;
 const float Remembered=Strength*FMath::Exp(-(Now-LastSoundAt)/3.f);
 const float OldPriority=LastCategory==TEXT("Gunshot") || LastCategory==TEXT("Crash")?3.f:LastCategory==TEXT("Bottle")?2.f:LastCategory==TEXT("Sprint")?1.5f:1.f;
 if(bHasSound && Priority<=OldPriority && Score<Remembered*.65f)return;
 const bool NewArea=!bHasSound || FVector::Dist2D(LastSound,Location)>250;
 LastSound=Location;LastSoundAt=Now;Strength=Score;bHasSound=true;LastCategory=Category;
 if(Priority>=2){CommitUntil=Now+GetDefault<ULZStealthSettings>()->InvestigationCommitSeconds;CommittedPriority=Priority;}
 Suspicion=FMath::Clamp(Suspicion+(Score>1.5f?.6f:.25f),0.f,1.f);
 if(Blackboard){Blackboard->SetValueAsVector(SoundKey,Location);Blackboard->SetValueAsFloat(SuspicionKey,Suspicion);}
 ++Generation;bQueryPending=false;bSearchMoving=false;Visited.Empty();NextMove=0;
 if(NewArea || State==ELZHearingState::Idle || State==ELZHearingState::Search)SetState(ELZHearingState::Alert);
 // A nearby ally can hear the caller, but never receives a hidden player coordinate.
 if(Category!=TEXT("Howl") && Category!=TEXT("Bottle") && Category!=TEXT("Radio") && Radius>=1800 && Suspicion>=.6f && Now>NextHowl)
 {
   NextHowl=Now+12;
   UAISense_Hearing::ReportNoiseEvent(this,E->GetActorLocation(),.8f,E,0,TEXT("Howl"));
   if(auto* Voice=LoadObject<USoundBase>(nullptr,TEXT("/Game/Gameplay/ChapterAudio/Howl.Howl")))UGameplayStatics::PlaySoundAtLocation(this,Voice,E->GetActorLocation(),.3f,1,0,VoiceAttenuation);
 }
}
void ALZHearingController::SetState(ELZHearingState NewState)
{
 State=NewState;StateAt=GetWorld()->GetTimeSeconds();NextMove=0;StopMovement();
 if(Blackboard)Blackboard->SetValueAsInt(StateKey,int32(State));
 if(State==ELZHearingState::Search)
 {SearchEnds=StateAt+(Cast<ALZEnemy>(GetPawn())?Cast<ALZEnemy>(GetPawn())->SearchDuration:12);PauseUntil=StateAt+.3f;bSearchMoving=false;}
 if(State==ELZHearingState::Idle){if(bHasSound){bCanWander=true;WanderCenter=LastSound;NextWander=StateAt+5;}CommitUntil=0;CommittedPriority=0;LastCategory=NAME_None;bHasSound=false;Suspicion=0;Strength=0;Visited.Empty();++Generation;bQueryPending=false;if(Blackboard)Blackboard->SetValueAsFloat(SuspicionKey,0);}
}
bool ALZHearingController::MoveToSoundPoint(FVector Location)
{
 auto* Nav=UNavigationSystemV1::GetCurrent(GetWorld());FNavLocation Projected;
 if(!Nav || !Nav->ProjectPointToNavigation(Location,Projected,FVector(140,140,250)))return false;
 return MoveToLocation(Projected.Location,65,true,true,false,true,nullptr,false)!=EPathFollowingRequestResult::Failed;
}
void ALZHearingController::UpdateAction(ELZHearingState Action,float Delta)
{
 auto* E=Cast<ALZEnemy>(GetPawn());auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(GM && !GM->UsesHearingAI())return;
 if(!E || !GM || !GM->IsCombatUnlocked() || GM->IsRunOver() || !E->IsActorTickEnabled() || !E->CanRunHearingMovement() || !E->IsHearingEnabled() || E->IsIncapacitated())
 {StopMovement();if(GM && GM->IsRunOver() && bHasSound)SetState(ELZHearingState::Idle);return;}
 const float Now=GetWorld()->GetTimeSeconds();
 // Contact is the only acquisition path besides authored noise; never track a remote player actor.
 TArray<FOverlapResult> Contacts;FCollisionQueryParams CQ(SCENE_QUERY_STAT(LZBlindContact),false,E);
 GetWorld()->OverlapMultiByObjectType(Contacts,E->GetActorLocation(),FQuat::Identity,FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeSphere(115),CQ);
 for(const auto& C:Contacts)if(auto* P=Cast<ALZCharacter>(C.GetActor()))
 {if(FVector::Dist2D(P->GetActorLocation(),E->GetActorLocation())<115){E->TryContactAttack(P);return;}}
 if(Action==ELZHearingState::Idle)
 {
  if(bCanWander && Now>NextWander)
  {
   NextWander=Now+6;FNavLocation Choice;
   if(auto* Nav=UNavigationSystemV1::GetCurrent(GetWorld()))if(Nav->GetRandomReachablePointInRadius(WanderCenter,GetDefault<ULZStealthSettings>()->WanderRadius,Choice))MoveToSoundPoint(Choice.Location);
  }
  return;
 }
 if(Action==ELZHearingState::Alert)
 {
   const FRotator Aim=(LastSound-E->GetActorLocation()).Rotation();E->SetActorRotation(FMath::RInterpConstantTo(E->GetActorRotation(),FRotator(0,Aim.Yaw,0),Delta,240));
   if(Now-StateAt>GetDefault<ULZStealthSettings>()->AlertTurnSeconds)SetState(ELZHearingState::Investigate);
 }
 else if(Action==ELZHearingState::Investigate)
 {
   if(FVector::Dist2D(E->GetActorLocation(),LastSound)<100 || Now-LastSoundAt>GetDefault<ULZStealthSettings>()->InvestigateTimeout){SetState(ELZHearingState::Search);return;}
   FVector Destination=LastSound;bool Waiting=false;
   for(TActorIterator<ALZAccessDoor> It(GetWorld());It;++It)if(It->SoundApproach(E->GetActorLocation(),LastSound,Destination))
   {
    Waiting=true;
    if(FVector::Dist2D(E->GetActorLocation(),Destination)<110){StopMovement();It->Knock(E->GetActorLocation());}
    break;
   }
   if(Now>=NextMove){NextMove=Now+.6f;if(!MoveToSoundPoint(Destination) && !Waiting)SetState(ELZHearingState::Search);}
 }
 else if(Action==ELZHearingState::Search)
 {
   Suspicion=FMath::Max(0.f,Suspicion-Delta*.06f);if(Blackboard)Blackboard->SetValueAsFloat(SuspicionKey,Suspicion);
   if(Now>SearchEnds){SetState(ELZHearingState::Idle);return;}
   if(bSearchMoving && (FVector::Dist2D(E->GetActorLocation(),SearchPoint)<100 || GetMoveStatus()!=EPathFollowingStatus::Moving))
   {StopMovement();Visited.Add(SearchPoint);bSearchMoving=false;PauseUntil=Now+.9f;}
   if(!bSearchMoving && !bQueryPending && Now>PauseUntil)BeginSearchQuery();
 }
}
void ALZHearingController::BeginSearchQuery()
{
 if(!SearchQuery){SetState(ELZHearingState::Idle);return;}
 bQueryPending=true;++SearchQueries;const int32 Requested=Generation;
 FEnvQueryRequest Request(SearchQuery,GetPawn());
 QueryId=Request.Execute(EEnvQueryRunMode::AllMatching,FQueryFinishedSignature::CreateUObject(this,&ALZHearingController::QueryFinished,Requested));
 if(QueryId==INDEX_NONE){bQueryPending=false;PauseUntil=GetWorld()->GetTimeSeconds()+1;}
}
void ALZHearingController::QueryFinished(TSharedPtr<FEnvQueryResult> Result,int32 Requested)
{
 if(Requested!=Generation || !GetPawn() || State!=ELZHearingState::Search)return;
 bQueryPending=false;float Best=-BIG_NUMBER;FVector Choice=FVector::ZeroVector;bool Found=false;
 if(Result.IsValid() && Result->IsSuccessful())for(int32 I=0;I<Result->Items.Num();++I)
 {
   FVector P=Result->GetItemAsLocation(I);float Score=-FVector::Dist2D(P,GetPawn()->GetActorLocation())*.001f;
   bool Seen=false;for(const FVector& V:Visited)if(FVector::Dist2D(P,V)<180){Seen=true;break;}if(Seen)continue;
   for(TActorIterator<ALZHearingController> It(GetWorld());It;++It)
     if(*It!=this && It->State!=ELZHearingState::Idle && It->GetPawn())
       {const FVector Reserved=It->State==ELZHearingState::Search?It->SearchPoint:It->GetPawn()->GetActorLocation();if(FVector::Dist2D(P,Reserved)<220)Score-=10;}
   if(Score>Best){Best=Score;Choice=P;Found=true;}
 }
 if(Found){SearchPoint=Choice;if(Blackboard)Blackboard->SetValueAsVector(SearchKey,Choice);bSearchMoving=MoveToSoundPoint(Choice);if(!bSearchMoving)Visited.Add(Choice);}
 PauseUntil=GetWorld()->GetTimeSeconds()+1;
}
bool UBTDecorator_LZHearingState::CalculateRawConditionValue(UBehaviorTreeComponent& Owner,uint8* Memory) const
{return Owner.GetBlackboardComponent() && Owner.GetBlackboardComponent()->GetValueAsInt(StateKey)==int32(State);}
UBTTask_LZHearingAction::UBTTask_LZHearingAction(){bNotifyTick=true;}
EBTNodeResult::Type UBTTask_LZHearingAction::ExecuteTask(UBehaviorTreeComponent& Owner,uint8* Memory){return EBTNodeResult::InProgress;}
void UBTTask_LZHearingAction::TickTask(UBehaviorTreeComponent& Owner,uint8* Memory,float Delta)
{
 auto* C=Cast<ALZHearingController>(Owner.GetAIOwner());
 if(!C || C->GetHearingState()!=State){FinishLatentTask(Owner,EBTNodeResult::Succeeded);return;}C->UpdateAction(State,Delta);
}
void UEnvQueryContext_LZLastSound::ProvideContext(FEnvQueryInstance& Query,FEnvQueryContextData& Data) const
{
 auto* Pawn=Cast<APawn>(Query.Owner.Get());auto* C=Pawn?Cast<ALZHearingController>(Pawn->GetController()):nullptr;
 if(C)UEnvQueryItemType_Point::SetContextHelper(Data,C->GetLastSound());
}
void ULZHearingAssets::BuildAssets()
{
#if WITH_EDITOR
 auto Save=[](UObject* Object)
 {FAssetRegistryModule::AssetCreated(Object);Object->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;UPackage::SavePackage(Object->GetOutermost(),Object,*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);};
 auto* BB=LoadObject<UBlackboardData>(nullptr,TEXT("/Game/Gameplay/AI/BB_LZHearing.BB_LZHearing"));
 if(!BB)BB=NewObject<UBlackboardData>(CreatePackage(TEXT("/Game/Gameplay/AI/BB_LZHearing")),TEXT("BB_LZHearing"),RF_Public|RF_Standalone);
 BB->GetOutermost()->FullyLoad();BB->Keys.Reset();
 for(FName Name:{StateKey,SoundKey,SearchKey,SuspicionKey})
 {FBlackboardEntry Entry;Entry.EntryName=Name;if(Name==StateKey)Entry.KeyType=NewObject<UBlackboardKeyType_Int>(BB);else if(Name==SuspicionKey)Entry.KeyType=NewObject<UBlackboardKeyType_Float>(BB);else Entry.KeyType=NewObject<UBlackboardKeyType_Vector>(BB);BB->Keys.Add(Entry);}Save(BB);
 auto* Tree=LoadObject<UBehaviorTree>(nullptr,TEXT("/Game/Gameplay/AI/BT_LZHearing.BT_LZHearing"));
 if(!Tree)Tree=NewObject<UBehaviorTree>(CreatePackage(TEXT("/Game/Gameplay/AI/BT_LZHearing")),TEXT("BT_LZHearing"),RF_Public|RF_Standalone);Tree->GetOutermost()->FullyLoad();Tree->BlackboardAsset=BB;
 auto* Root=NewObject<UBTComposite_Selector>(Tree);Tree->RootNode=Root;
 for(auto State:{ELZHearingState::Alert,ELZHearingState::Investigate,ELZHearingState::Search,ELZHearingState::Idle})
 {FBTCompositeChild Child;auto* Task=NewObject<UBTTask_LZHearingAction>(Tree);Task->State=State;Task->NodeName=UEnum::GetValueAsString(State);Child.ChildTask=Task;auto* Gate=NewObject<UBTDecorator_LZHearingState>(Tree);Gate->State=State;Child.Decorators.Add(Gate);Root->Children.Add(Child);}Save(Tree);
 auto* Query=LoadObject<UEnvQuery>(nullptr,TEXT("/Game/Gameplay/AI/EQS_LZSoundSearch.EQS_LZSoundSearch"));
 if(!Query)Query=NewObject<UEnvQuery>(CreatePackage(TEXT("/Game/Gameplay/AI/EQS_LZSoundSearch")),TEXT("EQS_LZSoundSearch"),RF_Public|RF_Standalone);
 Query->GetOutermost()->FullyLoad();Query->GetOptionsMutable().Reset();
 auto* Option=NewObject<UEnvQueryOption>(Query);auto* Generator=NewObject<UEnvQueryGenerator_Donut>(Option);Generator->InnerRadius.DefaultValue=150;Generator->OuterRadius.DefaultValue=600;Generator->NumberOfRings.DefaultValue=3;Generator->PointsPerRing.DefaultValue=12;Generator->Center=UEnvQueryContext_LZLastSound::StaticClass();Generator->ProjectionData.TraceMode=EEnvQueryTrace::Navigation;Generator->ProjectionData.ProjectDown=250;Generator->ProjectionData.ProjectUp=100;Option->Generator=Generator;
 auto* Test=NewObject<UEnvQueryTest_Pathfinding>(Option);Test->TestMode=EEnvTestPathfinding::PathExist;Option->Tests.Add(Test);Query->GetOptionsMutable().Add(Option);Save(Query);
 UE_LOG(LogTemp,Display,TEXT("LZ_HEARING_ASSETS_READY"));
#endif
}
