#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZOpeningQA.generated.h"
UCLASS()
class ALZOpeningQA:public AActor
{
 GENERATED_BODY()
public:
 ALZOpeningQA();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZChapter* C;
 UPROPERTY() class ALZCharacter* P;
 UPROPERTY() class ALZEnemy* E;
 UPROPERTY() class ALZChapterNode* Door;
 FVector EnemyStart;
 float Next=6,Started=0,MoveDeadline=0;
 int Step=0,Assertions=0,Failures=0;
 TArray<FString> Lines;
 TArray<FVector> Path;
 void Check(bool OK,const TCHAR* Text);
 void Shot(const TCHAR* Name);
 void Look(FVector Target);
 void Walk(TArray<FVector> Points);
 void Finish();
};
