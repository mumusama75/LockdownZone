#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZChapterQA.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZChapterQA:public AActor
{
 GENERATED_BODY()
public:
 ALZChapterQA();virtual void Tick(float Delta) override;virtual void BeginPlay() override;
private:
 UPROPERTY() class ALZCharacter* Player;
 UPROPERTY() class ALZChapter* Chapter;
 UPROPERTY() class ALZEnemy* Probe;
 int32 Step=0,Assertions=0,Failures=0,QTEIndex=0;
 float Next=0,Started=0,BeforeHealth=0;
 FVector Before;
 TArray<FString> Lines;
 void Check(bool Condition,const TCHAR* Text);
 void View(FVector Position,FVector Target);
 void Capture(const TCHAR* Name);
 void Use(uint8 Kind);
 void Finish();
};
