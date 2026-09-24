#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZInteractable.h"
#include "LZEnemy.h"
#include "LZGarageSlice.generated.h"
UCLASS(Config=Game,DefaultConfig)
class ULZGarageSliceSettings:public UObject {
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere) float SteeringDegrees=28;
 UPROPERTY(Config,EditAnywhere) float EngineAcceleration=500;
 UPROPERTY(Config,EditAnywhere) float MaxSpeed=1600;
 UPROPERTY(Config,EditAnywhere) float ExitSpeed=35;
 UPROPERTY(Config,EditAnywhere) float DriverLookYawLimit=85;
 UPROPERTY(Config,EditAnywhere) float DriverLookUpLimit=30;
 UPROPERTY(Config,EditAnywhere) float DriverLookDownLimit=25;
 UPROPERTY(Config,EditAnywhere) float DriverLookSensitivity=1.5f;
 UPROPERTY(Config,EditAnywhere) float ChargeSpeed=1100;
 UPROPERTY(Config,EditAnywhere) float WindupSeconds=1.4f;
 UPROPERTY(Config,EditAnywhere) float StunSeconds=4.5f;
 UPROPERTY(Config,EditAnywhere) float RamSpeed=850;
 UPROPERTY(Config,EditAnywhere) float ShutterInitialGap=28;
 UPROPERTY(Config,EditAnywhere) float ShutterOpenSeconds=3;
 UPROPERTY(Config,EditAnywhere) float BoothPrySeconds=1.6f;
 UPROPERTY(Config,EditAnywhere) float BossRevealDelay=1.2f;
 UPROPERTY(Config,EditAnywhere) float ShutterIdleInterval=19;

};
UENUM() enum class ELZGaragePhase:uint8 { Preparation,Encounter,Escape,POI };
UCLASS()
class ALZGarageControl:public ALZInteractable {
 GENERATED_BODY()
public:
 ALZGarageControl();
 void Setup(class ALZGarageSlice* InSlice,int Kind,FVector Size);
 virtual void Interact(class ALZCharacter* P) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* P) const override;
 UPROPERTY() class ALZGarageSlice* Slice;
 int Kind=0;
};
UCLASS()
class ALZGarageCharger:public ALZEnemy {
 GENERATED_BODY()
public:
 ALZGarageCharger();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 virtual bool IsHearingEnabled() const override {return Active && Action==0;}
 virtual bool CanRunHearingMovement() const override {return Active && Action==0;}
 virtual FString GetEnemyDisplayName() const override {return TEXT("维修区巨型感染者");}
 UPROPERTY() class ALZGarageSlice* Slice;
 bool Active=false;
 int Action=0,ChargeHits=0,ColumnStuns=0;
 float Until=0,NextCharge=0;
 FVector ChargeDirection,ChargeTarget;
 void BeginCharge(FVector Target);
private:
 bool bHitThisCharge=false;
};
UCLASS()
class ALZGarageSlice:public AActor {
 GENERATED_BODY()
public:
 bool bSeamlessArrival=false;
 ALZGarageSlice();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 static bool IsMap(const UObject* Context);
 void UseControl(int Kind,class ALZCharacter* P);
 void OpenExit();
 void Retry();
 FString Hint() const;
 void Say(FString Text);
 UPROPERTY() class ALZPickupTruck* Truck;
 UPROPERTY() ALZGarageCharger* Boss;
 UPROPERTY() class ALZCharacter* Player;
 UPROPERTY() ALZGarageControl* DutyDoor;
 UPROPERTY() ALZGarageControl* Button;
 UPROPERTY() class AStaticMeshActor* Shutter;
 UPROPERTY() class AStaticMeshActor* BossGate;
 ELZGaragePhase Phase=ELZGaragePhase::Preparation;
 bool DutyOpen=false;
 int GateActivations=0;
 float OpenAt=-1;
 static bool RetryPending;
 void SetBoothOpen();
 FVector ChairClosed;
 bool bRadioPlayed=false,bBoothPrying=false;
 UPROPERTY() class AStaticMeshActor* BlockingChair;
 UPROPERTY() class APostProcessVolume* GarageLook;
private:
 float PryAt=0,NextIdleNoise=7,BossRevealAt=-1;
 bool bBossResponse=false;
 TWeakObjectPtr<ALZCharacter> PryOperator;
 void TickBooth(float Delta);

private:
 UPROPERTY() class ALZGameMode* GM;
 FString Feedback;
 float FeedbackUntil=0,FootDistance=0,NextFoot=0;
 FVector LastFoot;
 void Build();
};
