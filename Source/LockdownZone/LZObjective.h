#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZObjective.generated.h"

UCLASS()
class LOCKDOWNZONE_API ALZObjective : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZObjective();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;
};
