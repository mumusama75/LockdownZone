#include "LZFlashlightPickup.h"
#include "Components/SpotLightComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "LZCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZFlashlightPickup::ALZFlashlightPickup()
{
    // Geometry spans X[-11,11], Y[-3.2,3.2], Z[0,6.4] centimetres.
    // The neutral scene root prevents cylinder scaling from reaching the label,
    // highlight light, lens or raycast target (the earlier pickup-scale failure).
    PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlashlightRoot"));
    SetRootComponent(PickupRoot);
    Mesh->SetupAttachment(PickupRoot);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterial(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GripMaterial(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_Recess.M_ZT_Recess"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HeadMaterial(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_Trim.M_ZT_Trim"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SafetyMaterial(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_SafetyYellow.M_ZT_SafetyYellow"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> LensMaterial(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDGreen.M_ZT_LEDGreen"));

    Mesh->SetStaticMesh(Cylinder.Object);
    Mesh->SetRelativeRotation(FRotator(90, 0, 0));
    Mesh->SetRelativeLocation(FVector(-1.625f, 0, 3.2f));
    Mesh->SetRelativeScale3D(FVector(0.043f, 0.043f, 0.1475f));
    if (BodyMaterial.Succeeded())
    {
        Mesh->SetMaterial(0, BodyMaterial.Object);
    }

    auto AddCylinder = [this](const TCHAR* Name, float X, float Diameter,
        float Length, UMaterialInterface* Material)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(PickupRoot);
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Cylinder.Object);
        Part->SetRelativeLocation(FVector(X, 0, 3.2f));
        Part->SetRelativeRotation(FRotator(90, 0, 0));
        Part->SetRelativeScale3D(FVector(Diameter / 100.0f, Diameter / 100.0f, Length / 100.0f));
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        if (Material)
        {
            Part->SetMaterial(0, Material);
        }
        Details.Add(Part);
        return Part;
    };
    AddCylinder(TEXT("RubberTailCap"), -10.0f, 5.2f, 2.0f, GripMaterial.Object);
    for (int32 Index = 0; Index < 5; ++Index)
    {
        const FString Name = FString::Printf(TEXT("GripRing%d"), Index);
        AddCylinder(*Name, -7.6f + Index * 2.1f, 4.6f, 0.38f, GripMaterial.Object);
    }
    AddCylinder(TEXT("SafetyBand"), 4.8f, 4.6f, 0.4f, SafetyMaterial.Object);
    AddCylinder(TEXT("MetalLampHead"), 8.0f, 6.0f, 4.5f, HeadMaterial.Object);
    AddCylinder(TEXT("HeadGripRing"), 6.7f, 6.15f, 0.45f, BodyMaterial.Object);
    AddCylinder(TEXT("LensBezel"), 10.2f, 6.4f, 1.0f, BodyMaterial.Object);
    Lens = AddCylinder(TEXT("GlassLens"), 10.84f, 5.3f, 0.32f, LensMaterial.Object);

    UStaticMeshComponent* Switch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YellowSwitch"));
    Switch->SetupAttachment(PickupRoot);
    Switch->SetMobility(EComponentMobility::Movable);
    Switch->SetStaticMesh(Cube.Object);
    Switch->SetRelativeLocation(FVector(2.4f, 0, 5.6f));
    Switch->SetRelativeScale3D(FVector(0.026f, 0.015f, 0.007f));
    Switch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Switch->SetGenerateOverlapEvents(false);
    Switch->SetCanEverAffectNavigation(false);
    if (SafetyMaterial.Succeeded())
    {
        Switch->SetMaterial(0, SafetyMaterial.Object);
    }
    Details.Add(Switch);

    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("FlashlightInteraction"));
    InteractionBounds->SetupAttachment(PickupRoot);
    InteractionBounds->SetRelativeLocation(FVector(0, 0, 8));
    InteractionBounds->SetBoxExtent(FVector(16, 11, 8));
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionBounds->SetGenerateOverlapEvents(false);
    InteractionBounds->SetCanEverAffectNavigation(false);

    Label->SetupAttachment(PickupRoot);
    Label->SetRelativeLocation(FVector(0, 0, 15));
    Label->SetVisibility(false);
    Label->SetHiddenInGame(true);
    Glow->SetupAttachment(PickupRoot);
    Glow->SetRelativeLocation(FVector(0, 0, 12));
    Glow->SetIntensityUnits(ELightUnits::Lumens);
    Glow->SetIntensity(0.08f);
    Glow->SetAttenuationRadius(60.0f);
    Glow->SetLightColor(FLinearColor(0.64f, 0.76f, 0.8f));
    Glow->SetCastShadows(false);
}

void ALZFlashlightPickup::BeginPlay()
{
    Super::BeginPlay();
    if (Lens && Lens->GetMaterial(0))
    {
        // An unpowered reflective lens with a very soft search highlight, not a beam.
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Lens->GetMaterial(0), this);
        Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.35f, 0.49f, 0.53f));
        Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.18f);
        Material->SetScalarParameterValue(TEXT("Roughness"), 0.22f);
        Lens->SetMaterial(0, Material);
    }
}

void ALZFlashlightPickup::Interact(ALZCharacter* Character)
{
    if (Character && Character->AcquireFlashlight())
    {
        Destroy();
    }
}

FString ALZFlashlightPickup::GetInteractionPrompt(const ALZCharacter* Character) const
{
    return TEXT("[E] 拾取手电筒（F开关）");
}



void ALZFlashlightPickup::EnableGuideBeam()
{
    GuideBeam=NewObject<USpotLightComponent>(this,TEXT("PickupGuideBeam"));
    AddInstanceComponent(GuideBeam);GuideBeam->SetupAttachment(PickupRoot);
    GuideBeam->SetRelativeLocation(FVector(13,0,3.2f));
    GuideBeam->SetIntensityUnits(ELightUnits::Candelas);GuideBeam->SetIntensity(1.4f);
    GuideBeam->SetAttenuationRadius(1100);GuideBeam->SetInnerConeAngle(16);GuideBeam->SetOuterConeAngle(30);
    GuideBeam->SetLightColor(FLinearColor(.8f,.9f,1));GuideBeam->SetCastShadows(true);GuideBeam->RegisterComponent();
    Glow->SetIntensity(.20f);Glow->SetAttenuationRadius(280);
    if(auto* M=Cast<UMaterialInstanceDynamic>(Lens->GetMaterial(0)))
    {M->SetScalarParameterValue(TEXT("EmissiveStrength"),.35f);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.8f,.9f,1));}
}
