#include "LZPowerInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "LZCharacter.h"
#include "LZGameMode.h"

ALZPowerInteractable::ALZPowerInteractable()
{
    USceneComponent* PowerRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PowerRoot"));
    SetRootComponent(PowerRoot);
    Mesh->SetupAttachment(PowerRoot);
    Label->SetupAttachment(PowerRoot);
    Glow->SetupAttachment(PowerRoot);
    Glow->SetRelativeLocation(FVector(20, 0, 12));
    Glow->SetIntensity(6.0f);
    Glow->SetAttenuationRadius(130.0f);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Detail = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("PowerDetail%d"), Index));
        Detail->SetupAttachment(PowerRoot);
        Detail->SetStaticMesh(Mesh->GetStaticMesh());
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Details.Add(Detail);
    }
}

void ALZPowerInteractable::Configure(EPowerInteractableType NewType)
{
    Type = NewType;
    if (Type == EPowerInteractableType::Fuse)
    {
        // A small protective fuse case on the service bench, 22 x 12 x 8 cm.
        Mesh->SetRelativeScale3D(FVector(0.22f, 0.12f, 0.08f));
        SetLabel(TEXT("15A FUSE"), FColor(255, 205, 80));
        Details[0]->SetRelativeLocation(FVector(-9,0,0));
        Details[1]->SetRelativeLocation(FVector(9,0,0));
        Details[0]->SetRelativeScale3D(FVector(.025f,.125f,.085f));
        Details[1]->SetRelativeScale3D(FVector(.025f,.125f,.085f));
        Details[2]->SetRelativeLocation(FVector(0,0,4.2f));
        Details[2]->SetRelativeScale3D(FVector(.10f,.05f,.005f));
    }
    else
    {
        // Wall-mounted steel cabinet, front face points into the room along +X.
        Mesh->SetRelativeScale3D(FVector(0.14f, 0.65f, 0.90f));
        SetLabel(TEXT("MAIN BREAKER"), FColor(80, 180, 255));
        Details[0]->SetRelativeLocation(FVector(8,0,0));
        Details[0]->SetRelativeScale3D(FVector(.025f,.54f,.64f));
        Details[1]->SetRelativeLocation(FVector(10,20,-10));
        Details[1]->SetRelativeScale3D(FVector(.035f,.045f,.23f));
        Details[2]->SetRelativeLocation(FVector(8,0,34));
        Details[2]->SetRelativeScale3D(FVector(.025f,.40f,.045f));
        Glow->SetRelativeLocation(FVector(45,0,40));
    }
    UMaterialInterface* Steel = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel"));
    UMaterialInterface* Dark = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal"));
    UMaterialInterface* Yellow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_SafetyYellow.M_ZT_SafetyYellow"));
    Mesh->SetMaterial(0, Type == EPowerInteractableType::Fuse ? Yellow : Steel);
    Details[0]->SetMaterial(0, Dark);
    Details[1]->SetMaterial(0, Dark);
    Details[2]->SetMaterial(0, Yellow);
}

void ALZPowerInteractable::Interact(ALZCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (Type == EPowerInteractableType::Fuse)
        {
            GameMode->CollectFuse();
            Destroy();
        }
        else
        {
            GameMode->TryRestoreOfficePower();
        }
    }
}

FString ALZPowerInteractable::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (Type == EPowerInteractableType::Fuse)
    {
        return TEXT("[E] 拾取15A保险丝");
    }
    if (const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        return GameMode->HasFuse()
            ? TEXT("[E] 安装保险丝并恢复大楼供电")
            : TEXT("配电箱缺少保险丝");
    }
    return TEXT("配电箱离线");
}
