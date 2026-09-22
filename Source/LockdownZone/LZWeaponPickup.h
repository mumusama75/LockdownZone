#pragma once

#include "CoreMinimal.h"
#include "LZCharacter.h"
#include "LZInteractable.h"
#include "LZWeaponPickup.generated.h"

UCLASS()
class LOCKDOWNZONE_API ALZWeaponPickup : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZWeaponPickup();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;
    void Configure(EPlayerWeapon NewWeapon);

private:
    UPROPERTY() class UStaticMeshComponent* AxeHead;
    UPROPERTY() class UBoxComponent* InteractionBounds;
    UPROPERTY(EditAnywhere, Category="Weapon") EPlayerWeapon Weapon = EPlayerWeapon::Melee;
};
