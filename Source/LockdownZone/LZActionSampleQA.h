#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZActionSampleQA.generated.h"
UCLASS()
class ALZActionSampleQA:public AActor
{
 GENERATED_BODY()
public:
 ALZActionSampleQA();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZCharacter* P;
 UPROPERTY() class ALZChapter* C;
 float Next=6,Start=0;
 int Step=0,Failures=0,Assertions=0;
 TArray<FString> Lines;
 void Check(bool Good,const TCHAR* Message);
 void Aim(FVector Target);
 void Shot(const TCHAR* Name);
 void Finish();
};
