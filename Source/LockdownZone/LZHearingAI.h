#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LZHearingAI.generated.h"
UENUM(BlueprintType) enum class ELZHearingState:uint8 { Idle, Alert, Investigate, Search };
UCLASS()
class LOCKDOWNZONE_API ALZHearingController:public AAIController
{
 GENERATED_BODY()
public:
 ALZHearingController();
 virtual void OnPossess(APawn* Pawn) override;
 virtual void OnUnPossess() override;
 void ReceiveSound(FVector Location,float Radius,FName Category,AActor* Source=nullptr);
 void UpdateAction(ELZHearingState Action,float Delta);
 void SetState(ELZHearingState NewState);
 UFUNCTION(BlueprintPure) ELZHearingState GetHearingState() const {return State;}
 UFUNCTION(BlueprintPure) FVector GetLastSound() const {return LastSound;}
 UFUNCTION(BlueprintPure) bool HasSoundMemory() const {return bHasSound;}
 UFUNCTION(BlueprintPure) FVector GetSearchPoint() const {return SearchPoint;}
 UFUNCTION(BlueprintPure) float GetSuspicion() const {return Suspicion;}
 FName GetLastCategory() const {return LastCategory;}
 float GetCommitUntil() const {return CommitUntil;}
 float SoundTransmission(FVector Location,AActor* Source=nullptr) const;
 int32 SearchQueries=0;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) class UAIPerceptionComponent* Hearing;
private:
 UPROPERTY() class UAISenseConfig_Hearing* HearingConfig;
 UPROPERTY() class UEnvQuery* SearchQuery;
 UPROPERTY() class USoundAttenuation* VoiceAttenuation;
 ELZHearingState State=ELZHearingState::Idle;
 FVector LastSound=FVector::ZeroVector,SearchPoint=FVector::ZeroVector;
 FVector WanderCenter=FVector::ZeroVector;
 FName LastCategory=NAME_None;
 float CommitUntil=0,CommittedPriority=0,NextWander=0;
 bool bCanWander=false;
 float StateAt=0,LastSoundAt=-100,Strength=0,Suspicion=0,NextMove=0,SearchEnds=0,PauseUntil=0,NextHowl=0;
 bool bHasSound=false,bQueryPending=false,bSearchMoving=false;
 int32 QueryId=INDEX_NONE,Generation=0;
 TArray<FVector> Visited;
 UFUNCTION() void Perceived(AActor* Source,FAIStimulus Stimulus);
 void BeginSearchQuery();
 void QueryFinished(TSharedPtr<struct FEnvQueryResult> Result,int32 RequestGeneration);
 bool MoveToSoundPoint(FVector Location);
};
UCLASS(meta=(DisplayName="LZ Hearing State"))
class LOCKDOWNZONE_API UBTDecorator_LZHearingState:public UBTDecorator
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category="Hearing") ELZHearingState State;
 virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& Owner,uint8* Memory) const override;
};
UCLASS(meta=(DisplayName="LZ Hearing Action"))
class LOCKDOWNZONE_API UBTTask_LZHearingAction:public UBTTaskNode
{
 GENERATED_BODY()
public:
 UBTTask_LZHearingAction();
 UPROPERTY(EditAnywhere,Category="Hearing") ELZHearingState State;
 virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& Owner,uint8* Memory) override;
 virtual void TickTask(UBehaviorTreeComponent& Owner,uint8* Memory,float Delta) override;
};
UCLASS()
class LOCKDOWNZONE_API UEnvQueryContext_LZLastSound:public UEnvQueryContext
{
 GENERATED_BODY()
public:
 virtual void ProvideContext(FEnvQueryInstance& Query,FEnvQueryContextData& Data) const override;
};
UCLASS()
class LOCKDOWNZONE_API ULZHearingAssets:public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="LZ|Editor") static void BuildAssets();
};
