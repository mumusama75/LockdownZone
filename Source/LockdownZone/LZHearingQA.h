#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZHearingQA.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZHearingQA:public AActor
{
 GENERATED_BODY()
public:
 ALZHearingQA();virtual void BeginPlay() override;virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZEnemy* Enemy;
 UPROPERTY() class ALZEnemy* Partner;
 UPROPERTY() class ALZGarageBoss* Boss;
 UPROPERTY() class ALZCharacter* Player;
 UPROPERTY() class ALZChapter* Chapter;
 UPROPERTY() class AStaticMeshActor* Wall;
 int32 Step=0,Assertions=0,Failures=0;
 float Next=0,Started=0;
 FVector Before,Remembered;
 TArray<FString> Lines;
 void Check(bool Value,const TCHAR* Text);
 void View(FVector Position,FVector Target);
 void Capture(const TCHAR* Name);
 void Finish();
};
