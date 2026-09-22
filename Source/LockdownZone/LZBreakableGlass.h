#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZBreakableGlass.generated.h"

class UStaticMeshComponent;

UCLASS()
class LOCKDOWNZONE_API ALZBreakableGlass : public AActor
{
    GENERATED_BODY()

public:
    ALZBreakableGlass();
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* GlassMesh;

    UPROPERTY(EditDefaultsOnly)
    float GlassHealth = 30.0f;

    void Shatter(const FVector& ImpactDirection);
};
