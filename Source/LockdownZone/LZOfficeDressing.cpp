#include "LZGameMode.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void ALZGameMode::SpawnModularWall(const FString& Name, const FVector& Location, const FVector& Size,
    const FRotator& Rotation, const FLinearColor& Color)
{
    const FString AssetPath(TEXT("/Game/Art/ZeroTower/SM_OfficeWallPanel.SM_OfficeWallPanel"));
    if (!LoadObject<UStaticMesh>(nullptr, *AssetPath))
    {
        // Preserve the playable boundary if an optional art package is unavailable.
        SpawnBlock(Name, Location, Size, Rotation, Color);
        return;
    }

    const bool bAlongX = Size.X >= Size.Y;
    const float Length = bAlongX ? Size.X : Size.Y;
    const float Thickness = bAlongX ? Size.Y : Size.X;
    if (Length <= 0.0f || Thickness <= 0.0f || Size.Z <= 0.0f)
    {
        return;
    }
    const FQuat ModuleOrientation = Rotation.Quaternion() *
        FQuat(FVector::UpVector, bAlongX ? HALF_PI : 0.0f);
    UMaterialInstanceDynamic* WallTint = nullptr;
    float Covered = 0.0f;
    int32 Index = 0;
    while (Covered < Length - KINDA_SMALL_NUMBER)
    {
        const float Segment = FMath::Min(400.0f, Length - Covered);
        const float Along = -Length * 0.5f + Covered + Segment * 0.5f;
        const FVector LocalBottom(bAlongX ? Along : 0.0f, bAlongX ? 0.0f : Along, -Size.Z * 0.5f);
        AStaticMeshActor* Panel = SpawnArtMesh(FString::Printf(TEXT("%s_Panel_%02d"), *Name, Index++),
            AssetPath, Location + Rotation.RotateVector(LocalBottom), ModuleOrientation.Rotator(),
            FVector(Thickness / 10.0f, Segment / 400.0f, Size.Z / 300.0f), true);
        if (Panel)
        {
            UStaticMeshComponent* Component = Panel->GetStaticMeshComponent();
            // Retain the skirting, steel wall skirt, and narrow joint material slots.
            for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
            {
                UMaterialInterface* Material = Component->GetMaterial(Slot);
                if (Material && Material->GetName().Contains(TEXT("WallPlaster")))
                {
                    if (!WallTint)
                    {
                        WallTint = UMaterialInstanceDynamic::Create(Material, this);
                        WallTint->SetVectorParameterValue(TEXT("Tint"), Color);
                    }
                    Component->SetMaterial(Slot, WallTint);
                }
            }
        }
        Covered += Segment;
    }
}

