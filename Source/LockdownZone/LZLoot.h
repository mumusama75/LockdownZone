#pragma once

#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZInventoryTypes.h"
#include "LZLoot.generated.h"

UENUM(BlueprintType)
enum class ELootType : uint8
{
    Scrap,
    Ammo,
    Medical,
    Rare
};

UCLASS()
class LOCKDOWNZONE_API ALZLoot : public ALZInteractable
{
    GENERATED_BODY()

public:
    ALZLoot();
    virtual void Interact(ALZCharacter* Character) override;
    virtual FString GetInteractionPrompt(const ALZCharacter* Character) const override;

    void Configure(ELootType NewType);
    ELZInventoryItemType GetInventoryType() const;
    int32 GetPickupQuantity() const { return LootType == ELootType::Ammo ? AmmoAmount : 1; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot") ELootType LootType = ELootType::Scrap;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Loot") int32 AmmoAmount = 0;
    UPROPERTY() TArray<class UStaticMeshComponent*> LootDetails;
};
