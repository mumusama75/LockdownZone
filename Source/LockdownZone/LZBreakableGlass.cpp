#include "LZBreakableGlass.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZBreakableGlass::ALZBreakableGlass()
{
    PrimaryActorTick.bCanEverTick = false;
    SetCanBeDamaged(true);

    GlassMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlassPane"));
    RootComponent = GlassMesh;
    GlassMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    GlassMesh->SetGenerateOverlapEvents(false);
    GlassMesh->CastShadow = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeAsset.Succeeded())
    {
        GlassMesh->SetStaticMesh(CubeAsset.Object);
    }

    // Engine translucent material keeps the office and infected visible through the pane.
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlassMaterial(
        TEXT("/Game/SampleScene/Building/Materials/Glass.Glass"));
    if (GlassMaterial.Succeeded())
    {
        GlassMesh->SetMaterial(0, GlassMaterial.Object);
    }
}

float ALZBreakableGlass::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    GlassHealth -= Applied > 0.0f ? Applied : DamageAmount;
    if (GlassHealth <= 0.0f)
    {
        const FVector Direction = DamageCauser
            ? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal()
            : GetActorForwardVector();
        Shatter(Direction);
    }
    return Applied;
}

void ALZBreakableGlass::Shatter(const FVector& ImpactDirection)
{
    UStaticMesh* MeshAsset = GlassMesh->GetStaticMesh();
    UMaterialInterface* GlassMaterial = GlassMesh->GetMaterial(0);
    const FVector PaneScale = GetActorScale3D();

    for (int32 Index = 0; Index < 12; ++Index)
    {
        const float Y = FMath::FRandRange(-310.0f, 310.0f);
        const float Z = FMath::FRandRange(-145.0f, 145.0f);
        const FVector SpawnLocation = GetActorLocation() + FVector(0.0f, Y, Z);
        AStaticMeshActor* Shard = GetWorld()->SpawnActor<AStaticMeshActor>(SpawnLocation,
            FRotator(FMath::FRandRange(-35.0f, 35.0f), FMath::FRandRange(0.0f, 180.0f), FMath::FRandRange(-35.0f, 35.0f)));
        if (!Shard)
        {
            continue;
        }
        UStaticMeshComponent* ShardMesh = Shard->GetStaticMeshComponent();
        ShardMesh->SetMobility(EComponentMobility::Movable);
        ShardMesh->SetStaticMesh(MeshAsset);
        ShardMesh->SetMaterial(0, GlassMaterial);
        ShardMesh->SetWorldScale3D(FVector(FMath::Max(0.025f, PaneScale.X * 0.45f),
            FMath::FRandRange(0.22f, 0.48f), FMath::FRandRange(0.18f, 0.42f)));
        ShardMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
        ShardMesh->SetSimulatePhysics(true);
        ShardMesh->SetEnableGravity(true);
        ShardMesh->AddImpulse((ImpactDirection * FMath::FRandRange(120.0f, 280.0f) +
            FVector(FMath::FRandRange(-80.0f, 80.0f), FMath::FRandRange(-100.0f, 100.0f),
                FMath::FRandRange(80.0f, 240.0f))) * ShardMesh->GetMass());
        Shard->SetLifeSpan(4.0f);
    }

    Destroy();
}