void ALZGameMode::DressOffice()
{
    if (!CubeMesh)
    {
        return;
    }
    const FLinearColor Blue(0.055f, 0.115f, 0.155f);
    const FLinearColor Charcoal(0.025f, 0.034f, 0.039f);
    const FLinearColor Chalk(0.54f, 0.57f, 0.55f);
    const FLinearColor Amber(0.52f, 0.29f, 0.055f);
    const FLinearColor Green(0.035f, 0.46f, 0.22f);

    // Small repeated details share instanced batches, material and draw submission.
    // Nothing in these batches participates in collision, navigation or visibility traces.
    AActor* DetailActor = GetWorld()->SpawnActor<AActor>();
    if (!DetailActor)
    {
        return;
    }
#if WITH_EDITOR
    DetailActor->SetActorLabel(TEXT("OfficeSurfaceDetails"));
#endif
    TMap<FString, UInstancedStaticMeshComponent*> Batches;
    auto Box = [this, DetailActor, &Batches](const FString& Batch, const FVector& Position,
        const FVector& Dimensions, const FLinearColor& Color, const FString& MaterialName,
        const FRotator& Rotation = FRotator::ZeroRotator)
    {
        UInstancedStaticMeshComponent* Component = Batches.FindRef(Batch);
        if (!Component)
        {
            Component = NewObject<UInstancedStaticMeshComponent>(DetailActor, FName(*Batch));
            DetailActor->AddInstanceComponent(Component);
            if (!DetailActor->GetRootComponent())
            {
                DetailActor->SetRootComponent(Component);
            }
            else
            {
                Component->SetupAttachment(DetailActor->GetRootComponent());
            }
            Component->SetMobility(EComponentMobility::Movable);
            Component->SetStaticMesh(CubeMesh);
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Component->SetGenerateOverlapEvents(false);
            Component->SetCanEverAffectNavigation(false);
            Component->SetCastShadow(false);
            const FString Path = FString::Printf(TEXT("/Game/Art/ZeroTower/Materials/%s.%s"), *MaterialName, *MaterialName);
            UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *Path);
            if (!Parent)
            {
                Parent = BasicMaterial;
            }
            if (Parent)
            {
                UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Parent, DetailActor);
                Material->SetVectorParameterValue(TEXT("Tint"), Color);
                Material->SetVectorParameterValue(TEXT("Color"), Color);
                if (MaterialName.Contains(TEXT("LED")))
                {
                    Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.04f);
                }
                Component->SetMaterial(0, Material);
            }
            Component->RegisterComponent();
            Batches.Add(Batch, Component);
        }
        Component->AddInstance(FTransform(Rotation, Position, Dimensions / 100.0f), true);
    };

    // Carpet establishes the quiet starting office and the uninterrupted circulation lane.
    Box(TEXT("QuarantineCarpet"), FVector(-3160, 0, 0.5f), FVector(1140, 1360, 0.8f),
        FLinearColor(0.09f, 0.105f, 0.11f), TEXT("M_ZT_Floor"));
    Box(TEXT("OfficeRunner"), FVector(440, -65, 0.65f), FVector(5700, 230, 1.0f), Blue * 0.60f, TEXT("M_ZT_Floor"));
    for (int32 Side : {-1, 1})
    {
        Box(TEXT("CarpetEdge"), FVector(440, -65 + Side * 112, 1.25f), FVector(5700, 3, 0.25f),
            Blue * 1.7f, TEXT("M_ZT_Floor"));
    }
    for (int32 Column = 0; Column <= 14; ++Column)
    {
        Box(TEXT("FloorJoint"), FVector(-2360 + Column * 400, 0, 0.18f), FVector(1.2f, 1700, 0.2f),
            Charcoal, TEXT("M_ZT_Floor"));
    }
    for (int32 Row = -2; Row <= 2; ++Row)
    {
        Box(TEXT("FloorJoint"), FVector(440, Row * 400, 0.2f), FVector(5740, 1.2f, 0.2f),
            Charcoal, TEXT("M_ZT_Floor"));
    }
    // Server threshold remains flush and clear for the search / return route.
    Box(TEXT("ServerThreshold"), FVector(1780, -960, 0.5f), FVector(900, 110, 0.6f),
        Charcoal, TEXT("M_ZT_Floor"));
    for (int32 Stripe = 0; Stripe < 10; ++Stripe)
    {
        Box(TEXT("HazardFloorStripe"), FVector(1400 + Stripe * 82, -960, 0.95f), FVector(16, 90, 0.25f),
            Amber, TEXT("M_ZT_SafetyYellow"), FRotator(0, -28, 0));
    }

    // A suspended 120/240 cm office ceiling grid gives the rooms a believable scale.
    auto Ceiling = [&Box, &Chalk](const FVector2D& Center, const FVector2D& Extent)
    {
        for (float X = Center.X - Extent.X + 60; X < Center.X + Extent.X; X += 240)
        {
            Box(TEXT("CeilingGrid"), FVector(X, Center.Y, 345), FVector(2.5f, Extent.Y * 2, 4),
                Chalk * 0.55f, TEXT("M_ZT_Trim"));
        }
        for (float Y = Center.Y - Extent.Y + 60; Y < Center.Y + Extent.Y; Y += 120)
        {
            Box(TEXT("CeilingGrid"), FVector(Center.X, Y, 345), FVector(Extent.X * 2, 2.5f, 4),
                Chalk * 0.55f, TEXT("M_ZT_Trim"));
        }
    };
    Ceiling(FVector2D(450, 0), FVector2D(2810, 840));
    Ceiling(FVector2D(-3160, 0), FVector2D(590, 700));
    Ceiling(FVector2D(2180, -1510), FVector2D(960, 530));
    Ceiling(FVector2D(3000, 1510), FVector2D(710, 530));
    const FVector Fixtures[] = {
        FVector(-3220, 0, 339), FVector(-1920, 0, 339), FVector(-720, 0, 339),
        FVector(480, 0, 339), FVector(1660, 0, 339), FVector(2850, 0, 339),
        FVector(1810, -1430, 339), FVector(2910, 1460, 339)
    };
    for (const FVector& Position : Fixtures)
    {
        Box(TEXT("FixtureHousing"), Position, FVector(138, 42, 8), Charcoal, TEXT("M_ZT_DarkMetal"));
        // Non-emissive lens follows the actual FacilityLights blackout state.
        Box(TEXT("FixtureLens"), Position - FVector(0, 0, 4.5f), FVector(126, 32, 1.5f),
            FLinearColor(0.75f, 0.85f, 0.87f), TEXT("M_ZT_WallPlaster"));
        for (int32 Slat = -2; Slat <= 2; ++Slat)
        {
            Box(TEXT("FixtureLouvre"), Position + FVector(Slat * 23, 0, -6), FVector(2, 34, 3),
                Charcoal, TEXT("M_ZT_DarkMetal"));
        }
    }
    Box(TEXT("MissingCeilingTile"), FVector(-780, 545, 348.6f), FVector(220, 108, 1),
        FLinearColor(0.008f, 0.012f, 0.014f), TEXT("M_ZT_Recess"));
    Box(TEXT("HangingCeilingTile"), FVector(-850, 552, 318), FVector(108, 106, 3),
        Chalk * 0.65f, TEXT("M_ZT_WallPlaster"), FRotator(0, 8, 23));

    // Deliberate local damage clusters, away from pickups and the opening sightline.
    Box(TEXT("FallenCeilingPanel"), FVector(-800, 550, 5), FVector(112, 74, 3),
        Chalk * 0.65f, TEXT("M_ZT_WallPlaster"), FRotator(3, 24, 0));
    Box(TEXT("FallenCeilingPanel"), FVector(1180, 580, 3), FVector(86, 55, 3),
        Chalk * 0.65f, TEXT("M_ZT_WallPlaster"), FRotator(0, -16, 0));
    FRandomStream Scatter(9022);
    const FVector ScatterCenters[] = { FVector(-1600, -600, 0), FVector(-760, 540, 0), FVector(1180, 590, 0), FVector(2540, -550, 0) };
    for (const FVector& Center : ScatterCenters)
    {
        for (int32 Paper = 0; Paper < 6; ++Paper)
        {
            const FVector Location = Center + FVector(Scatter.FRandRange(-120, 120), Scatter.FRandRange(-85, 85), 1.6f + Paper * 0.03f);
            Box(TEXT("AbandonedPaper"), Location, FVector(21, 29.7f, 0.18f),
                FLinearColor(0.48f, 0.465f, 0.39f), TEXT("M_ZT_WallPlaster"), FRotator(0, Scatter.FRandRange(-180, 180), 0));
        }
        for (int32 Fragment = 0; Fragment < 5; ++Fragment)
        {
            Box(TEXT("PlasterChips"), Center + FVector(Scatter.FRandRange(-95, 95), Scatter.FRandRange(-70, 70), 1.5f),
                FVector(Scatter.FRandRange(8, 20), Scatter.FRandRange(6, 16), 2), Chalk * 0.62f,
                TEXT("M_ZT_ConcreteDamage"), FRotator(0, Scatter.FRandRange(-180, 180), 0));
        }
        for (int32 Smear = 0; Smear < 3; ++Smear)
        {
            Box(TEXT("DustScuffs"), Center + FVector(Smear * 22 - 30, Smear * 18 - 24, 0.55f),
                FVector(95 - Smear * 13, 20 + Smear * 8, 0.15f), FLinearColor(0.085f, 0.078f, 0.062f),
                TEXT("M_ZT_ConcreteDamage"), FRotator(0, 17 + Smear * 11, 0));
        }
    }
    // Surface scars sit just proud of the real north wall (inside face Y = 860).
    for (int32 Scar = 0; Scar < 5; ++Scar)
    {
        Box(TEXT("WallPlasterLoss"), FVector(-640 + Scar * 34, 858.5f, 105 + (Scar % 3) * 19),
            FVector(68 - Scar * 6, 0.6f, 43 + (Scar % 2) * 15), FLinearColor(0.19f, 0.20f, 0.19f),
            TEXT("M_ZT_ConcreteDamage"), FRotator(0, 0, 0));
    }
    for (int32 Streak = 0; Streak < 7; ++Streak)
    {
        Box(TEXT("WallWaterStain"), FVector(-550 + Streak * 20, 858.0f, 230 - Streak * 8),
            FVector(5 + Streak % 3, 0.4f, 80 + Streak * 11), FLinearColor(0.14f, 0.16f, 0.15f),
            TEXT("M_ZT_ConcreteDamage"));
    }

    // Furniture is fitted to centimetres from asset bounds, including rotated floor contact.
    auto Furniture = [this](const FString& Name, const FString& MeshName, const FVector& Position,
        const FVector& DesiredSize, const FRotator& Rotation, bool bCollision)
    {
        const FString Path = FString::Printf(TEXT("/Game/Art/KenneyFurniture/%s.%s"), *MeshName, *MeshName);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh)
        {
            return;
        }
        const FVector NativeSize = Mesh->GetBounds().BoxExtent * 2;
        const FVector Scale(DesiredSize.X / FMath::Max(NativeSize.X, 1.0),
            DesiredSize.Y / FMath::Max(NativeSize.Y, 1.0), DesiredSize.Z / FMath::Max(NativeSize.Z, 1.0));
        AStaticMeshActor* Item = SpawnArtMesh(Name, Path, Position, Rotation, Scale, bCollision);
        if (Item)
        {
            const FBox Bounds = Item->GetComponentsBoundingBox(true);
            Item->AddActorWorldOffset(FVector(0, 0, Position.Z - Bounds.Min.Z));
            if (!bCollision)
            {
                Item->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
            }
        }
    };
    Furniture(TEXT("FuseServiceBench"), TEXT("SM_desk"), FVector(1850, -1300, 0),
        FVector(170, 85, 78), FRotator::ZeroRotator, true);
    Furniture(TEXT("ServicePartsBox"), TEXT("SM_cardboardBoxOpen"), FVector(2015, -1290, 0),
        FVector(43, 40, 34), FRotator(0, 17, 0), false);
    Furniture(TEXT("AbandonedChairA"), TEXT("SM_chairDesk"), FVector(-1150, -670, 0),
        FVector(57, 58, 91), FRotator(0, 28, 72), false);
    Furniture(TEXT("AbandonedChairB"), TEXT("SM_chairDesk"), FVector(1420, 590, 0),
        FVector(57, 58, 91), FRotator(0, -30, 83), false);
    Furniture(TEXT("QuarantineFiles"), TEXT("SM_cardboardBoxClosed"), FVector(-3650, 590, 0),
        FVector(48, 44, 40), FRotator(0, -8, 0), false);
    Furniture(TEXT("QuarantineFilesTop"), TEXT("SM_cardboardBoxOpen"), FVector(-3650, 590, 40),
        FVector(42, 39, 33), FRotator(0, 11, 0), false);

    // TextRender faces local +X. Backing boards and labels share an actual wall surface.
    auto Sign = [this, &Box, &Charcoal](const FString& Name, const FString& Text, const FVector& Position,
        float Yaw, float Width, const FColor& TextColor, float TextSize)
    {
        const FRotator Facing(0, Yaw, 0);
        const FVector Normal = Facing.Vector();
        Box(TEXT("SignBacking"), Position - Normal * 2, FVector(2, Width, 56),
            Charcoal, TEXT("M_ZT_DarkMetal"), Facing);
        if (ATextRenderActor* Label = GetWorld()->SpawnActor<ATextRenderActor>(Position, Facing))
        {
#if WITH_EDITOR
            Label->SetActorLabel(Name);
#endif
            UTextRenderComponent* TextComponent = Label->GetTextRender();
            TextComponent->SetText(FText::FromString(Text));
            TextComponent->SetWorldSize(TextSize);
            TextComponent->SetHorizontalAlignment(EHTA_Center);
            TextComponent->SetVerticalAlignment(EVRTA_TextCenter);
            TextComponent->SetTextRenderColor(TextColor);
            TextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    };
    Sign(TEXT("QuarantineWallSign"), TEXT("01 / QUARANTINE"), FVector(-3220, 705, 252), -90, 365, FColor(221, 210, 153), 27);
    Sign(TEXT("OperationsWallSign"), TEXT("02 / OPERATIONS"), FVector(-1190, 855, 272), -90, 360, FColor(143, 200, 224), 27);
    Sign(TEXT("ServerWallSign"), TEXT("03 / SERVER"), FVector(1195, -1510, 265), 0, 310, FColor(238, 181, 91), 29);
    Sign(TEXT("PowerWallSign"), TEXT("04 / POWER"), FVector(2245, 1510, 265), 0, 300, FColor(238, 181, 91), 29);
    Sign(TEXT("ExitWallSign"), TEXT("EXIT  >"), FVector(3745, 560, 270), 180, 270, FColor(121, 241, 169), 32);

    // Battery-backed navigation survives the scripted mains blackout.
    auto Beacon = [this, &Box](const FVector& Position, const FLinearColor& Color, bool bAmber, float Yaw)
    {
        const FRotator Facing(0, Yaw, 0);
        Box(bAmber ? TEXT("AmberEmergencyLens") : TEXT("GreenEmergencyLens"), Position,
            FVector(3, 32, 9), Color, bAmber ? TEXT("M_ZT_LEDAmber") : TEXT("M_ZT_LEDGreen"), Facing);
        if (APointLight* Lamp = GetWorld()->SpawnActor<APointLight>(Position + Facing.Vector() * 12, FRotator::ZeroRotator))
        {
#if WITH_EDITOR
            Lamp->SetActorLabel(TEXT("BatteryNavigationLamp"));
#endif
            UPointLightComponent* Point = Cast<UPointLightComponent>(Lamp->GetLightComponent());
            if (Point)
            {
                Point->SetMobility(EComponentMobility::Movable);
                Point->SetIntensityUnits(ELightUnits::Lumens);
                Point->SetIntensity(15.0f);
                Point->SetAttenuationRadius(240.0f);
                Point->SetLightColor(Color);
                Point->SetCastShadows(false);
            }
        }
    };
    Beacon(FVector(-2790, -705, 48), Green, false, 90);
    Beacon(FVector(-1110, -855, 48), Green, false, 90);
    Beacon(FVector(1195, -1180, 48), Amber, true, 0);
    Beacon(FVector(2245, 1220, 48), Amber, true, 0);
    Beacon(FVector(3745, 520, 225), Green, false, 180);
    for (int32 Mark = 0; Mark < 5; ++Mark)
    {
        Box(TEXT("ExitFloorGuide"), FVector(3050 + Mark * 90, -155, 1.3f), FVector(24, 7, 0.4f),
            Green * 0.75f, TEXT("M_ZT_LEDGreen"));
    }

    if (APostProcessVolume* Look = GetWorld()->SpawnActor<APostProcessVolume>())
    {
#if WITH_EDITOR
        Look->SetActorLabel(TEXT("OfficeExposureAndLens"));
#endif
        Look->bUnbound = true;
        Look->Priority = 5.0f;
        Look->BlendWeight = 1.0f;
        Look->Settings.bOverride_AutoExposureMinBrightness = true;
        Look->Settings.bOverride_AutoExposureMaxBrightness = true;
        // EV100 is deliberately fixed for this low-output, enclosed office so
        // the blackout cannot be undone by automatic eye-adaptation gain.
        Look->Settings.AutoExposureMinBrightness = -5.0f;
        Look->Settings.AutoExposureMaxBrightness = -5.0f;
        Look->Settings.bOverride_VignetteIntensity = true;
        Look->Settings.VignetteIntensity = 0.30f;
    }
}

