#include "LZInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZInteractable::ALZInteractable()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0.0, 0.0, 85.0));
    Label->SetRelativeRotation(FRotator(0.0, 90.0, 0.0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(28.0f);
    Label->SetTextRenderColor(FColor::White);
    Label->SetVisibility(false); // Chinese text is handled by the HUD font fallback.

    Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
    Glow->SetupAttachment(Mesh);
    Glow->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    Glow->SetIntensity(12.0f);
    Glow->SetAttenuationRadius(180.0f);
    Glow->SetCastShadows(false);
}

void ALZInteractable::Interact(ALZCharacter* Character)
{
}

FString ALZInteractable::GetInteractionPrompt(const ALZCharacter* Character) const
{
    return TEXT("[E] 交互");
}

void ALZInteractable::SetLabel(const FString& Text, const FColor& Color)
{
    Label->SetText(FText::FromString(Text));
    Label->SetTextRenderColor(Color);
    Glow->SetLightColor(FLinearColor(Color));
    if (UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(Color) * 0.55f);
        Mesh->SetMaterial(0, Material);
    }
}
