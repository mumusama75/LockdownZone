#pragma once
#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZVentNetwork.generated.h"
class ALZCharacter;
class ALZGameMode;
UCLASS()
class LOCKDOWNZONE_API ALZVentAccess : public ALZInteractable
{
 GENERATED_BODY()
public:
 void Setup(class ALZVentNetwork* InNetwork, FVector Bottom, FVector Top, bool bEntry=false);
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 UPROPERTY() class ALZVentNetwork* Network;
 FVector Ground, Upper;
 bool bEntryMouth=false;
 bool bGrateOpen=false;
 UPROPERTY() TArray<AActor*> Grate;
};
UCLASS()
class LOCKDOWNZONE_API ALZRollingCabinet : public ALZInteractable
{
 GENERATED_BODY()
public:
 ALZRollingCabinet();
 virtual void Tick(float Delta) override;
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 void Setup(class ALZVentNetwork* InNetwork);
 bool IsAligned() const;
 bool IsMoving() const { return bMoving; }
 UPROPERTY() class ALZVentNetwork* Network;
private:
 bool bMoving=false;
 FVector Target;
 TWeakObjectPtr<ALZCharacter> Operator;
};
UCLASS()
class LOCKDOWNZONE_API ALZVentNetwork : public AActor
{
 GENERATED_BODY()
public:
 void Build(ALZGameMode* GM);
 static bool IsNewLayout();
 // Entry kit uses local X across the mouth and local Y along the push/ramp.
 static FVector EntryPoint(FVector P) { return FVector(P.Y-3100, P.X+650, P.Z); }
 static FVector EntrySize(FVector S) { return FVector(S.Y,S.X,S.Z); }
 static TArray<FVector> Outlets();
 static TArray<FBox> CeilingPanels(FVector Center,FVector Size);
 static bool CeilingHole(FVector Center,FVector Size);
 UPROPERTY() ALZRollingCabinet* Cabinet;
 UPROPERTY() TArray<ALZVentAccess*> Accesses;
 bool bEntered=false;
};
