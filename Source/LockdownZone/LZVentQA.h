#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZVentQA.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZVentQA:public AActor
{
 GENERATED_BODY()
public:
 ALZVentQA();virtual void Tick(float Delta) override;
private:
 int32 Step=0,Assertions=0,Failures=0;float Next=4;
 TArray<FString> Lines;
 void Check(bool Value,const TCHAR* Message);
 void View(FVector P,FVector Target);
 void Capture(const TCHAR* Name);
};
