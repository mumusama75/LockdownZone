#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZExtraction.generated.h"

UCLASS()
class LOCKDOWNZONE_API ALZExtraction : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZExtraction();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;

private:
    UPROPERTY() TArray<class UStaticMeshComponent*> ExitDetails;
};
