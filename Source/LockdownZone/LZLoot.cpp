#include "LZLoot.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "LZCharacter.h"

ALZLoot::ALZLoot()
{
    Mesh->SetWorldScale3D(FVector(0.42f, 0.42f, 0.28f));
    SetLabel(TEXT("SUPPLY"), FColor(220, 200, 80));
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Detail = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("LootDetail%d"), Index));
        Detail->SetupAttachment(Mesh);
        Detail->SetStaticMesh(Mesh->GetStaticMesh());
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LootDetails.Add(Detail);
    }
}

void ALZLoot::Configure(ELootType NewType)
{
    LootType = NewType;
    AmmoAmount = 0;

    UMaterialInterface* DarkMetal = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal"));
    UMaterialInterface* Steel = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel"));
    UMaterialInterface* SafetyYellow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_SafetyYellow.M_ZT_SafetyYellow"));
    UMaterialInterface* AmberLED = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDAmber.M_ZT_LEDAmber"));
    UMaterialInterface* GreenLED = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDGreen.M_ZT_LEDGreen"));

    switch (LootType)
    {
    case ELootType::Ammo:
        AmmoAmount = 12;
        Mesh->SetWorldScale3D(FVector(0.28f, 0.20f, 0.14f));
        SetLabel(TEXT("9毫米弹药 ×12"), FColor(255, 190, 60));
        Glow->SetIntensity(4.0f);
        Glow->SetAttenuationRadius(90.0f);
        if (DarkMetal) Mesh->SetMaterial(0, DarkMetal);
        LootDetails[0]->SetVisibility(true);
        LootDetails[0]->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
        LootDetails[0]->SetRelativeScale3D(FVector(0.12f, 0.40f, 0.10f));
        LootDetails[1]->SetVisibility(true);
        LootDetails[1]->SetRelativeLocation(FVector(48.0f, 0.0f, 0.0f));
        LootDetails[1]->SetRelativeScale3D(FVector(0.08f, 0.60f, 0.25f));
        LootDetails[2]->SetVisibility(false);
        if (Steel) LootDetails[0]->SetMaterial(0, Steel);
        if (SafetyYellow) LootDetails[1]->SetMaterial(0, SafetyYellow);
        break;

    case ELootType::Medical:
        Mesh->SetWorldScale3D(FVector(0.34f, 0.26f, 0.16f));
        SetLabel(TEXT("MEDKIT"), FColor(80, 220, 130));
        Glow->SetIntensity(5.0f);
        Glow->SetAttenuationRadius(110.0f);
        if (Steel) Mesh->SetMaterial(0, Steel);
        LootDetails[0]->SetVisibility(true);
        LootDetails[0]->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
        LootDetails[0]->SetRelativeScale3D(FVector(0.45f, 0.14f, 0.08f));
        LootDetails[1]->SetVisibility(true);
        LootDetails[1]->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
        LootDetails[1]->SetRelativeScale3D(FVector(0.14f, 0.45f, 0.08f));
        LootDetails[2]->SetVisibility(true);
        LootDetails[2]->SetRelativeLocation(FVector(48.0f, 0.0f, 0.0f));
        LootDetails[2]->SetRelativeScale3D(FVector(0.06f, 0.35f, 0.18f));
        if (GreenLED)
        {
            LootDetails[0]->SetMaterial(0, GreenLED);
            LootDetails[1]->SetMaterial(0, GreenLED);
        }
        if (DarkMetal) LootDetails[2]->SetMaterial(0, DarkMetal);
        break;

    case ELootType::Rare:
        Mesh->SetWorldScale3D(FVector(0.50f, 0.26f, 0.16f));
        SetLabel(TEXT("RARE PART  $500"), FColor(130, 100, 255));
        Glow->SetIntensity(5.0f);
        Glow->SetAttenuationRadius(120.0f);
        if (DarkMetal) Mesh->SetMaterial(0, DarkMetal);
        LootDetails[0]->SetVisibility(true);
        LootDetails[0]->SetRelativeLocation(FVector(48.0f, 0.0f, 0.0f));
        LootDetails[0]->SetRelativeScale3D(FVector(0.06f, 0.85f, 0.70f));
        LootDetails[1]->SetVisibility(true);
        LootDetails[1]->SetRelativeLocation(FVector(54.0f, 0.0f, 0.0f));
        LootDetails[1]->SetRelativeScale3D(FVector(0.06f, 0.35f, 0.15f));
        LootDetails[2]->SetVisibility(true);
        LootDetails[2]->SetRelativeLocation(FVector(49.0f, 28.0f, 20.0f));
        LootDetails[2]->SetRelativeScale3D(FVector(0.06f, 0.12f, 0.12f));
        if (Steel) LootDetails[0]->SetMaterial(0, Steel);
        if (DarkMetal) LootDetails[1]->SetMaterial(0, DarkMetal);
        if (AmberLED) LootDetails[2]->SetMaterial(0, AmberLED);
        break;

    default: // Scrap
        Mesh->SetWorldScale3D(FVector(0.26f, 0.24f, 0.12f));
        SetLabel(TEXT("SCRAP  $120"), FColor(220, 200, 80));
        Glow->SetIntensity(4.0f);
        Glow->SetAttenuationRadius(90.0f);
        if (DarkMetal) Mesh->SetMaterial(0, DarkMetal);
        LootDetails[0]->SetVisibility(true);
        LootDetails[0]->SetRelativeLocation(FVector(0.0f, 0.0f, 48.0f));
        LootDetails[0]->SetRelativeScale3D(FVector(0.65f, 0.55f, 0.08f));
        LootDetails[1]->SetVisibility(true);
        LootDetails[1]->SetRelativeLocation(FVector(18.0f, 12.0f, 52.0f));
        LootDetails[1]->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.08f));
        LootDetails[2]->SetVisibility(false);
        if (SafetyYellow) LootDetails[0]->SetMaterial(0, SafetyYellow);
        if (Steel) LootDetails[1]->SetMaterial(0, Steel);
        break;
    }
}

void ALZLoot::Interact(ALZCharacter* Character)
{
    if (!CanIdentifyPickup(Character)) return;
    if (Character && !Character->IsInventoryOpen() &&
        Character->TryStoreItem(GetInventoryType(), GetPickupQuantity()))
    {
        Destroy();
    }
}

FString ALZLoot::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (!CanIdentifyPickup(Character)) return FString();
    if (Character && !Character->CanStoreItem(GetInventoryType(), GetPickupQuantity()))
    {
        if (LootType == ELootType::Rare)
        {
            return TEXT("背包空间不足：服务器备件需连续2×2格！[B] 整理或按 [Delete] 丢弃");
        }
        return TEXT("背包空间不足：[B] 整理背包或按 [Delete] 丢弃物品");
    }
    switch (LootType)
    {
    case ELootType::Ammo: return TEXT("[E] 拾取9毫米弹药（12发 / 每格最多30发）");
    case ELootType::Medical: return TEXT("[E] 拾取医疗包（占1×2格 / 背包内使用恢复35生命）");
    case ELootType::Rare: return TEXT("[E] 搜集服务器备件（价值500 / 占2×2格）");
    default: return TEXT("[E] 搜集电子零件（价值120 / 占1格）");
    }
}

ELZInventoryItemType ALZLoot::GetInventoryType() const
{
    switch (LootType)
    {
    case ELootType::Ammo: return ELZInventoryItemType::Ammo;
    case ELootType::Medical: return ELZInventoryItemType::Medical;
    case ELootType::Rare: return ELZInventoryItemType::Rare;
    default: return ELZInventoryItemType::Scrap;
    }
}
