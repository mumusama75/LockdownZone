#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZPowerInteractable.generated.h"

UENUM(BlueprintType)
enum class EPowerInteractableType : uint8
{
    Fuse,
    Breaker
};

UCLASS()
class LOCKDOWNZONE_API ALZPowerInteractable : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZPowerInteractable();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;
    void Configure(EPowerInteractableType NewType);

private:
    UPROPERTY() TArray<class UStaticMeshComponent*> Details;
    UPROPERTY(EditAnywhere, Category="Power") EPowerInteractableType Type = EPowerInteractableType::Fuse;
};
