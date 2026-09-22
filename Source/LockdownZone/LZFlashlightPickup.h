#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZFlashlightPickup.generated.h"

class UBoxComponent;
class USceneComponent;

/** Desk-sized utility flashlight. Origin is the bottom centre; lens faces local +X. */
UCLASS()
class LOCKDOWNZONE_API ALZFlashlightPickup : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZFlashlightPickup();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() USceneComponent* PickupRoot;
    UPROPERTY() UBoxComponent* InteractionBounds;
    UPROPERTY() UStaticMeshComponent* Lens;
    UPROPERTY() TArray<UStaticMeshComponent*> Details;
};
