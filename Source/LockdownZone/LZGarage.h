#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZEnemy.h"
#include "LZGarage.generated.h"
UENUM() enum class ELZGarageState:uint8 { Exploring, Pursuit, Driving, CityReveal, Complete };
UCLASS()
class LOCKDOWNZONE_API ALZGarageBoss:public ALZEnemy
{
 GENERATED_BODY()
public:
 ALZGarageBoss();
 virtual FString GetEnemyDisplayName() const override {return TEXT("巨型感染者 · 装甲外壳");}
 virtual void BeginPlay() override;
 virtual bool IsHearingEnabled() const override {return bActive;}
 virtual void Tick(float Delta) override;
 virtual float TakeDamage(float Amount,const FDamageEvent& Event,AController* Source,AActor* Causer) override;
 UPROPERTY() class ALZGarage* Garage;
 bool bActive=false;
 float StaggerUntil=0;
};
UCLASS()
class LOCKDOWNZONE_API ALZGarage:public AActor
{
 GENERATED_BODY()
public:
 ALZGarage();
 virtual void Tick(float Delta) override;
 void Start(class ALZChapter* OwnerChapter,class ALZCharacter* Player);
 bool EnterVehicle(class ALZCharacter* Player);
 ELZGarageState State=ELZGarageState::Exploring;
 bool bBossHit=false;
 float Speed=0;
 FVector Origin=FVector(12000,0,-2200);
 FVector World(FVector Local) const {return Origin+Local;}
 UPROPERTY() ALZGarageBoss* Boss;
 UPROPERTY() class AStaticMeshActor* Vehicle;
 FString Hint() const;
 void SetQAInput(float Throttle,float Steering);
private:
 UPROPERTY() class ALZChapter* Chapter;
 UPROPERTY() class ALZGameMode* GM;
 UPROPERTY() class ALZCharacter* Player;
 UPROPERTY() class ACameraActor* DriveCamera;
 UPROPERTY() class ALZChapterNode* VehicleNode;
 UPROPERTY() class AStaticMeshActor* RampSurface;
 float RevealAt=0,NextEngineNoise=0;
 float QAThrottle=0,QASteer=0;
 bool bQAInput=false;
 void Build();
 void Drive(float Delta,float Throttle,float Steering);
};
