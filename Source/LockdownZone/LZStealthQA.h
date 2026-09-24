#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZStealthQA.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZStealthQA:public AActor
{
 GENERATED_BODY()
public:
 ALZStealthQA();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZChapter* C;
 UPROPERTY() class ALZCharacter* P;
 UPROPERTY() TArray<class ALZEnemy*> Crowd;
 UPROPERTY() class ALZEnemy* Probe;
 UPROPERTY() class ALZNoiseProjectile* Bottle;
 TArray<FVector> Waypoints,Before;
 TArray<FString> Lines;
 int32 Step=0,Assertions=0,Failures=0,CountBefore=0,CallsBefore=0;
 float Next=0,Started=0,MoveDeadline=0,ImpactAt=0,BeforeHealth=0;
 FVector Remembered;
 bool bRestarted=false;
 void Check(bool OK,const TCHAR* Message);
 void View(FVector Position,FVector Target);
 void Capture(const TCHAR* Name);
 void Walk(TArray<FVector> Points,float Timeout=10,bool Crouch=true);
 void Finish();
};
