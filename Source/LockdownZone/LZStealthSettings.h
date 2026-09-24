#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LZStealthSettings.generated.h"

// Centimetres / seconds. Shared acoustic rules, not encounter-specific AI exceptions.
UCLASS(Config=Game,DefaultConfig)
class LOCKDOWNZONE_API ULZStealthSettings : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere,Category="Door") float DoorHoldSeconds=7;
 UPROPERTY(Config,EditAnywhere,Category="Door") float DoorClearDelay=1.2f;
 UPROPERTY(Config,EditAnywhere,Category="Door") float DoorTravelSeconds=1.1f;
 UPROPERTY(Config,EditAnywhere,Category="Door") float DoorSafetyDepth=210;
 UPROPERTY(Config,EditAnywhere,Category="Door") float DoorSoundRadius=120;
 UPROPERTY(Config,EditAnywhere,Category="Encounter") int32 CrowdCount=3;
 UPROPERTY(Config,EditAnywhere,Category="Radio") float RadioInterval=14;
 UPROPERTY(Config,EditAnywhere,Category="Radio") float RadioRadius=3800;
 UPROPERTY(Config,EditAnywhere,Category="Radio") float RadioVolume=.35f;
 UPROPERTY(Config,EditAnywhere,Category="Radio") float RadioSubtitleSeconds=2.8f;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float BottleRadius=3200;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float InvestigationCommitSeconds=12;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float AlertTurnSeconds=.65f;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float InvestigateTimeout=12;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float SearchSeconds=8;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float WalkRadius=550;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float CrouchRadius=80;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float SprintRadius=1200;
 UPROPERTY(Config,EditAnywhere,Category="Hearing") float WanderRadius=280;
};
