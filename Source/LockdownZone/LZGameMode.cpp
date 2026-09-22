#include "LZGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/LightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LZCharacter.h"
#include "LZBreakableGlass.h"
#include "LZEnemy.h"
#include "LZExtraction.h"
#include "LZFlashlightPickup.h"
#include "LZHUD.h"
#include "LZLoot.h"
#include "LZObjective.h"
#include "LZPuzzleTerminal.h"
#include "LZWeaponPickup.h"
#include "LZPowerInteractable.h"
#include "LZSliceQA.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZGameMode::ALZGameMode()
{
    DefaultPawnClass = ALZCharacter::StaticClass();
    HUDClass = ALZHUD::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        CubeMesh = CubeAsset.Object;
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderAsset.Succeeded())
    {
        CylinderMesh = CylinderAsset.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
        TEXT("/Game/Art/ZeroTower/Materials/M_ZT_WallPlaster.M_ZT_WallPlaster"));
    if (MaterialAsset.Succeeded())
    {
        BasicMaterial = MaterialAsset.Object;
    }
}

void ALZGameMode::BeginPlay()
{
    Super::BeginPlay();
    RunStartTime = GetWorld()->GetTimeSeconds();
    StatusText = TEXT("WASD 移动 | 左键射击 | 右键瞄准 | R 换弹 | E 交互");
    BuildGrayboxLevel();

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->GetActorLocation().X < -1500.0f)
        {
            const FBox Bounds = It->GetComponentsBoundingBox(true);
            UE_LOG(LogTemp, Display, TEXT("START_ART %s class=%s loc=%s size=%s scale=%s"),
                *It->GetName(), *It->GetClass()->GetName(), *It->GetActorLocation().ToCompactString(),
                *Bounds.GetSize().ToCompactString(), *It->GetActorScale3D().ToCompactString());
        }
    }

    if (ALZCharacter* Character = Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
    {
        Character->SetActorLocation(FVector(-3300.0f, 0.0f, 110.0f));
        Character->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
        if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
        {
            PC->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (FParse::Param(FCommandLine::Get(), TEXT("LZSliceQA")))
    {
        GetWorld()->SpawnActor<ALZSliceQA>();
    }
#endif
}

AStaticMeshActor* ALZGameMode::SpawnBlock(const FString& Name, const FVector& Location, const FVector& Size,
    const FRotator& Rotation, const FLinearColor& Color)
{
    if (!CubeMesh)
    {
        return nullptr;
    }
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Block)
    {
        return nullptr;
    }
#if WITH_EDITOR
    Block->SetActorLabel(Name);
#endif
    UStaticMeshComponent* MeshComponent = Block->GetStaticMeshComponent();
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetWorldScale3D(Size / 100.0f);
    MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
    if (BasicMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicMaterial, Block);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Material->SetVectorParameterValue(TEXT("Tint"), Color);
        MeshComponent->SetMaterial(0, Material);
    }
    return Block;
}

AStaticMeshActor* ALZGameMode::SpawnArtMesh(const FString& Name, const FString& AssetPath,
    const FVector& Location, const FRotator& Rotation, const FVector& Scale, bool bCollision)
{
    UStaticMesh* ArtMesh = LoadObject<UStaticMesh>(nullptr, *AssetPath);
    if (!ArtMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Missing art mesh: %s"), *AssetPath);
        return nullptr;
    }
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Actor)
    {
        return nullptr;
    }
#if WITH_EDITOR
    Actor->SetActorLabel(Name);
#endif
    UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(ArtMesh);
    Component->SetWorldScale3D(Scale);
    // Imported Kenney pivots sit on a corner. Locations describe footprint centres,
    // so furniture, monitors and their collision occupy the intended metric positions.
    const FBoxSphereBounds AssetBounds = ArtMesh->GetBounds();
    const FVector AssetPivotOffset(AssetBounds.Origin.X, AssetBounds.Origin.Y,
        AssetBounds.Origin.Z - AssetBounds.BoxExtent.Z);
    Actor->SetActorLocation(Location - Rotation.RotateVector(AssetPivotOffset * Scale));
    Component->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
    return Actor;
}

void ALZGameMode::SpawnPipe(const FString& Name, const FVector& Location, float Radius, float Length,
    const FRotator& Rotation, const FLinearColor& Color)
{
    if (!CylinderMesh)
    {
        return;
    }
    AStaticMeshActor* Pipe = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Pipe)
    {
        return;
    }
#if WITH_EDITOR
    Pipe->SetActorLabel(Name);
#endif
    UStaticMeshComponent* Component = Pipe->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(CylinderMesh);
    Component->SetWorldScale3D(FVector(Radius / 50.0f, Radius / 50.0f, Length / 100.0f));
    Component->SetCollisionProfileName(TEXT("BlockAll"));
    if (BasicMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BasicMaterial, Pipe);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Component->SetMaterial(0, Material);
    }
}

void ALZGameMode::SpawnZoneLabel(const FString& Text, const FVector& Location, const FColor& Color)
{
    ATextRenderActor* LabelActor = GetWorld()->SpawnActor<ATextRenderActor>(Location, FRotator(0.0f, 180.0f, 0.0f));
    if (LabelActor)
    {
        LabelActor->GetTextRender()->SetText(FText::FromString(Text));
        LabelActor->GetTextRender()->SetWorldSize(70.0f);
        LabelActor->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
        LabelActor->GetTextRender()->SetTextRenderColor(Color);
    }
}

void ALZGameMode::SpawnEnemy(const FVector& Location, bool bRanged)
{
    if (ALZEnemy* Enemy = GetWorld()->SpawnActor<ALZEnemy>(Location, FRotator::ZeroRotator))
    {
        Enemy->Configure(bRanged ? EEnemyType::Raider : EEnemyType::Infected);
    }
}

void ALZGameMode::SpawnLoot(const FVector& Location, uint8 LootTypeValue)
{
    if (ALZLoot* Loot = GetWorld()->SpawnActor<ALZLoot>(Location, FRotator::ZeroRotator))
    {
        Loot->Configure(static_cast<ELootType>(LootTypeValue));
    }
}

void ALZGameMode::BuildGrayboxLevel()
{
    BuildOfficeLevel();
}

