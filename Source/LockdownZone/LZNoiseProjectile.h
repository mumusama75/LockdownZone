#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZNoiseProjectile.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZNoiseProjectile:public AActor
{
 GENERATED_BODY()
public:
 ALZNoiseProjectile();
 void Launch(FVector Velocity);
 UPROPERTY() class UProjectileMovementComponent* Movement;
 UPROPERTY() class UStaticMeshComponent* Shape;
 UFUNCTION() void Bounce(const FHitResult& Hit,const FVector& Velocity);
 int32 Impacts=0;
 FVector FirstImpact=FVector::ZeroVector;
};
