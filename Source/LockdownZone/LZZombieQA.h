#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZZombieQA.generated.h"
UCLASS()
class ALZZombieQA:public AActor
{
 GENERATED_BODY()
public:
 ALZZombieQA();
 virtual void Tick(float Delta) override;
private:
 UPROPERTY() class ALZEnemy* E;
 UPROPERTY() class ACameraActor* Camera;
 float Next=6;int Step=0;int Failures=0;FVector Before;
 void Check(bool OK,const TCHAR* Text);
 void Shot(const TCHAR* Name);
};