void ALZGameMode::BuildOfficeLevel()
{
    const FLinearColor Floor(0.055f, 0.06f, 0.065f);
    const FLinearColor Wall(0.17f, 0.18f, 0.19f);
    const FLinearColor Partition(0.12f, 0.16f, 0.18f);
    const FLinearColor Carpet(0.10f, 0.13f, 0.16f);
    const FLinearColor Emergency(0.32f, 0.035f, 0.025f);
    const FLinearColor OfficeBlue(0.055f, 0.20f, 0.28f);

    // One dense office floor: wake room -> open office -> service rooms -> electrical room -> exit.
    SpawnBlock(TEXT("OfficeFloor"), FVector(0.0f, 0.0f, -55.0f), FVector(7600.0f, 4200.0f, 110.0f), FRotator::ZeroRotator, Floor);
    SpawnBlock(TEXT("OfficeCeiling"), FVector(0.0f, 0.0f, 385.0f), FVector(7600.0f, 4200.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.10f, 0.115f, 0.12f));
    SpawnModularWall(TEXT("NorthExterior"), FVector(0.0f, 2100.0f, 175.0f), FVector(7600.0f, 100.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("SouthExterior"), FVector(0.0f, -2100.0f, 175.0f), FVector(7600.0f, 100.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("WestExterior"), FVector(-3800.0f, 0.0f, 175.0f), FVector(100.0f, 4200.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("EastExteriorA"), FVector(3800.0f, 1250.0f, 175.0f), FVector(100.0f, 1700.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("EastExteriorB"), FVector(3800.0f, -1250.0f, 175.0f), FVector(100.0f, 1700.0f, 350.0f), FRotator::ZeroRotator, Wall);

    // Isolated wake-up room with an observation window and a separate locked door.
    SpawnModularWall(TEXT("WakeRoomNorth"), FVector(-3150.0f, 750.0f, 175.0f), FVector(1300.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnModularWall(TEXT("WakeRoomSouth"), FVector(-3150.0f, -750.0f, 175.0f), FVector(1300.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnBlock(TEXT("WindowFrameTop"), FVector(-2520.0f, 120.0f, 330.0f), FVector(35.0f, 900.0f, 40.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnBlock(TEXT("WindowFrameBottom"), FVector(-2520.0f, 120.0f, 40.0f), FVector(35.0f, 900.0f, 80.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnBlock(TEXT("WindowFrameSideA"), FVector(-2520.0f, 535.0f, 195.0f), FVector(35.0f, 20.0f, 230.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnBlock(TEXT("WindowFrameSideB"), FVector(-2520.0f, -295.0f, 195.0f), FVector(35.0f, 20.0f, 230.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnModularWall(TEXT("WindowPierNorth"), FVector(-2520, 652, 175), FVector(40, 216, 350), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("WindowPierSouth"), FVector(-2520, -400, 175), FVector(40, 190, 350), FRotator::ZeroRotator, Wall);
    SpawnBlock(TEXT("StartDoorLintel"), FVector(-2520,-610,325), FVector(40,230,50), FRotator::ZeroRotator, Wall);
    if (ALZBreakableGlass* Glass = GetWorld()->SpawnActor<ALZBreakableGlass>(
        FVector(-2520.0f, 120.0f, 195.0f), FRotator::ZeroRotator))
    {
        Glass->SetActorScale3D(FVector(0.04f, 8.1f, 2.3f));
        Glass->SetActorLabel(TEXT("可破坏观察窗"));
    }
    StartRoomDoor = SpawnArtMesh(TEXT("WakeRoomDoor"), TEXT("/Game/Art/ZeroTower/SM_IndustrialDoor.SM_IndustrialDoor"),
        FVector(-2520,-610,0), FRotator::ZeroRotator, FVector(1));

    // Office departments and corridors.
    SpawnModularWall(TEXT("NorthOfficeWallA"), FVector(-900.0f, 900.0f, 175.0f), FVector(2300.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnModularWall(TEXT("NorthOfficeWallB"), FVector(1950.0f, 900.0f, 175.0f), FVector(1700.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnModularWall(TEXT("SouthOfficeWallA"), FVector(-300.0f, -900.0f, 175.0f), FVector(3200.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnModularWall(TEXT("SouthOfficeWallB"), FVector(2700.0f, -900.0f, 175.0f), FVector(900.0f, 80.0f, 350.0f), FRotator::ZeroRotator, Partition);
    SpawnModularWall(TEXT("ServerRoomWall"), FVector(1150.0f, -1500.0f, 175.0f), FVector(80.0f, 1200.0f, 350.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnModularWall(TEXT("ElectricalRoomWall"), FVector(2200.0f, 1500.0f, 175.0f), FVector(80.0f, 1200.0f, 350.0f), FRotator::ZeroRotator, Emergency);

    // Real art assets replace the former cube-only office furniture.
    for (int32 Desk = 0; Desk < 10; ++Desk)
    {
        const float X = -1750.0f + (Desk % 5) * 480.0f;
        const float Y = -420.0f + (Desk / 5) * 620.0f;
        const float Yaw = Desk % 2 == 0 ? 0.0f : 180.0f;
        SpawnArtMesh(FString::Printf(TEXT("Desk_%d"), Desk), TEXT("/Game/Art/KenneyFurniture/SM_desk.SM_desk"),
            FVector(X, Y, 0.0f), FRotator(0.0f, Yaw, 0.0f), FVector(0.20f));
        SpawnArtMesh(FString::Printf(TEXT("Chair_%d"), Desk), TEXT("/Game/Art/KenneyFurniture/SM_chairDesk.SM_chairDesk"),
            FVector(X + (Desk % 2 == 0 ? 105.0f : -105.0f), Y + 20.0f, 0.0f),
            FRotator(0.0f, Yaw + 180.0f, 0.0f), FVector(0.14f));
        SpawnArtMesh(FString::Printf(TEXT("Monitor_%d"), Desk), TEXT("/Game/Art/KenneyFurniture/SM_computerScreen.SM_computerScreen"),
            FVector(X - 20.0f, Y, 78.0f), FRotator(0.0f, Yaw, 0.0f), FVector(0.14f), false);
        SpawnArtMesh(FString::Printf(TEXT("Keyboard_%d"), Desk), TEXT("/Game/Art/KenneyFurniture/SM_computerKeyboard.SM_computerKeyboard"),
            FVector(X + 45.0f, Y, 78.0f), FRotator(0.0f, Yaw, 0.0f), FVector(0.14f), false);
    }
    for (int32 Rack = 0; Rack < 4; ++Rack)
    {
        SpawnArtMesh(FString::Printf(TEXT("ServerRack_%d"), Rack), TEXT("/Game/Art/ZeroTower/SM_ServerRack.SM_ServerRack"),
            FVector(1500.0f + Rack * 330.0f, -1710.0f, 0), FRotator(0,-90,0), FVector(1));
    }
    SpawnArtMesh(TEXT("ConferenceDeskA"), TEXT("/Game/Art/KenneyFurniture/SM_deskCorner.SM_deskCorner"),
        FVector(2420.0f, 150.0f, 0.0f), FRotator::ZeroRotator, FVector(0.28f));
    SpawnArtMesh(TEXT("ConferenceDeskB"), TEXT("/Game/Art/KenneyFurniture/SM_deskCorner.SM_deskCorner"),
        FVector(2820.0f, 510.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), FVector(0.28f));
    for (int32 ChairIndex = 0; ChairIndex < 6; ++ChairIndex)
    {
        const float ChairY = 30.0f + (ChairIndex % 3) * 260.0f;
        const bool bLeft = ChairIndex < 3;
        SpawnArtMesh(FString::Printf(TEXT("ConferenceChair_%d"), ChairIndex),
            TEXT("/Game/Art/KenneyFurniture/SM_chairModernFrameCushion.SM_chairModernFrameCushion"),
            FVector(bLeft ? 2320.0f : 3080.0f, ChairY, 0.0f),
            FRotator(0.0f, bLeft ? 0.0f : 180.0f, 0.0f), FVector(0.14f));
    }

    SpawnArtMesh(TEXT("ReceptionSofa"), TEXT("/Game/Art/KenneyFurniture/SM_loungeSofa.SM_loungeSofa"),
        FVector(-3000.0f, 1320.0f, 0.0f), FRotator(0.0f, 90.0f, 0.0f), FVector(0.16f));
    SpawnArtMesh(TEXT("ReceptionCoffeeTable"), TEXT("/Game/Art/KenneyFurniture/SM_tableCoffee.SM_tableCoffee"),
        FVector(-2720.0f, 1320.0f, 0.0f), FRotator::ZeroRotator, FVector(0.16f));
    SpawnArtMesh(TEXT("OfficeBookcaseA"), TEXT("/Game/Art/KenneyFurniture/SM_bookcaseOpen.SM_bookcaseOpen"),
        FVector(700.0f, 1880.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), FVector(0.20f));
    SpawnArtMesh(TEXT("OfficeBookcaseB"), TEXT("/Game/Art/KenneyFurniture/SM_bookcaseClosed.SM_bookcaseClosed"),
        FVector(1150.0f, 1880.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), FVector(0.20f));
    SpawnArtMesh(TEXT("DeadOfficePlant"), TEXT("/Game/Art/KenneyFurniture/SM_pottedPlant.SM_pottedPlant"),
        FVector(3300.0f, 1650.0f, 0.0f), FRotator::ZeroRotator, FVector(0.18f));
    SpawnArtMesh(TEXT("HallTrashcan"), TEXT("/Game/Art/KenneyFurniture/SM_trashcan.SM_trashcan"),
        FVector(400.0f, -730.0f, 0.0f), FRotator::ZeroRotator, FVector(0.15f));
    SpawnArtMesh(TEXT("ScatteredBoxA"), TEXT("/Game/Art/KenneyFurniture/SM_cardboardBoxOpen.SM_cardboardBoxOpen"),
        FVector(1050.0f, -650.0f, 0.0f), FRotator(0.0f, 28.0f, 0.0f), FVector(0.18f));
    SpawnArtMesh(TEXT("ScatteredBoxB"), TEXT("/Game/Art/KenneyFurniture/SM_cardboardBoxClosed.SM_cardboardBoxClosed"),
        FVector(1220.0f, -690.0f, 0.0f), FRotator(0.0f, -12.0f, 0.0f), FVector(0.16f));

    SpawnArtMesh(TEXT("WeaponDeskSouth"),TEXT("/Game/Art/KenneyFurniture/SM_desk.SM_desk"),
        FVector(-3300,-400,0), FRotator::ZeroRotator, FVector(.20f));
    SpawnArtMesh(TEXT("WeaponDeskNorth"),TEXT("/Game/Art/KenneyFurniture/SM_desk.SM_desk"),
        FVector(-3100,490,0), FRotator::ZeroRotator, FVector(.20f));
    SpawnArtMesh(TEXT("DeskLaptop"), TEXT("/Game/Art/KenneyFurniture/SM_laptop.SM_laptop"),
        FVector(-3050.0f, 490.0f, 78.0f), FRotator(0.0f, -20.0f, 0.0f), FVector(0.14f), false);
    if (APointLight* DeskLamp = GetWorld()->SpawnActor<APointLight>(FVector(-3100.0f, 490.0f, 155.0f), FRotator::ZeroRotator))
    {
        if (UPointLightComponent* Point = Cast<UPointLightComponent>(DeskLamp->GetLightComponent()))
        {
            Point->SetIntensity(240.0f);
            Point->SetAttenuationRadius(320.0f);
            Point->SetLightColor(FLinearColor(1.0f, 0.88f, 0.72f));
            Point->SetSourceRadius(15.0f);
            Point->SetCastShadows(false);
        }
        FacilityLights.Add(DeskLamp);
    }
    if (ALZWeaponPickup* MeleePickup = GetWorld()->SpawnActor<ALZWeaponPickup>(FVector(-3300.0f, -400.0f, 89.0f), FRotator::ZeroRotator))
    {
        MeleePickup->Configure(EPlayerWeapon::Melee);
    }
    if (ALZWeaponPickup* FirearmPickup = GetWorld()->SpawnActor<ALZWeaponPickup>(FVector(-3100.0f, 490.0f, 84.0f), FRotator::ZeroRotator))
    {
        FirearmPickup->Configure(EPlayerWeapon::Firearm);
    }
    // Optional equipment beside the pistol. The pickup origin is its bottom, at desk height.
    GetWorld()->SpawnActor<ALZFlashlightPickup>(FVector(-3150.0f, 490.0f, 77.0f), FRotator(0.0f, 20.0f, 0.0f));

    // Low tactical cover in corridor outside the observation window (non-blocking for AI/nav, waist-high cover)
    SpawnArtMesh(TEXT("CorridorCoffeeTable"), TEXT("/Game/Art/KenneyFurniture/SM_tableCoffee.SM_tableCoffee"),
        FVector(-2250.0f, 20.0f, 0.0f), FRotator::ZeroRotator, FVector(0.18f), true);
    SpawnArtMesh(TEXT("CorridorPlant"), TEXT("/Game/Art/KenneyFurniture/SM_plantSmall1.SM_plantSmall1"),
        FVector(-2250.0f, 20.0f, 45.0f), FRotator::ZeroRotator, FVector(0.16f), false);

    // Ammunition is deliberately outside the starting room: the player must conserve the first three rounds.
    SpawnLoot(FVector(-2250.0f, -520.0f, 35.0f), static_cast<uint8>(ELootType::Ammo));
    SpawnLoot(FVector(-650.0f, 620.0f, 35.0f), static_cast<uint8>(ELootType::Ammo));
    SpawnLoot(FVector(1450.0f, -1180.0f, 35.0f), static_cast<uint8>(ELootType::Ammo));
    // Optional recoveries and valuables give the north/south detours a concrete purpose.
    SpawnLoot(FVector(-2700, 1320, 60), static_cast<uint8>(ELootType::Medical));
    SpawnLoot(FVector(2850, 1650, 35), static_cast<uint8>(ELootType::Medical));
    SpawnLoot(FVector(850, 1700, 35), static_cast<uint8>(ELootType::Scrap));
    // Rare server parts are elevated on authored consoles/desks with visual sightlines from the main corridors
    SpawnArtMesh(TEXT("SouthRareCart"), TEXT("/Game/Art/KenneyFurniture/SM_desk.SM_desk"),
        FVector(2850.0f, -1750.0f, 0.0f), FRotator::ZeroRotator, FVector(0.16f, 0.16f, 0.18f), false);
    SpawnLoot(FVector(2850, -1750, 75), static_cast<uint8>(ELootType::Rare));
    SpawnArtMesh(TEXT("NorthRareDesk"), TEXT("/Game/Art/KenneyFurniture/SM_deskCorner.SM_deskCorner"),
        FVector(500.0f, 1800.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), FVector(0.22f), false);
    SpawnLoot(FVector(500, 1800, 78), static_cast<uint8>(ELootType::Rare));

    // Colleagues visible through the glass before the player can leave the room.
    SpawnEnemy(FVector(-2050.0f, 250.0f, 100.0f), false);
    SpawnEnemy(FVector(-1900.0f, -250.0f, 100.0f), false);
    SpawnEnemy(FVector(-900.0f, 300.0f, 100.0f), false);
    SpawnEnemy(FVector(-250.0f, -350.0f, 100.0f), false);
    SpawnEnemy(FVector(700.0f, 250.0f, 100.0f), false);
    SpawnEnemy(FVector(1650.0f, 300.0f, 100.0f), false);
    SpawnEnemy(FVector(2700.0f, -350.0f, 100.0f), false);

    if (ALZPowerInteractable* Fuse = GetWorld()->SpawnActor<ALZPowerInteractable>(FVector(1850.0f, -1300.0f, 83.0f), FRotator::ZeroRotator))
    {
        Fuse->Configure(EPowerInteractableType::Fuse);
        Fuse->SetActorHiddenInGame(true);
        Fuse->SetActorEnableCollision(false);
        FusePickupActor = Fuse;
    }
    if (ALZPowerInteractable* Breaker = GetWorld()->SpawnActor<ALZPowerInteractable>(FVector(2250.0f, 1510.0f, 145.0f), FRotator::ZeroRotator))
    {
        Breaker->Configure(EPowerInteractableType::Breaker);
    }

    ExitDoor = SpawnArtMesh(TEXT("PoweredExitDoor"), TEXT("/Game/Art/ZeroTower/SM_IndustrialDoor.SM_IndustrialDoor"),
        FVector(3500,0,0), FRotator::ZeroRotator, FVector(1));
    SpawnModularWall(TEXT("ExitDoorPierA"),FVector(3500,-270,175),FVector(40,300,350),FRotator::ZeroRotator,Wall);
    SpawnModularWall(TEXT("ExitDoorPierB"),FVector(3500,270,175),FVector(40,300,350),FRotator::ZeroRotator,Wall);
    SpawnBlock(TEXT("ExitDoorLintel"),FVector(3500,0,325),FVector(40,240,50),FRotator::ZeroRotator,Wall);

    // Fully enclosed evacuation airlock to physically prevent falling out of the world
    SpawnBlock(TEXT("AirlockFloor"), FVector(3850.0f, 0.0f, -55.0f), FVector(700.0f, 600.0f, 110.0f), FRotator::ZeroRotator, Floor);
    SpawnBlock(TEXT("AirlockCeiling"), FVector(3850.0f, 0.0f, 385.0f), FVector(700.0f, 600.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.10f, 0.115f, 0.12f));
    SpawnModularWall(TEXT("AirlockWallNorth"), FVector(3850.0f, 270.0f, 175.0f), FVector(700.0f, 60.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("AirlockWallSouth"), FVector(3850.0f, -270.0f, 175.0f), FVector(700.0f, 60.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnModularWall(TEXT("AirlockWallEast"), FVector(4200.0f, 0.0f, 175.0f), FVector(60.0f, 600.0f, 350.0f), FRotator::ZeroRotator, Wall);
    SpawnArtMesh(TEXT("FinalAirlockDoor"), TEXT("/Game/Art/ZeroTower/SM_IndustrialDoor.SM_IndustrialDoor"),
        FVector(4180.0f, 0.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f), FVector(1));
    SpawnBlock(TEXT("SafetyBlockerEast"), FVector(4250.0f, 0.0f, 175.0f), FVector(80.0f, 700.0f, 450.0f), FRotator::ZeroRotator, Wall);
    SpawnBlock(TEXT("SafetyBlockerNorth"), FVector(3850.0f, 320.0f, 175.0f), FVector(700.0f, 80.0f, 450.0f), FRotator::ZeroRotator, Wall);
    SpawnBlock(TEXT("SafetyBlockerSouth"), FVector(3850.0f, -320.0f, 175.0f), FVector(700.0f, 80.0f, 450.0f), FRotator::ZeroRotator, Wall);

    GetWorld()->SpawnActor<ALZExtraction>(FVector(3650.0f, 0.0f, 20.0f), FRotator::ZeroRotator);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55.0f, -25.0f, 0.0f));
    if (Sun)
    {
        Sun->GetLightComponent()->SetIntensity(0.25f);
    }
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(0.35f);
    }
    GetWorld()->SpawnActor<ASkyAtmosphere>();

    const FVector OfficeLightLocations[] = {
        FVector(-3200.0f, 0.0f, 310.0f), FVector(-1900.0f, 0.0f, 310.0f),
        FVector(-700.0f, 0.0f, 310.0f), FVector(500.0f, 0.0f, 310.0f),
        FVector(1850.0f, -1400.0f, 310.0f), FVector(2850.0f, 1400.0f, 310.0f),
        FVector(2200.0f, 200.0f, 310.0f), FVector(3400.0f, 0.0f, 310.0f)
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(OfficeLightLocations); ++Index)
    {
        if (APointLight* Light = GetWorld()->SpawnActor<APointLight>(OfficeLightLocations[Index], FRotator::ZeroRotator))
        {
            if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
            {
                Point->SetIntensity(Index == 0 ? 650.0f : 900.0f);
                Point->SetAttenuationRadius(1150.0f);
                Point->SetLightColor(Index < 4 ? FLinearColor(0.68f, 0.81f, 0.90f) : FLinearColor(1.0f, 0.68f, 0.35f));
                Point->SetSourceRadius(25.0f);
            }
            FacilityLights.Add(Light);
        }
    }

    DressOffice();
    StatusText = TEXT("你在封闭办公室醒来。透过观察窗确认情况，并寻找可以防身的武器。");
}

void ALZGameMode::BuildLegacyGrayboxLevel()
{
    // Overall slab and perimeter: 70m x 50m compact portfolio blockout.
    const FLinearColor Concrete(0.11f, 0.14f, 0.16f);
    const FLinearColor DarkFloor(0.035f, 0.045f, 0.055f);
    const FLinearColor OfficeBlue(0.08f, 0.18f, 0.24f);
    const FLinearColor Rust(0.30f, 0.11f, 0.045f);
    const FLinearColor Hazard(0.38f, 0.22f, 0.025f);
    const FLinearColor Steel(0.18f, 0.22f, 0.25f);

    SpawnBlock(TEXT("Ground"), FVector(0.0f, 0.0f, -60.0f), FVector(7000.0f, 5000.0f, 120.0f), FRotator::ZeroRotator, DarkFloor);
    SpawnBlock(TEXT("NorthWall"), FVector(0.0f, 2500.0f, 250.0f), FVector(7000.0f, 100.0f, 620.0f), FRotator::ZeroRotator, Concrete);
    SpawnBlock(TEXT("SouthWall"), FVector(0.0f, -2500.0f, 250.0f), FVector(7000.0f, 100.0f, 620.0f), FRotator::ZeroRotator, Concrete);
    SpawnBlock(TEXT("WestWall"), FVector(-3500.0f, 0.0f, 250.0f), FVector(100.0f, 5000.0f, 620.0f), FRotator::ZeroRotator, Concrete);
    SpawnBlock(TEXT("EastWall"), FVector(3500.0f, 0.0f, 250.0f), FVector(100.0f, 5000.0f, 620.0f), FRotator::ZeroRotator, Concrete);

    // Subtle floor zoning makes the route choice readable without floating arrows.
    SpawnBlock(TEXT("OfficeFloor"), FVector(0.0f, 1580.0f, 3.0f), FVector(5400.0f, 1450.0f, 8.0f), FRotator::ZeroRotator, OfficeBlue * 0.45f);
    SpawnBlock(TEXT("MaintenanceFloor"), FVector(350.0f, -1700.0f, 4.0f), FVector(5200.0f, 1150.0f, 10.0f), FRotator::ZeroRotator, Rust * 0.42f);
    SpawnBlock(TEXT("CentralYardFloor"), FVector(450.0f, -80.0f, 4.0f), FVector(3900.0f, 1450.0f, 10.0f), FRotator::ZeroRotator, FLinearColor(0.07f, 0.075f, 0.08f));

    // Partial roofs turn the side routes into authored interiors while the yard stays open.
    SpawnBlock(TEXT("OfficeRoofA"), FVector(-1450.0f, 1700.0f, 610.0f), FVector(3000.0f, 1450.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.055f, 0.07f, 0.08f));
    SpawnBlock(TEXT("OfficeRoofB"), FVector(1200.0f, 1700.0f, 610.0f), FVector(1800.0f, 1450.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.055f, 0.07f, 0.08f));
    SpawnBlock(TEXT("MaintenanceRoofA"), FVector(-900.0f, -1830.0f, 560.0f), FVector(3000.0f, 1250.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.075f, 0.045f, 0.035f));
    SpawnBlock(TEXT("MaintenanceRoofB"), FVector(1900.0f, -1830.0f, 560.0f), FVector(1800.0f, 1250.0f, 70.0f), FRotator::ZeroRotator, FLinearColor(0.075f, 0.045f, 0.035f));

    // Route separation: central loading yard, safer office route north, risky maintenance route south.
    SpawnBlock(TEXT("WarehouseDividerA"), FVector(-1500.0f, 720.0f, 210.0f), FVector(2000.0f, 100.0f, 520.0f), FRotator::ZeroRotator, Concrete);
    SpawnBlock(TEXT("WarehouseDividerB"), FVector(850.0f, 720.0f, 210.0f), FVector(1300.0f, 100.0f, 520.0f), FRotator::ZeroRotator, Concrete);
    SpawnBlock(TEXT("MaintenanceDividerA"), FVector(-900.0f, -950.0f, 210.0f), FVector(3000.0f, 100.0f, 520.0f), FRotator::ZeroRotator, Rust);
    SpawnBlock(TEXT("MaintenanceDividerB"), FVector(2100.0f, -950.0f, 210.0f), FVector(1300.0f, 100.0f, 520.0f), FRotator::ZeroRotator, Rust);
    SpawnBlock(TEXT("CoreRoomNorth"), FVector(2750.0f, 1050.0f, 210.0f), FVector(100.0f, 2700.0f, 520.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnBlock(TEXT("CoreRoomSouth"), FVector(2750.0f, -1850.0f, 210.0f), FVector(100.0f, 1200.0f, 520.0f), FRotator::ZeroRotator, OfficeBlue);
    VaultBarrier = SpawnBlock(TEXT("SealedVaultDoor"), FVector(2750.0f, -770.0f, 210.0f),
        FVector(115.0f, 850.0f, 520.0f), FRotator::ZeroRotator, FLinearColor(0.42f, 0.075f, 0.035f));

    // Cover and sightline breaks in the contested central yard.
    SpawnBlock(TEXT("Container01"), FVector(-500.0f, -150.0f, 120.0f), FVector(500.0f, 210.0f, 240.0f), FRotator::ZeroRotator, FLinearColor(0.18f, 0.32f, 0.38f));
    SpawnBlock(TEXT("Container02"), FVector(450.0f, 260.0f, 120.0f), FVector(580.0f, 210.0f, 240.0f), FRotator(0.0f, 14.0f, 0.0f), Rust);
    SpawnBlock(TEXT("Container03"), FVector(1150.0f, -350.0f, 120.0f), FVector(350.0f, 260.0f, 240.0f), FRotator::ZeroRotator, FLinearColor(0.20f, 0.25f, 0.12f));
    SpawnBlock(TEXT("OfficeCover"), FVector(-350.0f, 1500.0f, 90.0f), FVector(260.0f, 700.0f, 180.0f), FRotator::ZeroRotator, OfficeBlue);
    SpawnBlock(TEXT("TunnelCover"), FVector(450.0f, -1700.0f, 90.0f), FVector(400.0f, 260.0f, 180.0f), FRotator::ZeroRotator, Rust);
    SpawnBlock(TEXT("CoreCover01"), FVector(3100.0f, 300.0f, 110.0f), FVector(250.0f, 600.0f, 220.0f), FRotator::ZeroRotator, Steel);

    // Entry airlock silhouette and industrial framing create stronger landmarks.
    SpawnBlock(TEXT("EntryPostL"), FVector(-3270.0f, -1320.0f, 210.0f), FVector(120.0f, 120.0f, 520.0f), FRotator::ZeroRotator, Hazard);
    SpawnBlock(TEXT("EntryPostR"), FVector(-2400.0f, -1320.0f, 210.0f), FVector(120.0f, 120.0f, 520.0f), FRotator::ZeroRotator, Hazard);
    SpawnBlock(TEXT("EntryHeader"), FVector(-2835.0f, -1320.0f, 485.0f), FVector(990.0f, 130.0f, 90.0f), FRotator::ZeroRotator, Steel);
    SpawnBlock(TEXT("CranePostL"), FVector(-300.0f, 50.0f, 350.0f), FVector(90.0f, 90.0f, 820.0f), FRotator::ZeroRotator, Hazard);
    SpawnBlock(TEXT("CranePostR"), FVector(1350.0f, 50.0f, 350.0f), FVector(90.0f, 90.0f, 820.0f), FRotator::ZeroRotator, Hazard);
    SpawnBlock(TEXT("CraneBeam"), FVector(525.0f, 50.0f, 730.0f), FVector(1800.0f, 110.0f, 110.0f), FRotator::ZeroRotator, Hazard);

    // Repeated construction bays create depth and make the compound feel assembled, not boxed in.
    for (int32 Bay = 0; Bay < 6; ++Bay)
    {
        const float BayX = -2450.0f + Bay * 900.0f;
        SpawnBlock(FString::Printf(TEXT("OfficeColumn_%d"), Bay), FVector(BayX, 2320.0f, 275.0f),
            FVector(85.0f, 85.0f, 550.0f), FRotator::ZeroRotator, Steel);
        SpawnBlock(FString::Printf(TEXT("MaintenanceColumn_%d"), Bay), FVector(BayX, -2350.0f, 250.0f),
            FVector(85.0f, 85.0f, 500.0f), FRotator::ZeroRotator, Steel);
        SpawnBlock(FString::Printf(TEXT("OfficeCeilingBeam_%d"), Bay), FVector(BayX, 1720.0f, 565.0f),
            FVector(90.0f, 1300.0f, 80.0f), FRotator::ZeroRotator, FLinearColor(0.12f, 0.15f, 0.17f));
        SpawnBlock(FString::Printf(TEXT("MaintenanceCeilingBeam_%d"), Bay), FVector(BayX, -1810.0f, 515.0f),
            FVector(90.0f, 1100.0f, 80.0f), FRotator::ZeroRotator, FLinearColor(0.13f, 0.10f, 0.085f));
    }

    // Shallow wall relief keeps the perimeter from reading as a single prototype plane.
    for (int32 Panel = 0; Panel < 9; ++Panel)
    {
        const float PanelX = -2800.0f + Panel * 700.0f;
        SpawnBlock(FString::Printf(TEXT("NorthRelief_%d"), Panel), FVector(PanelX, 2435.0f, 250.0f),
            FVector(460.0f, 35.0f, 340.0f), FRotator::ZeroRotator,
            Panel % 3 == 0 ? OfficeBlue : FLinearColor(0.075f, 0.09f, 0.10f));
        SpawnBlock(FString::Printf(TEXT("SouthRelief_%d"), Panel), FVector(PanelX, -2435.0f, 230.0f),
            FVector(460.0f, 35.0f, 300.0f), FRotator::ZeroRotator,
            Panel % 3 == 1 ? Rust : FLinearColor(0.075f, 0.065f, 0.06f));
    }

    // Maintenance pipe network: visual density plus repeated cover decisions.
    SpawnPipe(TEXT("PipeMain"), FVector(450.0f, -2220.0f, 330.0f), 34.0f, 4800.0f, FRotator(0.0f, 90.0f, 0.0f), FLinearColor(0.18f, 0.34f, 0.30f));
    SpawnPipe(TEXT("PipeDropA"), FVector(-1250.0f, -2220.0f, 165.0f), 34.0f, 330.0f, FRotator::ZeroRotator, FLinearColor(0.18f, 0.34f, 0.30f));
    SpawnPipe(TEXT("PipeDropB"), FVector(2150.0f, -2220.0f, 165.0f), 34.0f, 330.0f, FRotator::ZeroRotator, FLinearColor(0.18f, 0.34f, 0.30f));

    // Hazard stripes and small prop clusters reduce the empty-box feeling.
    for (int32 Stripe = 0; Stripe < 7; ++Stripe)
    {
        SpawnBlock(FString::Printf(TEXT("HazardStripe_%d"), Stripe),
            FVector(-2250.0f + Stripe * 150.0f, -1120.0f, 8.0f), FVector(70.0f, 360.0f, 12.0f),
            FRotator(0.0f, -18.0f, 0.0f), Stripe % 2 == 0 ? Hazard : FLinearColor(0.04f, 0.04f, 0.04f));
    }
    for (int32 Crate = 0; Crate < 5; ++Crate)
    {
        SpawnBlock(FString::Printf(TEXT("SupplyCrate_%d"), Crate),
            FVector(1800.0f + (Crate % 2) * 130.0f, 1200.0f + (Crate / 2) * 120.0f, 45.0f),
            FVector(105.0f, 105.0f, 90.0f), FRotator(0.0f, Crate * 9.0f, 0.0f), Steel);
    }

    SpawnZoneLabel(TEXT("SAFE ENTRY / EXTRACTION"), FVector(-2750.0f, -2100.0f, 180.0f), FColor(80, 255, 120));
    SpawnZoneLabel(TEXT("OFFICE ROUTE  /  LOWER RISK"), FVector(-250.0f, 2250.0f, 180.0f), FColor(120, 190, 255));
    SpawnZoneLabel(TEXT("MAINTENANCE  /  HIGH VALUE"), FVector(450.0f, -2250.0f, 180.0f), FColor(255, 170, 60));
    SpawnZoneLabel(TEXT("CONTROL VAULT"), FVector(3180.0f, 2050.0f, 180.0f), FColor(80, 220, 255));

    // Environmental puzzle: clue at entry, then power -> coolant -> purifier across three routes.
    if (ALZPuzzleTerminal* Clue = GetWorld()->SpawnActor<ALZPuzzleTerminal>(FVector(-3100.0f, -720.0f, 90.0f), FRotator::ZeroRotator))
    {
        Clue->Configure(EPuzzleNode::Clue);
    }
    if (ALZPuzzleTerminal* Generator = GetWorld()->SpawnActor<ALZPuzzleTerminal>(FVector(-1250.0f, -1820.0f, 90.0f), FRotator::ZeroRotator))
    {
        Generator->Configure(EPuzzleNode::Generator);
    }
    if (ALZPuzzleTerminal* Cooling = GetWorld()->SpawnActor<ALZPuzzleTerminal>(FVector(650.0f, 1820.0f, 90.0f), FRotator::ZeroRotator))
    {
        Cooling->Configure(EPuzzleNode::Cooling);
    }
    if (ALZPuzzleTerminal* Purifier = GetWorld()->SpawnActor<ALZPuzzleTerminal>(FVector(2250.0f, -250.0f, 90.0f), FRotator::ZeroRotator))
    {
        Purifier->Configure(EPuzzleNode::Purifier);
    }

    GetWorld()->SpawnActor<ALZExtraction>(FVector(-2850.0f, -1250.0f, 20.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<ALZObjective>(FVector(3150.0f, 1650.0f, 70.0f), FRotator::ZeroRotator);

    // Safe-route supplies.
    SpawnLoot(FVector(-2050.0f, 1500.0f, 35.0f), static_cast<uint8>(ELootType::Medical));
    SpawnLoot(FVector(-650.0f, 1850.0f, 35.0f), static_cast<uint8>(ELootType::Scrap));
    SpawnLoot(FVector(1200.0f, 1650.0f, 35.0f), static_cast<uint8>(ELootType::Ammo));
    // High-risk maintenance route pays substantially more.
    SpawnLoot(FVector(-650.0f, -1800.0f, 35.0f), static_cast<uint8>(ELootType::Rare));
    SpawnLoot(FVector(900.0f, -1800.0f, 35.0f), static_cast<uint8>(ELootType::Ammo));
    SpawnLoot(FVector(1900.0f, -1700.0f, 35.0f), static_cast<uint8>(ELootType::Rare));
    SpawnLoot(FVector(3100.0f, -1200.0f, 35.0f), static_cast<uint8>(ELootType::Scrap));

    SpawnEnemy(FVector(-750.0f, 1200.0f, 100.0f), false);
    SpawnEnemy(FVector(650.0f, 1450.0f, 100.0f), true);
    SpawnEnemy(FVector(-250.0f, -1500.0f, 100.0f), false);
    SpawnEnemy(FVector(1050.0f, -1650.0f, 100.0f), false);
    SpawnEnemy(FVector(2100.0f, -1500.0f, 100.0f), true);
    SpawnEnemy(FVector(3000.0f, 1050.0f, 100.0f), true);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector, FRotator(-48.0f, -35.0f, 0.0f));
    if (Sun)
    {
        if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
        {
            SunComponent->SetIntensity(1.8f);
            SunComponent->SetLightColor(FLinearColor(0.88f, 0.78f, 0.65f));
            SunComponent->SetAtmosphereSunLight(true);
        }
    }
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(0.68f);
    }
    AExponentialHeightFog* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    if (Fog)
    {
        Fog->GetComponent()->SetFogDensity(0.007f);
        Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.055f, 0.095f, 0.12f));
        Fog->GetComponent()->SetVolumetricFog(false);
        Fog->GetComponent()->SetVolumetricFogDistance(4500.0f);
    }
    const FVector LightLocations[] = {
        FVector(-2400.0f, -1500.0f, 360.0f), FVector(-1600.0f, 1750.0f, 470.0f),
        FVector(0.0f, 0.0f, 420.0f), FVector(200.0f, -1800.0f, 430.0f),
        FVector(1800.0f, -1500.0f, 380.0f), FVector(1500.0f, 1750.0f, 470.0f),
        FVector(3100.0f, 1300.0f, 360.0f)
    };
    const FLinearColor LightColors[] = {
        FLinearColor(0.20f, 0.70f, 1.0f), FLinearColor(0.18f, 0.62f, 1.0f),
        FLinearColor(1.0f, 0.42f, 0.10f), FLinearColor(1.0f, 0.18f, 0.06f),
        FLinearColor(1.0f, 0.18f, 0.06f), FLinearColor(0.18f, 0.62f, 1.0f),
        FLinearColor(0.12f, 0.72f, 1.0f)
    };
    for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(LightLocations); ++LightIndex)
    {
        if (APointLight* Light = GetWorld()->SpawnActor<APointLight>(LightLocations[LightIndex], FRotator::ZeroRotator))
        {
            if (UPointLightComponent* PointComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
            {
                PointComponent->SetIntensity(3200.0f);
                PointComponent->SetAttenuationRadius(1250.0f);
                PointComponent->SetLightColor(LightColors[LightIndex]);
            }
        }
    }
}

void ALZGameMode::CompleteObjective()
{
    if (bObjectiveComplete || bRunOver)
    {
        return;
    }
    if (!bPuzzleComplete)
    {
        StatusText = TEXT("控制模块仍被隔离门锁定：先恢复三个系统节点");
        return;
    }
    bObjectiveComplete = true;
    StatusText = TEXT("警报：主撤离点已上线，返程路线出现新的敌人");
    SpawnReturnAmbush();
}

void ALZGameMode::SpawnReturnAmbush()
{
    SpawnEnemy(FVector(1750.0f, 250.0f, 100.0f), false);
    SpawnEnemy(FVector(550.0f, -250.0f, 100.0f), true);
    SpawnEnemy(FVector(-1250.0f, -450.0f, 100.0f), false);
}

void ALZGameMode::TryExtract(ALZCharacter* Character)
{
    if (!Character || bRunOver)
    {
        return;
    }
    if (!bObjectiveComplete)
    {
        StatusText = TEXT("无法撤离：先找到15A保险丝并恢复大楼供电");
        return;
    }
    bRunOver = true;
    bExtractionSuccessful = true;
    StatusText = TEXT("行动完成，战利品已结算");
    if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
    {
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
    }
}

void ALZGameMode::HandlePlayerDeath()
{
    if (!bRunOver)
    {
        bRunOver = true;
        bExtractionSuccessful = false;
        StatusText = TEXT("角色阵亡，本局携带物资全部丢失");
    }
}

void ALZGameMode::RestartRun()
{
    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

FString ALZGameMode::GetObjectiveText() const
{
    const ALZCharacter* Character = Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (Character && (!Character->HasMeleeWeapon() || !Character->HasFirearm()))
    {
        return TEXT("当前目标 / 在房间内寻找近战武器和手枪");
    }
    if (!bOfficeBlackout)
    {
        return TEXT("当前目标 / 杀出办公区，前往大楼出口");
    }
    if (!bHasFuse)
    {
        return TEXT("大楼断电 / 前往南侧服务器机房寻找15A保险丝");
    }
    if (!bOfficePowerRestored)
    {
        return TEXT("当前目标 / 将保险丝安装到北侧主配电箱");
    }
    return TEXT("电力已恢复 / 从东侧安全门逃离大厦");
}

void ALZGameMode::NotifyWeaponCollected(EPlayerWeapon Weapon)
{
    ALZCharacter* Character = Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Character)
    {
        return;
    }
    if (Character->HasMeleeWeapon() && Character->HasFirearm())
    {
        StatusText = TEXT("武器已备齐：按1切换消防斧，按2切换手枪。房门已解锁，击碎观察窗迎敌。");
        if (StartRoomDoor)
        {
            StartRoomDoor->Destroy();
            StartRoomDoor = nullptr;
        }
    }
    else
    {
        StatusText = Weapon == EPlayerWeapon::Melee
            ? TEXT("获得消防斧：检视完成后寻找格洛克手枪与手电。")
            : TEXT("获得格洛克17（初始弹匣3发/备用0发）：节约弹药，寻找防身斧头与手电。");
    }
}

void ALZGameMode::NotifyEnemyKilled()
{
    ++EnemiesKilled;
    if (!bOfficeBlackout && EnemiesKilled >= 4)
    {
        TriggerOfficeBlackout();
    }
}

void ALZGameMode::TriggerOfficeBlackout()
{
    bOfficeBlackout = true;
    for (UMaterialInstanceDynamic* Fixture : FacilityEmissives)
    {
        if (Fixture) Fixture->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.0f);
    }
    StatusText = TEXT("大楼突然断电：出口门失去供电。服务器机房可能有备用保险丝。");
    for (APointLight* Light : FacilityLights)
    {
        if (IsValid(Light))
        {
            if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
            {
                if (Light->GetActorLocation().X < -2500.0f && Light->GetActorLocation().Y > 200.0f)
                {
                    Point->SetIntensity(0.0f);
                }
                else
                {
                    Point->SetIntensity(65.0f);
                    Point->SetLightColor(FLinearColor(1.0f, 0.04f, 0.02f));
                }
            }
        }
    }
    if (FusePickupActor)
    {
        FusePickupActor->SetActorHiddenInGame(false);
        FusePickupActor->SetActorEnableCollision(true);
    }
    SpawnEnemy(FVector(1100.0f, -200.0f, 100.0f), false);
    SpawnEnemy(FVector(2200.0f, 350.0f, 100.0f), false);
}

void ALZGameMode::CollectFuse()
{
    if (!bOfficeBlackout || bHasFuse)
    {
        return;
    }
    bHasFuse = true;
    StatusText = TEXT("已获得15A保险丝：前往北侧配电室恢复供电。");
}

void ALZGameMode::TryRestoreOfficePower()
{
    if (!bOfficeBlackout)
    {
        StatusText = TEXT("当前供电正常，配电箱无需操作。");
        return;
    }
    if (!bHasFuse)
    {
        StatusText = TEXT("配电箱保险丝烧毁：需要找到一枚15A保险丝。");
        return;
    }
    if (bOfficePowerRestored)
    {
        return;
    }

    bOfficePowerRestored = true;
    for (UMaterialInstanceDynamic* Fixture : FacilityEmissives)
    {
        if (Fixture) Fixture->SetScalarParameterValue(TEXT("EmissiveStrength"), 2.0f);
    }
    bObjectiveComplete = true;
    StatusText = TEXT("供电恢复：东侧安全门已开启，立即撤离大厦。");
    for (APointLight* Light : FacilityLights)
    {
        if (IsValid(Light))
        {
            if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
            {
                Point->SetIntensity(900.0f);
                Point->SetLightColor(FLinearColor(0.72f, 0.88f, 1.0f));
            }
        }
    }
    if (ExitDoor)
    {
        ExitDoor->Destroy();
        ExitDoor = nullptr;
    }
}

void ALZGameMode::RegisterPuzzleTerminal(ALZPuzzleTerminal* Terminal)
{
    if (Terminal)
    {
        PuzzleTerminals.AddUnique(Terminal);
    }
}

bool ALZGameMode::IsPuzzleNodeActivated(EPuzzleNode Node) const
{
    if (Node == EPuzzleNode::Clue)
    {
        return false;
    }
    return bPuzzleComplete || static_cast<int32>(Node) < PuzzleStep;
}

void ALZGameMode::ShowPuzzleClue()
{
    StatusText = TEXT("维修记录：净化必须最后；冷却节点不能早于动力节点启动。");
}

void ALZGameMode::TryActivatePuzzleNode(EPuzzleNode Node)
{
    if (bPuzzleComplete || Node == EPuzzleNode::Clue)
    {
        return;
    }

    const EPuzzleNode ExpectedNode = static_cast<EPuzzleNode>(PuzzleStep);
    if (Node == ExpectedNode)
    {
        ++PuzzleStep;
        if (PuzzleStep >= 3)
        {
            bPuzzleComplete = true;
            StatusText = TEXT("供电恢复：隔离门已开启，可以取得净水控制模块");
            if (VaultBarrier)
            {
                VaultBarrier->Destroy();
                VaultBarrier = nullptr;
            }
        }
        else
        {
            StatusText = FString::Printf(TEXT("节点启动正确：供电恢复进度 %d/3"), PuzzleStep);
        }
    }
    else
    {
        PuzzleStep = 0;
        StatusText = TEXT("顺序错误：保护性断电，节点全部重置；噪声引来了感染者");
        SpawnEnemy(FVector(200.0f, -450.0f, 100.0f), false);
    }
    RefreshPuzzleTerminals();
}

void ALZGameMode::RefreshPuzzleTerminals()
{
    for (ALZPuzzleTerminal* Terminal : PuzzleTerminals)
    {
        if (IsValid(Terminal))
        {
            Terminal->RefreshState();
        }
    }
}

FString ALZGameMode::GetElapsedTimeText() const
{
    const float Elapsed = GetWorld() ? GetWorld()->GetTimeSeconds() - RunStartTime : 0.0f;
    const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Elapsed));
    return FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60);
}
