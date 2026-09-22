#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZSliceQA.generated.h"

class ALZCharacter;
class ALZEnemy;
class ALZGameMode;
class ALZInteractable;
class ALZWeaponPickup;
class ALZLoot;
class ALZPowerInteractable;
class ALZBreakableGlass;
class ALZExtraction;
class ALZFlashlightPickup;
class UStaticMeshComponent;
struct FKey;
enum class ELZInventoryItemType : uint8;

/** Opt-in development regression driver. Spawn only with -LZSliceQA; never changes gameplay defaults. */
UCLASS(NotBlueprintable, Transient)
class LOCKDOWNZONE_API ALZSliceQA : public AActor
{
    GENERATED_BODY()

public:
    ALZSliceQA();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    enum class EStep : uint8
    {
        OpeningIdle, InventoryEmptyOpen, InventoryEmptyView, InventoryEmptyRight, InventoryEmptyClose,
        UnownedFlashlightInput, VerifyUnownedFlashlight, FlashlightPickupView,
        InteractFlashlight, PickupFlashlight,
        FlashlightUnarmedIdle, FlashlightToggleOff, FlashlightToggleOn,
        PickupMelee, MeleeIdle, PickupPistol, PistolInspect,
        PickupAmmo, ReloadFinished, AimGlass, FireGlass, RecoilView, GlassResult,
        AimEnemy, DamageEnemy, ReaimEnemy, KillEnemy, Blackout, ServerView, ServerWideSetup,
        FlashlightDarkOff, FlashlightDarkEnable, FlashlightDarkOn, ServerWideCapture,
        AimFuse, CollectFuse, AimRareLoot, CollectRareLoot,
        InventoryLootView, InventoryBlockedInput, InventoryCollectSupplies, InventorySupplyResult,
        InventoryFullRejected, InventoryFullView, InventoryMedicalFullHealth, InventoryMedicalNoUse,
        InventoryMedicalUsed, InventoryMedicalDiscarded, InventoryWorldBlocked, InventoryScrapDiscarded,
        InventoryReloadPrepare, InventoryReloadFirst, InventoryReloadShot, InventoryReloadReleased,
        AimBreaker, RestorePower, PowerView, OfficeWideSetup, OfficeWideCapture, AimExit, Extract, SettlementIdle,
        RequestRestart, AwaitRestart, VerifyRestart, Finish
    };

    TWeakObjectPtr<ALZCharacter> Player;
    TWeakObjectPtr<ALZGameMode> GameMode;
    TWeakObjectPtr<ALZWeaponPickup> MeleePickup;
    TWeakObjectPtr<ALZWeaponPickup> PistolPickup;
    TWeakObjectPtr<ALZPowerInteractable> Fuse;
    TWeakObjectPtr<ALZPowerInteractable> Breaker;
    TWeakObjectPtr<ALZBreakableGlass> Glass;
    TWeakObjectPtr<ALZExtraction> Extraction;
    TWeakObjectPtr<ALZLoot> RarePickup;
    TWeakObjectPtr<ALZLoot> OtherRarePickup;
    TWeakObjectPtr<ALZLoot> ScrapPickup;
    TArray<TWeakObjectPtr<ALZLoot>> MedicalPickups;
    TWeakObjectPtr<ALZFlashlightPickup> FlashlightPickup;
    TWeakObjectPtr<UStaticMeshComponent> WeaponVisual;
    TArray<TWeakObjectPtr<ALZLoot>> AmmoPickups;
    TArray<TWeakObjectPtr<ALZEnemy>> CombatTargets;
    TArray<TWeakObjectPtr<ALZEnemy>> SnapshotEnemies;
    TArray<FVector> SnapshotPositions;
    TMap<TWeakObjectPtr<ALZEnemy>, bool> SavedEnemyTickStates;
    TArray<FString> ReportLines;
    TArray<FString> ScreenshotPaths;
    EStep Step = EStep::OpeningIdle;
    double NextStepTime = 0.0;
    double StartedAt = 0.0;
    double ElapsedBeforeRestart = 0.0;
    float SnapshotHealth = 0.0f;
    float HipShotRecoil = 0.0f;
    FTransform HipWeaponRestPose = FTransform::Identity;
    FRotator HipViewRestRotation = FRotator::ZeroRotator;
    FVector FlashlightComparisonLocation = FVector::ZeroVector;
    FRotator FlashlightComparisonRotation = FRotator::ZeroRotator;
    FVector InventoryBlockedPosition = FVector::ZeroVector;
    FRotator InventoryBlockedRotation = FRotator::ZeroRotator;
    double InventoryProbeWorldTime = 0.0;
    int32 InventorySupplyIndex = 0;
    int32 InventoryWorldLootCount = 0;
    int32 KillIndex = 0;
    int32 Assertions = 0;
    int32 Failures = 0;
    bool bRunning = false;
    bool bIsolatingEnemies = false;

    void Advance(EStep NextStep, float Delay = 0.8f);
    bool Check(bool bCondition, const FString& Description);
    void Capture(const FString& Name);
    void PlaceAndAim(const FVector& Position, const FVector& Target);
    bool Approach(ALZInteractable* Target, const FString& Description);
    bool ApproachEnemy(ALZEnemy* Target);
    bool InteractWithAimed(ALZInteractable* Target, const FString& Description);
    bool PressInputKey(const FKey& Key, const FString& Description);
    void CheckFlashlightState(bool bExpectedOn, const FString& Description);
    int32 CountInventoryItems(ELZInventoryItemType Type) const;
    bool SelectInventoryItem(ELZInventoryItemType Type);
    int32 CountWorldLoot() const;
    void SaveEnemySnapshot();
    void CheckEnemySnapshot(const FString& Description);
    void CheckOpeningPresentation();
    void IsolateEnemies();
    void RestoreEnemies();
    void CompleteRun();
};
