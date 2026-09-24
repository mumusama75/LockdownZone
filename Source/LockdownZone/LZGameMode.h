#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LZGameMode.generated.h"

class ALZCharacter;
class ALZPuzzleTerminal;
class APointLight;
class UStaticMesh;
class UMaterialInterface;
enum class EPuzzleNode : uint8;
enum class EPlayerWeapon : uint8;

UCLASS()
class LOCKDOWNZONE_API ALZGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALZGameMode();
    UPROPERTY() class ALZGarageSlice* GarageSlice=nullptr;
    bool UsesHearingAI() const {return Chapter || GarageSlice;}
    class ALZChapter* GetChapter() const { return Chapter; }
    friend class ALZChapter;
    friend class ALZElevatorFinale;
    friend class ALZVentNetwork;
    friend class ALZGarage;
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable) void CompleteObjective();
    UFUNCTION(BlueprintCallable) void TryExtract(ALZCharacter* Character);
    UFUNCTION(BlueprintCallable) void HandlePlayerDeath();
    UFUNCTION(BlueprintCallable) void RestartRun();

    UFUNCTION(BlueprintPure) bool IsObjectiveComplete() const { return bObjectiveComplete; }
    UFUNCTION(BlueprintPure) bool IsPuzzleComplete() const { return bPuzzleComplete; }
    UFUNCTION(BlueprintPure) int32 GetPuzzleStep() const { return PuzzleStep; }
    UFUNCTION(BlueprintPure) bool IsRunOver() const { return bRunOver; }
    UFUNCTION(BlueprintPure) bool WasExtractionSuccessful() const { return bExtractionSuccessful; }
    UFUNCTION(BlueprintPure) FString GetObjectiveText() const;
    UFUNCTION(BlueprintPure) FString GetStatusText() const;
    UFUNCTION(BlueprintPure) FString GetElapsedTimeText() const;

    bool IsCombatUnlocked() const;
    bool IsServiceShortcutOpen() const { return bHasFuse; }
    void NotifyWeaponCollected(EPlayerWeapon Weapon);
    void NotifyEnemyKilled();
    void CollectFuse();
    void TryRestoreOfficePower();
    UFUNCTION(BlueprintPure) bool HasFuse() const { return bHasFuse; }
    UFUNCTION(BlueprintPure) bool IsOfficeBlackout() const { return bOfficeBlackout; }
    UFUNCTION(BlueprintPure) bool IsOfficePowerRestored() const { return bOfficePowerRestored; }
    UFUNCTION(BlueprintPure) int32 GetEnemiesKilled() const { return EnemiesKilled; }

    void RegisterPuzzleTerminal(ALZPuzzleTerminal* Terminal);
    void TryActivatePuzzleNode(EPuzzleNode Node);
    bool IsPuzzleNodeActivated(EPuzzleNode Node) const;
    void ShowPuzzleClue();

protected:
    UPROPERTY(BlueprintReadOnly, Category="Run") bool bObjectiveComplete = false;
    UPROPERTY(BlueprintReadOnly, Category="Run") bool bRunOver = false;
    UPROPERTY(BlueprintReadOnly, Category="Run") bool bExtractionSuccessful = false;
    UPROPERTY(BlueprintReadOnly, Category="Puzzle") bool bPuzzleComplete = false;
    UPROPERTY(BlueprintReadOnly, Category="Puzzle") int32 PuzzleStep = 0;
    UPROPERTY(BlueprintReadOnly, Category="Story") int32 EnemiesKilled = 0;
    UPROPERTY(BlueprintReadOnly, Category="Story") bool bOfficeBlackout = false;
    UPROPERTY(BlueprintReadOnly, Category="Story") bool bHasFuse = false;
    UPROPERTY(BlueprintReadOnly, Category="Story") bool bOfficePowerRestored = false;
    UPROPERTY(BlueprintReadOnly, Category="Run") FString StatusText;

private:
    UPROPERTY() class ALZChapter* Chapter=nullptr;
    float RunStartTime = 0.0f;
    bool bCombatUnlocked = false;
    bool bPuzzleAlarmRaised = false;
    TArray<float> OriginalLightIntensities;
    TArray<FLinearColor> OriginalLightColors;
    UPROPERTY() AActor* ServiceShortcutDoor = nullptr;
    UStaticMesh* CubeMesh = nullptr;
    UStaticMesh* CylinderMesh = nullptr;
    UMaterialInterface* BasicMaterial = nullptr;
    UPROPERTY() AActor* VaultBarrier = nullptr;
    UPROPERTY() TArray<ALZPuzzleTerminal*> PuzzleTerminals;
    UPROPERTY() AActor* StartRoomDoor = nullptr;
    UPROPERTY() AActor* ExitDoor = nullptr;
    UPROPERTY() AActor* FusePickupActor = nullptr;
    UPROPERTY() TArray<APointLight*> FacilityLights;
    UPROPERTY() TArray<class UMaterialInstanceDynamic*> FacilityEmissives;

    void BuildGrayboxLevel();
    void BuildOfficeLevel();
    void BuildOfficeCirculation();
    void DressOffice();
    void SpawnModularWall(const FString& Name, const FVector& Location, const FVector& Size,
        const FRotator& Rotation, const FLinearColor& Color);
    void BuildLegacyGrayboxLevel();
public: // Runtime layout factories shared by reusable scenery builders.
    class AStaticMeshActor* SpawnBlock(const FString& Name, const FVector& Location, const FVector& Size,
        const FRotator& Rotation = FRotator::ZeroRotator,
        const FLinearColor& Color = FLinearColor(0.18f, 0.22f, 0.25f));
    class AStaticMeshActor* SpawnArtMesh(const FString& Name, const FString& AssetPath,
        const FVector& Location, const FRotator& Rotation, const FVector& Scale,
        bool bCollision = true);
private:
    void SpawnPipe(const FString& Name, const FVector& Location, float Radius, float Length,
        const FRotator& Rotation, const FLinearColor& Color);
    void SpawnZoneLabel(const FString& Text, const FVector& Location, const FColor& Color);
    void SpawnEnemy(const FVector& Location, bool bRanged);
    void SpawnLoot(const FVector& Location, uint8 LootTypeValue);
    void SpawnReturnAmbush();
    void RefreshPuzzleTerminals();
    void TriggerOfficeBlackout();
};
