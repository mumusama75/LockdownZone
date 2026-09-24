#include "LZWeaponPickup.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZWeaponPickup::ALZWeaponPickup()
{
    USceneComponent* PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot"));
    SetRootComponent(PickupRoot);
    Mesh->SetupAttachment(PickupRoot);
    Label->SetupAttachment(PickupRoot);
    Label->SetRelativeLocation(FVector(0, 0, 25));
    Label->SetWorldSize(12.0f);
    Label->SetVisibility(false); // Chinese interaction text is rendered by the HUD.
    Glow->SetupAttachment(PickupRoot);
    Glow->SetRelativeLocation(FVector(0, 0, 18));
    Glow->SetIntensity(6.0f);
    Glow->SetAttenuationRadius(100.0f);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("PickupInteraction"));
    InteractionBounds->SetupAttachment(PickupRoot);
    InteractionBounds->SetBoxExtent(FVector(25, 20, 14));
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    AxeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupAxeHead"));
    AxeHead->SetupAttachment(PickupRoot);
    AxeHead->SetStaticMesh(Mesh->GetStaticMesh());
    AxeHead->SetRelativeLocation(FVector(25, 0, 0));
    AxeHead->SetRelativeScale3D(FVector(0.18f, 0.045f, 0.14f));
    AxeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALZWeaponPickup::Configure(EPlayerWeapon NewWeapon)
{
    Weapon = NewWeapon;
    if (Weapon == EPlayerWeapon::Firearm)
    {
        SetLabel(TEXT("格洛克17 · 9毫米\n弹匣剩余3发"), FColor(255, 190, 70));
        AxeHead->SetVisibility(false);
        if (UStaticMesh* PistolMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol")))
        {
            Mesh->SetStaticMesh(PistolMesh);
            Mesh->EmptyOverrideMaterials(); // Preserve the official PBR material.
            const float Scale = 20.4f / PistolMesh->GetBounds().BoxExtent.GetMax() / 2.0f;
            Mesh->SetRelativeScale3D(FVector(Scale));
            Mesh->SetRelativeRotation(FRotator(90.0f, -90.0f, 0.0f));
            Mesh->SetRelativeLocation(-Mesh->GetRelativeRotation().RotateVector(PistolMesh->GetBounds().Origin * Scale));
        }
    }
    else
    {
        Mesh->SetRelativeScale3D(FVector(0.72f, 0.035f, 0.035f));
        AxeHead->SetVisibility(true);
        InteractionBounds->SetBoxExtent(FVector(40, 14, 14));
        SetLabel(TEXT("FIRE AXE"), FColor(220, 70, 50));
        if (UMaterialInterface* Steel = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel")))
        {
            UMaterialInstanceDynamic* HeadMat = UMaterialInstanceDynamic::Create(Steel, this);
            HeadMat->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.82f, 0.08f, 0.06f));
            HeadMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.08f, 0.06f));
            AxeHead->SetMaterial(0, HeadMat);
        }
        if (UMaterialInterface* Dark = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal")))
        {
            Mesh->SetMaterial(0, Dark);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("LZ_PICKUP %s mesh_cm=%s actor_cm=%s"), *GetName(),
        *(Mesh->Bounds.BoxExtent * 2).ToCompactString(), *GetComponentsBoundingBox(true).GetSize().ToCompactString());
}

void ALZWeaponPickup::Interact(ALZCharacter* Character)
{
    if (!CanIdentifyPickup(Character)) return;
    if (Character && Character->AcquireWeapon(Weapon))
    {
        Destroy();
    }
}

FString ALZWeaponPickup::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (!CanIdentifyPickup(Character)) return FString();
    return Weapon == EPlayerWeapon::Firearm
        ? TEXT("[E] 拾取格洛克17（弹匣内3发）并检视")
        : TEXT("[E] 拾取消防斧并检视");
}
