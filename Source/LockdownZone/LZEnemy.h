#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LZEnemy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
    Infected,
    Raider
};

UCLASS()
class LOCKDOWNZONE_API ALZEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ALZEnemy();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    void Configure(EEnemyType NewType);

    UFUNCTION(BlueprintPure) float GetEnemyHealth() const { return Health; }
    UFUNCTION(BlueprintPure) float GetEnemyMaxHealth() const { return MaxHealth; }
    UFUNCTION(BlueprintPure) FString GetEnemyDisplayName() const;

protected:
    virtual void BeginPlay() override;

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
    float NextAttackTime = 0.0f;
    void Attack(class ALZCharacter* Target);
    void RefreshHealthLabel();
};
