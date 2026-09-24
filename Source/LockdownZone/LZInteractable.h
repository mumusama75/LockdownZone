#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZInteractable.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class ALZCharacter;

UCLASS(Abstract)
class LOCKDOWNZONE_API ALZInteractable : public AActor
{
    GENERATED_BODY()

public:
    ALZInteractable();
    UPROPERTY(EditAnywhere, Category="Interaction") bool bRequiresFlashlight = false;
    bool CanIdentifyPickup(const ALZCharacter* Character) const;

    virtual void Interact(ALZCharacter* Character);
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
    UTextRenderComponent* Label;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
    UPointLightComponent* Glow;

    void SetLabel(const FString& Text, const FColor& Color);
};
