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
    const FLinearColor AcousticTile(0.68f, 0.70f, 0.72f);
    const FLinearColor AluminumMullion(0.035f, 0.042f, 0.050f);
    const FLinearColor FabricScreen(0.08f, 0.16f, 0.22f);

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
    // Modular 50x50cm heather slate carpet tile field under workstation pods
    for (float CX = -1950.0f; CX <= 350.0f; CX += 100.0f)
    {
        for (float CY = 75.0f; CY <= 780.0f; CY += 100.0f)
        {
            const bool bAlt = ((int32)(FMath::Abs(CX) / 100.0f) + (int32)(CY / 100.0f)) % 2 == 0;
            const FLinearColor TileColor = bAlt ? FLinearColor(0.062f, 0.072f, 0.082f) : FLinearColor(0.074f, 0.086f, 0.096f);
            Box(TEXT("CarpetTileNorth"), FVector(CX, CY, 0.4f), FVector(96, 96, 0.4f), TileColor, TEXT("M_ZT_Floor"));
        }
        for (float CY = -780.0f; CY <= -75.0f; CY += 100.0f)
        {
            const bool bAlt = ((int32)(FMath::Abs(CX) / 100.0f) + (int32)(FMath::Abs(CY) / 100.0f)) % 2 == 0;
            const FLinearColor TileColor = bAlt ? FLinearColor(0.062f, 0.072f, 0.082f) : FLinearColor(0.074f, 0.086f, 0.096f);
            Box(TEXT("CarpetTileSouth"), FVector(CX, CY, 0.4f), FVector(96, 96, 0.4f), TileColor, TEXT("M_ZT_Floor"));
        }
    }
    Box(TEXT("CarpetTransitionNorth"), FVector(-800, 52, 0.75f), FVector(2350, 4.0f, 0.35f), Chalk * 0.7f, TEXT("M_ZT_Trim"));
    Box(TEXT("CarpetTransitionSouth"), FVector(-800, -52, 0.75f), FVector(2350, 4.0f, 0.35f), Chalk * 0.7f, TEXT("M_ZT_Trim"));
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

    // Suspended acoustic mineral fiber ceiling tile panels & T-bar grid across all rooms
    auto Ceiling = [&Box, &Chalk, &AcousticTile](const FVector2D& Center, const FVector2D& Extent)
    {
        // Solid acoustic tile panel surface (eliminates void)
        Box(TEXT("CeilingTileSurface"), FVector(Center.X, Center.Y, 347.0f),
            FVector(Extent.X * 2.0f, Extent.Y * 2.0f, 2.0f), AcousticTile, TEXT("M_ZT_WallPlaster"));
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
    Ceiling(FVector2D(450, 1500), FVector2D(2810, 560));
    Ceiling(FVector2D(2180, -1510), FVector2D(960, 530));
    Ceiling(FVector2D(300, -1510), FVector2D(800, 530));
    Ceiling(FVector2D(3850, 0), FVector2D(340, 260));

    // Recessed 60x120cm troffer lights with acrylic diffusers & louvres
    const FVector Fixtures[] = {
        // Corridor main axis
        FVector(-3220, 0, 339), FVector(-1920, 0, 339), FVector(-720, 0, 339),
        FVector(480, 0, 339), FVector(1660, 0, 339), FVector(2850, 0, 339),
        // North workstation pod lighting
        FVector(-1400, 240, 339), FVector(-600, 240, 339), FVector(200, 240, 339),
        // South workstation pod lighting
        FVector(-1400, -380, 339), FVector(-600, -380, 339), FVector(200, -380, 339),
        // Operations & service suites
        FVector(1810, -1430, 339), FVector(2910, 1460, 339), FVector(-2600, 1300, 339),
        FVector(2550, 300, 339)
    };
    for (const FVector& Position : Fixtures)
    {
        Box(TEXT("FixtureHousing"), Position, FVector(124, 58, 6), Charcoal, TEXT("M_ZT_DarkMetal"));
        // Non-emissive lens follows the actual FacilityLights blackout state.
        Box(TEXT("FixtureLens"), Position - FVector(0, 0, 3.5f), FVector(114, 48, 1.5f),
            FLinearColor(0.82f, 0.88f, 0.90f), TEXT("M_ZT_WallPlaster"));
        for (int32 Slat = -2; Slat <= 2; ++Slat)
        {
            Box(TEXT("FixtureLouvre"), Position + FVector(Slat * 22, 0, -4.5f), FVector(2, 48, 2.5f),
                Charcoal, TEXT("M_ZT_DarkMetal"));
        }
    }

    // Commercial 60x60cm square 4-way stepped louvre HVAC supply air diffusers
    auto Diffuser = [&Box, &Charcoal, &Chalk](const FVector& Position)
    {
        Box(TEXT("DiffuserOuterFrame"), Position, FVector(58, 58, 2.5f), Chalk * 0.65f, TEXT("M_ZT_Trim"));
        Box(TEXT("DiffuserRecess"), Position + FVector(0, 0, 1.0f), FVector(50, 50, 2.0f), Charcoal, TEXT("M_ZT_Recess"));
        Box(TEXT("DiffuserLouver1"), Position + FVector(0, 0, 0.4f), FVector(42, 42, 1.2f), Chalk * 0.70f, TEXT("M_ZT_Trim"));
        Box(TEXT("DiffuserLouver2"), Position + FVector(0, 0, 0.8f), FVector(26, 26, 1.2f), Chalk * 0.70f, TEXT("M_ZT_Trim"));
        Box(TEXT("DiffuserCenter"), Position + FVector(0, 0, 1.2f), FVector(12, 12, 1.0f), Chalk * 0.60f, TEXT("M_ZT_Trim"));
    };
    Diffuser(FVector(-3200, 360, 344));
    Diffuser(FVector(-3200, -360, 344));
    Diffuser(FVector(-1300, 520, 344));
    Diffuser(FVector(100, 520, 344));
    Diffuser(FVector(1500, 520, 344));
    Diffuser(FVector(2700, 520, 344));
    Diffuser(FVector(-1300, -520, 344));
    Diffuser(FVector(100, -520, 344));
    Diffuser(FVector(1500, -520, 344));
    Diffuser(FVector(2700, -520, 344));
    Diffuser(FVector(1850, -1510, 344));
    Diffuser(FVector(2500, -1510, 344));
    Diffuser(FVector(-2600, 1500, 344));
    Diffuser(FVector(2500, 1500, 344));

    // Fire protection sprinkler heads
    auto Sprinkler = [&Box](const FVector& Position)
    {
        Box(TEXT("SprinklerEscutcheon"), Position, FVector(7, 7, 0.8f), FLinearColor(0.85f, 0.85f, 0.88f), TEXT("M_ZT_Trim"));
        Box(TEXT("SprinklerHead"), Position - FVector(0, 0, 2.0f), FVector(2.5f, 2.5f, 3.5f), FLinearColor(0.7f, 0.7f, 0.75f), TEXT("M_ZT_Trim"));
        Box(TEXT("SprinklerBulb"), Position - FVector(0, 0, 2.0f), FVector(1.2f, 1.2f, 2.0f), FLinearColor(0.85f, 0.08f, 0.05f), TEXT("M_ZT_WallPlaster"));
    };
    Sprinkler(FVector(-3100, 0, 344));
    Sprinkler(FVector(-1600, 0, 344));
    Sprinkler(FVector(0, 0, 344));
    Sprinkler(FVector(1200, 0, 344));
    Sprinkler(FVector(2400, 0, 344));

    Box(TEXT("MissingCeilingTile"), FVector(-780, 545, 346.5f), FVector(235, 115, 2.5f),
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

    // 10cm dark metal architectural baseboards along wall bottoms
    auto BaseboardX = [&Box, &Charcoal](float StartX, float EndX, float Y)
    {
        const float CenterX = (StartX + EndX) * 0.5f;
        const float Length = FMath::Abs(EndX - StartX);
        Box(TEXT("WallBaseboard"), FVector(CenterX, Y, 5.0f), FVector(Length, 2.5f, 10.0f),
            Charcoal, TEXT("M_ZT_DarkMetal"));
    };
    auto BaseboardY = [&Box, &Charcoal](float X, float StartY, float EndY)
    {
        const float CenterY = (StartY + EndY) * 0.5f;
        const float Length = FMath::Abs(EndY - StartY);
        Box(TEXT("WallBaseboard"), FVector(X, CenterY, 5.0f), FVector(2.5f, Length, 10.0f),
            Charcoal, TEXT("M_ZT_DarkMetal"));
    };
    BaseboardX(-3740, 3740, 2048);
    BaseboardX(-3740, 3740, -2048);
    BaseboardY(-3748, -2040, 2040);
    BaseboardY(3748, 410, 2040);
    BaseboardY(3748, -2040, -410);
    BaseboardX(-3740, -2530, 708);
    BaseboardX(-3740, -2530, -708);
    BaseboardY(-2538, 544, 708);
    BaseboardY(-2538, -495, -305);
    BaseboardX(-2040, 240, 858);
    BaseboardX(-2040, 240, 942);
    BaseboardX(1110, 2790, 858);
    BaseboardX(1110, 2790, 942);
    BaseboardX(-1890, 1290, -858);
    BaseboardX(-1890, 1290, -942);
    BaseboardX(2260, 3140, -858);
    BaseboardX(2260, 3140, -942);
    BaseboardY(1108, -2040, -910);
    BaseboardY(1192, -2040, -910);
    BaseboardY(2158, 910, 2040);
    BaseboardY(2242, 910, 2040);
    BaseboardX(-1190, -510, 1068);
    BaseboardX(-1190, -510, 1132);
    BaseboardY(-532, 1110, 1490);
    BaseboardY(-468, 1110, 1490);

    // Concrete architectural structural columns with baseboard collars and safety boxes
    auto Column = [&Box, &Charcoal, &Chalk](const FVector& Position)
    {
        Box(TEXT("StructuralColumn"), Position, FVector(56, 56, 350), Chalk * 0.72f, TEXT("M_ZT_WallPlaster"));
        Box(TEXT("ColumnBaseboard"), Position - FVector(0, 0, 170), FVector(59, 59, 10), Charcoal, TEXT("M_ZT_DarkMetal"));
        Box(TEXT("ColumnTopTrim"), Position + FVector(0, 0, 170), FVector(59, 59, 8), Charcoal, TEXT("M_ZT_DarkMetal"));
    };
    Column(FVector(-1350, 760, 175));
    Column(FVector(1200, 760, 175));
    Column(FVector(-1350, -760, 175));
    Column(FVector(1200, -760, 175));
    Box(TEXT("ExtinguisherBox"), FVector(-1320, 730, 115), FVector(20, 12, 38),
        FLinearColor(0.75f, 0.08f, 0.06f), TEXT("M_ZT_SafetyYellow"));
    Box(TEXT("FireAlarmBox"), FVector(-1320, -730, 125), FVector(14, 10, 18),
        FLinearColor(0.80f, 0.06f, 0.05f), TEXT("M_ZT_SafetyYellow"));

    // Modern architectural dark aluminum observation window mullions and frosted manifestation
    Box(TEXT("WindowVerticalMullionA"), FVector(-2520, -80, 195), FVector(35, 12, 230),
        AluminumMullion, TEXT("M_ZT_DarkMetal"));
    Box(TEXT("WindowVerticalMullionB"), FVector(-2520, 320, 195), FVector(35, 12, 230),
        AluminumMullion, TEXT("M_ZT_DarkMetal"));
    Box(TEXT("WindowSillProtrusion"), FVector(-2520, 120, 81), FVector(44, 820, 4),
        AluminumMullion, TEXT("M_ZT_DarkMetal"));
    Box(TEXT("WindowTopCasing"), FVector(-2520, 120, 310), FVector(44, 820, 4),
        AluminumMullion, TEXT("M_ZT_DarkMetal"));
    Box(TEXT("WindowManifestation"), FVector(-2520, 120, 137), FVector(2.0f, 790, 14),
        FLinearColor(0.82f, 0.88f, 0.90f), TEXT("M_ZT_WallPlaster"));
    Box(TEXT("WindowManifestationPinA"), FVector(-2520, 120, 148), FVector(2.2f, 790, 1.5f),
        FLinearColor(0.82f, 0.88f, 0.90f), TEXT("M_ZT_WallPlaster"));
    Box(TEXT("WindowManifestationPinB"), FVector(-2520, 120, 126), FVector(2.2f, 790, 1.5f),
        FLinearColor(0.82f, 0.88f, 0.90f), TEXT("M_ZT_WallPlaster"));

    // Workstation acoustic privacy divider screens and 3-drawer under-desk mobile pedestals
    for (int32 Desk = 0; Desk < 10; ++Desk)
    {
        const float X = -1750.0f + (Desk % 5) * 480.0f;
        const float Y = -420.0f + (Desk / 5) * 620.0f;
        const bool bNorth = (Desk / 5 == 1);
        const float ScreenY = bNorth ? (Y - 38.0f) : (Y + 38.0f);
        Box(TEXT("DeskScreenFabric"), FVector(X, ScreenY, 94.0f), FVector(135.0f, 3.2f, 38.0f),
            FabricScreen, TEXT("M_ZT_WallPlaster"));
        Box(TEXT("DeskScreenBracketA"), FVector(X - 50.0f, ScreenY, 76.0f), FVector(4.0f, 5.0f, 8.0f),
            Charcoal, TEXT("M_ZT_DarkMetal"));
        Box(TEXT("DeskScreenBracketB"), FVector(X + 50.0f, ScreenY, 76.0f), FVector(4.0f, 5.0f, 8.0f),
            Charcoal, TEXT("M_ZT_DarkMetal"));

        const float PedestalX = (Desk % 2 == 0) ? (X - 45.0f) : (X + 45.0f);
        const float PedestalY = bNorth ? (Y + 15.0f) : (Y - 15.0f);
        Box(TEXT("DeskPedestalBody"), FVector(PedestalX, PedestalY, 28.0f), FVector(36.0f, 48.0f, 56.0f),
            Charcoal, TEXT("M_ZT_DarkMetal"));
        for (int32 Drawer = 0; Drawer < 3; ++Drawer)
        {
            Box(TEXT("DeskPedestalDrawer"), FVector(PedestalX, PedestalY, 12.0f + Drawer * 18.0f),
                FVector(34.0f, 49.0f, 1.2f), Chalk * 0.4f, TEXT("M_ZT_Trim"));
        }
        Box(TEXT("DeskCableGrommet"), FVector(X - 25.0f, Y + 25.0f, 78.2f), FVector(7.0f, 7.0f, 0.4f),
            Charcoal, TEXT("M_ZT_DarkMetal"));
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
    Furniture(TEXT("WakeBookcase"), TEXT("SM_bookcaseOpen"), FVector(-3600, -180, 0),
        FVector(55, 100, 180), FRotator::ZeroRotator, false);
    Furniture(TEXT("WakeChairNorth"), TEXT("SM_chairDesk"), FVector(-3010, 490, 0),
        FVector(57, 58, 91), FRotator(0, 180, 0), false);
    Furniture(TEXT("WakeChairSouth"), TEXT("SM_chairDesk"), FVector(-3210, -400, 0),
        FVector(57, 58, 91), FRotator(0, 180, 0), false);
    Furniture(TEXT("WakePlant"), TEXT("SM_pottedPlant"), FVector(-3600, 300, 0),
        FVector(45, 45, 95), FRotator::ZeroRotator, false);
    Furniture(TEXT("WakeTrash"), TEXT("SM_trashcan"), FVector(-3050, 560, 0),
        FVector(25, 25, 35), FRotator::ZeroRotator, false);

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

    // Wake room emergency equipment backer board behind the axe
    Box(TEXT("EmergencyAxeMount"), FVector(-3300, -746, 140), FVector(180, 2, 85),
        Amber, TEXT("M_ZT_SafetyYellow"));
    Sign(TEXT("EmergencyAxeSign"), TEXT("EMERGENCY AXE / 应急消防斧"), FVector(-3300, -743, 165),
        90, 240, FColor(255, 220, 80), 16);

    Sign(TEXT("QuarantineWallSign"), TEXT("01 / QUARANTINE"), FVector(-3220, 705, 252), -90, 365, FColor(221, 210, 153), 27);
    Sign(TEXT("OperationsWallSign"), TEXT("02 / OPERATIONS"), FVector(-1190, 855, 272), -90, 360, FColor(143, 200, 224), 27);
    Sign(TEXT("ServerWallSign"), TEXT("03 / SERVER"), FVector(1195, -1510, 265), 0, 310, FColor(238, 181, 91), 29);
    Sign(TEXT("PowerWallSign"), TEXT("04 / POWER"), FVector(2245, 1510, 265), 0, 300, FColor(238, 181, 91), 29);
    Sign(TEXT("ExitWallSign"), TEXT("EXIT  >"), FVector(3745, 560, 270), 180, 270, FColor(121, 241, 169), 32);

    // Cross-corridor directional signs giving confirmation at key decision points
    Sign(TEXT("CrosswayServerSign"), TEXT("03 SERVER [->]"), FVector(650, -895, 230), -90, 220, FColor(238, 181, 91), 18);
    Sign(TEXT("CrosswayPowerSign"), TEXT("04 POWER [->]"), FVector(650, 895, 230), 90, 220, FColor(245, 130, 80), 18);
    Sign(TEXT("ExitOverheadSign"), TEXT("< EXIT AIRLOCK / 气闸撤离口 >"), FVector(3490, 0, 325), 180, 380, FColor(121, 241, 169), 22);

    // Reception feature wall branding & Manager whiteboard SOP clue
    Sign(TEXT("CorporateLogoSign"), TEXT("ZERO TOWER / BIOMEDICAL RESEARCH HUB"), FVector(-2500, 1750, 240), 180, 420, FColor(220, 240, 255), 24);
    Sign(TEXT("ManagerWhiteboardSign"), TEXT("SOP-17 / FACILITY REBOOT MEMO"), FVector(-850, 1880, 210), 0, 320, FColor(100, 220, 255), 18);
    Sign(TEXT("VaultLockSign"), TEXT("[02 EXECUTIVE DATA VAULT - LOCKED]"), FVector(-600, 1550, 250), -90, 340, FColor(255, 120, 80), 16);
    Sign(TEXT("ServerAisleSign"), TEXT("COLD AISLE / RESTRICTED 03-A"), FVector(2000, -1530, 290), 0, 310, FColor(120, 210, 255), 16);
    Sign(TEXT("HighVoltageSign"), TEXT("HIGH VOLTAGE 480V / 04 POWER"), FVector(2200, 1400, 240), -90, 310, FColor(255, 220, 60), 18);
    Sign(TEXT("BreakroomSign"), TEXT("BREAK ROOM & PANTRY"), FVector(200, -850, 220), 90, 260, FColor(200, 230, 200), 18);
    Sign(TEXT("WakeNoticeSign"), TEXT("QUARANTINE PROTOCOL 01-B / 隔离守则"), FVector(-3100, 743, 210), -90, 260, FColor(220, 235, 250), 16);

    // Server Room overhead landmark silhouette and hazard trim
    Box(TEXT("ServerEntryHeader"), FVector(1780, -960, 320), FVector(480, 25, 20),
        Charcoal, TEXT("M_ZT_DarkMetal"));
    for (int32 Louvre = 0; Louvre < 6; ++Louvre)
    {
        Box(TEXT("ServerEntryLouvre"), FVector(1580 + Louvre * 80, -960, 315), FVector(4, 30, 15),
            Charcoal, TEXT("M_ZT_DarkMetal"));
    }

    // Server Cold-Aisle overhead yellow cable tray network
    Box(TEXT("ServerCableTrayMain"), FVector(2000, -1530, 320), FVector(1200, 40, 8),
        Amber, TEXT("M_ZT_SafetyYellow"));
    for (int32 TrayHanger = 0; TrayHanger < 5; ++TrayHanger)
    {
        Box(TEXT("ServerCableHanger"), FVector(1500 + TrayHanger * 260, -1530, 345), FVector(6, 6, 42),
            Charcoal, TEXT("M_ZT_DarkMetal"));
    }

    // Electrical Room doorway hazard frame & transformer hazard striping
    Box(TEXT("PowerDoorFrameL"), FVector(2200, 905, 160), FVector(10, 8, 320),
        Amber, TEXT("M_ZT_SafetyYellow"));
    Box(TEXT("PowerDoorHeader"), FVector(2200, 1050, 320), FVector(10, 290, 12),
        Amber, TEXT("M_ZT_SafetyYellow"));
    for (int32 TStripe = 0; TStripe < 6; ++TStripe)
    {
        Box(TEXT("TransformerHazardStripe"), FVector(2100 + TStripe * 65, 1400, 0.95f), FVector(14, 80, 0.25f),
            Amber, TEXT("M_ZT_SafetyYellow"), FRotator(0, 35, 0));
    }

    // Server Cold-Aisle raised perforated metal floor panels
    for (int32 Tile = 0; Tile < 8; ++Tile)
    {
        Box(TEXT("ServerAisleTile"), FVector(1500.0f + Tile * 140.0f, -1530.0f, 0.7f),
            FVector(130.0f, 130.0f, 0.4f), Charcoal * 1.6f, TEXT("M_ZT_Floor"));
        Box(TEXT("ServerAislePerforation"), FVector(1500.0f + Tile * 140.0f, -1530.0f, 0.95f),
            FVector(90.0f, 90.0f, 0.15f), Charcoal, TEXT("M_ZT_Recess"));
    }
    // High-voltage anti-static insulation rubber mat in Electrical Room
    Box(TEXT("ElectricalSafetyMat"), FVector(2250.0f, 1420.0f, 0.7f), FVector(110.0f, 90.0f, 0.4f),
        FLinearColor(0.04f, 0.04f, 0.04f), TEXT("M_ZT_Floor"));

    // Battery-backed navigation survives the scripted mains blackout.
    auto Beacon = [this, &Box, &Charcoal](const FVector& Position, const FLinearColor& Color, bool bAmber, float Yaw)
    {
        const FRotator Facing(0, Yaw, 0);
        Box(TEXT("EmergencySignHousing"), Position - Facing.Vector() * 1.5f,
            FVector(4, 34, 14), Charcoal, TEXT("M_ZT_DarkMetal"), Facing);
        Box(bAmber ? TEXT("AmberEmergencyLens") : TEXT("GreenEmergencyLens"), Position,
            FVector(2, 30, 10), Color, bAmber ? TEXT("M_ZT_LEDAmber") : TEXT("M_ZT_LEDGreen"), Facing);
        if (APointLight* Lamp = GetWorld()->SpawnActor<APointLight>(Position + Facing.Vector() * 10, FRotator::ZeroRotator))
        {
#if WITH_EDITOR
            Lamp->SetActorLabel(TEXT("BatteryNavigationLamp"));
#endif
            UPointLightComponent* Point = Cast<UPointLightComponent>(Lamp->GetLightComponent());
            if (Point)
            {
                Point->SetMobility(EComponentMobility::Movable);
                Point->SetIntensityUnits(ELightUnits::Lumens);
                Point->SetIntensity(2.5f);
                Point->SetAttenuationRadius(60.0f);
                Point->SetLightColor(Color);
                Point->SetCastShadows(false);
            }
        }
    };
    Beacon(FVector(-2538, -610, 275), Green, false, 180);
    Beacon(FVector(-1110, -855, 275), Green, false, 90);
    Beacon(FVector(1195, -1180, 275), Amber, true, 0);
    Beacon(FVector(1780, -960, 290), Amber, true, 90);
    Beacon(FVector(2200, 915, 295), FLinearColor(1.0f, 0.12f, 0.08f), false, 180);
    Beacon(FVector(2245, 1220, 275), Amber, true, 0);
    Beacon(FVector(3480, 0, 305), Green, false, 180);
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

