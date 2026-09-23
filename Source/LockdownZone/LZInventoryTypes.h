#pragma once

#include "CoreMinimal.h"
#include "LZInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class ELZInventoryItemType : uint8
{
    Ammo,
    Medical,
    Scrap,
    Rare,
    Axe,
    Pistol,
    Flashlight
};

/** One placed inventory entry in the 6x6 spatial grid. */
USTRUCT(BlueprintType)
struct LOCKDOWNZONE_API FLZInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    ELZInventoryItemType Type = ELZInventoryItemType::Ammo;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 Quantity = 1;

    /** Unique identifier for drag-and-drop and selection tracking. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 ItemId = 0;

    /** Top-left grid X coordinate (0..5). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 PosX = 0;

    /** Top-left grid Y coordinate (0..5). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 PosY = 0;

    /** Footprint width in grid cells (1..6). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 Width = 1;

    /** Footprint height in grid cells (1..6). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 Height = 1;

    /** Backwards-compatible linear start index (PosY * 6 + PosX). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 StartSlot = 0;

    /** Backwards-compatible total occupied cell count (Width * Height). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
    int32 SlotsPerItem = 1;

    /** Helper to check if a specific 2D cell is covered by this entry. */
    bool CoversCell(int32 X, int32 Y) const
    {
        return X >= PosX && X < (PosX + Width) && Y >= PosY && Y < (PosY + Height);
    }

    /** Sync legacy fields. */
    void SyncLegacyFields()
    {
        StartSlot = PosY * 6 + PosX;
        SlotsPerItem = Width * Height;
    }
};
