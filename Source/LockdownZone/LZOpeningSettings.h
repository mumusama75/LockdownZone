#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LZOpeningSettings.generated.h"

// Opening-only tuning, centimetres and seconds; hearing uses the shared perception system.
UCLASS(Config=Game,DefaultConfig)
class LOCKDOWNZONE_API ULZOpeningSettings : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere) float PrySeconds=1.6f;
 UPROPERTY(Config,EditAnywhere) float CheckCooldown=2.8f;
 UPROPERTY(Config,EditAnywhere) float PopRadius=1100.f;
 UPROPERTY(Config,EditAnywhere) float CheckRadius=80.f;
 UPROPERTY(Config,EditAnywhere) float OpenDegrees=-96.f;
 UPROPERTY(Config,EditAnywhere) FVector RepairBench=FVector(-2700,-350,80);
 UPROPERTY(Config,EditAnywhere) FVector TeachingInfected=FVector(-1850,-650,90);
};
