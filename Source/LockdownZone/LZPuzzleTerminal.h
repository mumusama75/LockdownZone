#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZPuzzleTerminal.generated.h"

UENUM(BlueprintType)
enum class EPuzzleNode : uint8
{
    Generator,
    Cooling,
    Purifier,
    Clue
};

UCLASS()
class LOCKDOWNZONE_API ALZPuzzleTerminal : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZPuzzleTerminal();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;

    void Configure(EPuzzleNode NewNode);
    void RefreshState();
    EPuzzleNode GetNode() const { return Node; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditAnywhere, Category="Puzzle") EPuzzleNode Node = EPuzzleNode::Generator;
    FString GetNodeName() const;
};
