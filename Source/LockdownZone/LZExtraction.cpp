#include "LZExtraction.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "LZCharacter.h"
#include "LZGameMode.h"

ALZExtraction::ALZExtraction()
{
    Mesh->SetWorldScale3D(FVector(1.6f, 1.6f, 0.06f));
    SetLabel(TEXT("EXTRACTION"), FColor(80, 255, 120));
    Glow->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
    Glow->SetIntensity(5.0f);
    Glow->SetAttenuationRadius(220.0f);

    UMaterialInterface* DarkMetal = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal"));
    UMaterialInterface* GreenLED = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDGreen.M_ZT_LEDGreen"));
    UMaterialInterface* HazardYellow = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_SafetyYellow.M_ZT_SafetyYellow"));

    if (DarkMetal)
    {
        Mesh->SetMaterial(0, DarkMetal);
    }

    for (int32 Index = 0; Index < 4; ++Index)
    {
        UStaticMeshComponent* Detail = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("ExitDetail%d"), Index));
        Detail->SetupAttachment(Mesh);
        Detail->SetStaticMesh(Mesh->GetStaticMesh());
        Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ExitDetails.Add(Detail);
    }
    // North & South hazard border lines
    ExitDetails[0]->SetRelativeLocation(FVector(0.0f, 46.0f, 0.6f));
    ExitDetails[0]->SetRelativeScale3D(FVector(0.96f, 0.08f, 0.25f));
    ExitDetails[1]->SetRelativeLocation(FVector(0.0f, -46.0f, 0.6f));
    ExitDetails[1]->SetRelativeScale3D(FVector(0.96f, 0.08f, 0.25f));
    // Center evacuation direction marker
    ExitDetails[2]->SetRelativeLocation(FVector(0.0f, 0.0f, 0.6f));
    ExitDetails[2]->SetRelativeScale3D(FVector(0.40f, 0.40f, 0.25f));
    // Entrance threshold indicator
    ExitDetails[3]->SetRelativeLocation(FVector(-46.0f, 0.0f, 0.6f));
    ExitDetails[3]->SetRelativeScale3D(FVector(0.08f, 0.92f, 0.25f));

    if (HazardYellow)
    {
        ExitDetails[0]->SetMaterial(0, HazardYellow);
        ExitDetails[1]->SetMaterial(0, HazardYellow);
    }
    if (GreenLED)
    {
        ExitDetails[2]->SetMaterial(0, GreenLED);
        ExitDetails[3]->SetMaterial(0, GreenLED);
    }
}

void ALZExtraction::Interact(ALZCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        GameMode->TryExtract(Character);
    }
}

FString ALZExtraction::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (!GameMode->IsOfficePowerRestored())
        {
            return TEXT("安全门断电：需要先恢复大楼供电");
        }
    }
    return TEXT("[E] 离开大厦");
}
