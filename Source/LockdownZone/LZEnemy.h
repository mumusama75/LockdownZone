#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/PoseSnapshot.h"
#include "LZEnemy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
    Infected,
    Raider
};

UENUM(BlueprintType)
enum class ELZEnemyMotionState : uint8 { Locomotion, Staggered, KnockedDown, GettingUp, Dead };

UCLASS()
class LOCKDOWNZONE_API ALZEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ALZEnemy();
    void HearNoise(FVector Position,float Radius);
    void PushFrom(FVector Position);
    FVector GetHeardLocation() const;
    bool HasHeardNoise() const;
    virtual bool CanRunHearingMovement() const {return true;}
    virtual bool IsHearingEnabled() const {return true;}
    void TryContactAttack(class ALZCharacter* Player);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hearing") float HearingSensitivity=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hearing") float SearchDuration=12;

    const FPoseSnapshot& GetRecoveryPose() const { return RecoveryPose; }
    float GetRecoveryBlend() const { return MotionState==ELZEnemyMotionState::GettingUp?FMath::Clamp(StateElapsed/.35f,0.f,1.f):1.f; }

    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    void Configure(EEnemyType NewType);
    void ApplyMotionProfile(class ULZMotionProfile* Profile);
    class ULZMotionProfile* GetMotionProfile() const { return MotionProfile; }
    class UAnimSequence* GetActionPose(float& Time,float& Weight) const;
    UFUNCTION(BlueprintPure) ELZEnemyMotionState GetMotionState() const { return MotionState; }
    UFUNCTION(BlueprintPure) bool IsIncapacitated() const { return MotionState!=ELZEnemyMotionState::Locomotion; }
    UFUNCTION(BlueprintPure) bool IsDead() const { return MotionState==ELZEnemyMotionState::Dead; }


    UFUNCTION(BlueprintPure) float GetEnemyHealth() const { return Health; }
    UFUNCTION(BlueprintPure) float GetEnemyMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintPure) virtual FString GetEnemyDisplayName() const;

protected:
    virtual void BeginPlay() override;
    // Specialized enemies may retain their own visual profile.
    UPROPERTY(EditDefaultsOnly, Category="Appearance") bool bUseInfectedVisuals = true;

    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body;
    UPROPERTY(VisibleAnywhere) UTextRenderComponent* TypeLabel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EEnemyType EnemyType = EEnemyType::Infected;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Health = 65.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxHealth = 65.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DetectionRange = 1800.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackRange = 145.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 16.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpeed = 210.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float AttackCooldown = 1.2f;

private:
    FPoseSnapshot RecoveryPose;
    UPROPERTY() ULZMotionProfile* MotionProfile = nullptr;
    UPROPERTY() class UNavigationPath* ChasePath = nullptr;
    ELZEnemyMotionState MotionState=ELZEnemyMotionState::Locomotion;
    float StateElapsed=0;
    float AttackStarted=-100;
    float NextPathTime=0;
    float LastSeenTime=-100;
    FVector LastSeenPosition=FVector::ZeroVector;
    int32 PathIndex=1;
    void EnterMotionState(ELZEnemyMotionState State);
    void UpdateMotionState(float DeltaSeconds);
    void SpawnCorpse();
    float NextAttackTime = 0.0f;
    void Attack(class ALZCharacter* Target);
    void RefreshHealthLabel();
};
