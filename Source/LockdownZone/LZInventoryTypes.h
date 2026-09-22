#pragma once

#include "CoreMinimal.h"
#include "LZInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class ELZInventoryItemType : uint8
{
    Ammo,
    Medical,
    Scrap,
    Rare
};

/** One placed stack. Ammo quantity is rounds; all other entries contain a single item. */
USTRUCT(BlueprintType)
struct LOCKDOWNZONE_API FLZInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    ELZInventoryItemType Type = ELZInventoryItemType::Ammo;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 Quantity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 StartSlot = INDEX_NONE;

    /** Occupied cells for this entry: one for an ammo stack/medical/scrap, two for rare parts. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 SlotsPerItem = 1;
};
