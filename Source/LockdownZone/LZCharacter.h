#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LZInventoryTypes.h"
#include "LZCharacter.generated.h"

class UCameraComponent;
class UPointLightComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class ALZInteractable;
class APlayerController;

UENUM(BlueprintType)
enum class EPlayerWeapon : uint8
{
    None,
    Melee,
    Firearm
};

UCLASS()
class LOCKDOWNZONE_API ALZCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALZCharacter();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category="Movement")
    void StartCrouch();
    UFUNCTION(BlueprintCallable, Category="Movement")
    void StopCrouch();
    UFUNCTION(BlueprintCallable, Category="Movement")
    void ToggleCrouchState();

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool AddLoot(int32 Value, int32 Slots, int32 AmmoAmount, int32 HealAmount);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool TryStoreItem(ELZInventoryItemType Type, int32 Quantity = 1);
    UFUNCTION(BlueprintPure, Category="Inventory")
    bool CanStoreItem(ELZInventoryItemType Type, int32 Quantity = 1) const;

    const TArray<FLZInventoryEntry>& GetInventoryEntries() const { return InventoryEntries; }
    const FLZInventoryEntry* GetInventoryItemAtCell(int32 X, int32 Y) const;
    FLZInventoryEntry* GetInventoryItemAtCellMutable(int32 X, int32 Y);
    const FLZInventoryEntry* GetInventoryItemAtSlot(int32 Slot) const;
    const FLZInventoryEntry* GetInventoryItemById(int32 ItemId) const;

    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetInventoryGridWidth() const { return InventoryGridWidth; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetInventoryGridHeight() const { return InventoryGridHeight; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetCursorX() const { return CursorX; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetCursorY() const { return CursorY; }
    void SetCursorPos(int32 NewX, int32 NewY);
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetSelectedInventorySlot() const { return CursorY * InventoryGridWidth + CursorX; }
    void SetSelectedInventorySlot(int32 Slot);

    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetHeldItemId() const { return HeldItemId; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetHeldWidth() const { return HeldWidth; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetHeldHeight() const { return HeldHeight; }
    UFUNCTION(BlueprintPure, Category="Inventory") bool HasHeldItem() const { return HeldItemId != 0; }
    const FLZInventoryEntry* GetHeldItem() const { return GetInventoryItemById(HeldItemId); }

    UFUNCTION(BlueprintCallable, Category="Inventory") void PickUpItemAtCursor();
    UFUNCTION(BlueprintCallable, Category="Inventory") void PickUpItemAtCell(int32 X, int32 Y);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool PlaceHeldItemAtCursor();
    UFUNCTION(BlueprintCallable, Category="Inventory") bool PlaceHeldItemAtCell(int32 X, int32 Y);
    UFUNCTION(BlueprintCallable, Category="Inventory") void RotateHeldItem();
    UFUNCTION(BlueprintCallable, Category="Inventory") void CancelHeldItem();

    UFUNCTION(BlueprintPure, Category="Inventory")
    bool CanPlaceItem(int32 TargetX, int32 TargetY, int32 W, int32 H, int32 IgnoreItemId = 0) const;
    bool AutoFindPlacement(int32 W, int32 H, int32& OutX, int32& OutY) const;
    static void GetDefaultItemSize(ELZInventoryItemType Type, int32& OutW, int32& OutH);

    UFUNCTION(BlueprintPure, Category="Inventory") bool IsInventoryOpen() const { return bInventoryOpen; }
    UFUNCTION(BlueprintPure, Category="Inventory") FString GetInventoryStatusText() const { return InventoryStatusText; }
    void SetInventoryStatusText(const FString& InText) { InventoryStatusText = InText; }
    UFUNCTION(BlueprintCallable, Category="Inventory") void ToggleInventory();
    void InventoryLeft();
    void InventoryRight();
    void InventoryUp();
    void InventoryDown();
    void InventoryRotate();
    void InventoryInteract();
    void InventoryCancel();
    UFUNCTION(BlueprintCallable, Category="Inventory") bool UseSelectedInventoryItem();
    UFUNCTION(BlueprintCallable, Category="Inventory") bool DiscardSelectedInventoryItem();
    UFUNCTION(BlueprintCallable, Category="Inventory") bool DiscardItemAtCell(int32 X, int32 Y);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool DiscardItemById(int32 ItemId);
    void SyncEquippedGear();

    UFUNCTION(BlueprintPure, Category="Interaction")
    ALZInteractable* FindInteractable(float Range = 350.0f) const;

    UFUNCTION(BlueprintPure) float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure) int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
    UFUNCTION(BlueprintPure) int32 GetReserveAmmo() const;
    UFUNCTION(BlueprintPure) int32 GetUsedBagSlots() const;
    UFUNCTION(BlueprintPure) int32 GetMaxBagSlots() const { return InventoryGridWidth * InventoryGridHeight; }
    UFUNCTION(BlueprintPure) int32 GetLootValue() const;
    UFUNCTION(BlueprintPure) bool HasMeleeWeapon() const { return bHasMeleeWeapon; }
    UFUNCTION(BlueprintPure) bool HasFirearm() const { return bHasFirearm; }
    UFUNCTION(BlueprintPure) EPlayerWeapon GetSelectedWeapon() const { return SelectedWeapon; }
    UFUNCTION(BlueprintPure) FString GetSelectedWeaponName() const;
    void AcquireWeapon(EPlayerWeapon Weapon);
    UFUNCTION(BlueprintCallable, Category="Equipment") void AcquireFlashlight();
    UFUNCTION(BlueprintCallable, Category="Equipment") void ToggleFlashlight();
    UFUNCTION(BlueprintPure, Category="Equipment") bool HasFlashlight() const { return bHasFlashlight; }
    UFUNCTION(BlueprintPure, Category="Equipment") bool IsFlashlightOn() const { return bFlashlightOn; }
    UFUNCTION(BlueprintPure, Category="Combat") float GetRecoilPitch() const { return RecoilPitch; }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    void QAFire() { StartFire(); }
    void QAReload() { Reload(); }
    void QASelectMelee() { SelectMeleeWeapon(); }
    void QASelectFirearm() { SelectFirearm(); }
    void QAStartAim() { StartAim(); }
    void QAStopAim() { StopAim(); }
#endif

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    UCameraComponent* FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* WeaponBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* WeaponBarrel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* WeaponGrip;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* WeaponMagazine;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* FrontSight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UPointLightComponent* MuzzleLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    USpotLightComponent* Flashlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UPointLightComponent* WeaponFillLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* MeleeHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* MeleeHandle;

    UPROPERTY(EditDefaultsOnly, Category="Combat") float MaxHealth = 100.0f;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float Health = 100.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat") float WeaponDamage = 34.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat") int32 MagazineSize = 17;
    UPROPERTY(BlueprintReadOnly, Category="Combat") int32 AmmoInMagazine = 0;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") bool bHasMeleeWeapon = false;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") bool bHasFirearm = false;
    UPROPERTY(BlueprintReadOnly, Category="Weapon") EPlayerWeapon SelectedWeapon = EPlayerWeapon::None;
    UPROPERTY(BlueprintReadOnly, Category="Equipment") bool bHasFlashlight = false;
    UPROPERTY(BlueprintReadOnly, Category="Equipment") bool bFlashlightOn = false;

private:
    static constexpr int32 InventoryGridWidth = 6;
    static constexpr int32 InventoryGridHeight = 6;
    static constexpr int32 AmmoStackLimit = 30;
    UPROPERTY() TArray<FLZInventoryEntry> InventoryEntries;
    int32 NextItemId = 1;
    bool bInventoryOpen = false;
    int32 CursorX = 0;
    int32 CursorY = 0;
    int32 HeldItemId = 0;
    int32 HeldWidth = 1;
    int32 HeldHeight = 1;
    int32 HeldOriginalX = 0;
    int32 HeldOriginalY = 0;
    int32 HeldOriginalWidth = 1;
    int32 HeldOriginalHeight = 1;
    FString InventoryStatusText;
    float CrouchPressTime = 0.0f;
    bool bCrouchToggled = false;
    TWeakObjectPtr<APlayerController> InventoryInputController;
    bool bInventoryInputLockApplied = false;
    bool bAiming = false;
    bool bInspectingWeapon = false;
    float InspectElapsed = 0.0f;
    bool bReloading = false;
    float ReloadElapsed = 0.0f;
    static constexpr float ReloadDuration = 1.35f;
    static constexpr float RecoilRecoveryDuration = 0.50f;
    static constexpr float MaxRecoilPitch = 5.0f;
    float RecoilPitch = 0.0f;
    float RecoilYaw = 0.0f;
    float RecoilVisualAmount = 0.0f;
    float RecoilStartPitch = 0.0f;
    float RecoilStartYaw = 0.0f;
    float RecoilStartVisualAmount = 0.0f;
    float RecoilElapsed = 0.0f;
    uint32 RecoilShotCounter = 0;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartJump();
    void StartFire();
    void HideMuzzleFlash();
    void ResetMeleePose();
    void StartAim();
    void StopAim();
    void SelectMeleeWeapon();
    void SelectFirearm();
    void UpdateWeaponVisibility();
    void Reload();
    void FinishReload();
    void Interact();
    void RestartRun();
    bool IsRunInactive() const;
    void SetFlashlightEnabled(bool bEnabled);
    void ApplyShotRecoil();
    void UpdateRecoil(float DeltaSeconds);
    float ApplyCameraRecoilDelta(float PitchDelta, float YawDelta);
    void ApplyWeaponRecoilPose();
    void ResetRecoil();
    void CancelWeaponActions();
    bool PlanInventoryStorage(TArray<FLZInventoryEntry>& Entries, ELZInventoryItemType Type, int32 Quantity) const;
    int32 ConsumeStoredAmmo(int32 RequestedRounds);
    void SetInventoryOpen(bool bOpen);
    void MoveInventorySelection(int32 ColumnDelta, int32 RowDelta);
    void InventoryDiscard();
};
