#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZGarageEscapeQA.generated.h"
UCLASS()
class ALZGarageEscapeQA:public AActor {
 GENERATED_BODY()
public:
 ALZGarageEscapeQA();
 virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZGarageSlice* G;
 float Next=7,Started=0,StageAt=0;
 int Step=0,Assertions=0,Failures=0,Point=0;
 bool bRampShot=false,bMechanics=false;
 float SavedHealth=0;
 int SavedImpacts=0;
 TArray<AActor*> ExitBlocks;
 FVector Parked;
 TArray<FVector> Route;
 TArray<FString> Lines,Trace;
 void Check(bool Good,const TCHAR* Text);
 void Shot(const TCHAR* Name);
 void Finish();
 bool Follow(float Delta);
 void GuidanceTick(float Delta);
 double GuidanceSeconds=0; int GuidanceFrames=0;
};
