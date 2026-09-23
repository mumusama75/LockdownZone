#include "LZEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Animation/AnimationAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ALZEnemy::ALZEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(36.0f, 82.0f);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(GetCapsuleComponent());
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
    Body->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.45f));
    // The capsule handles movement; the body explicitly catches weapon visibility traces.
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Body->SetStaticMesh(CubeMesh.Object);
    }
    Body->SetVisibility(false);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MannyMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> IdleAnimation(
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    if (IdleAnimation.Succeeded())
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(IdleAnimation.Object);
        GetMesh()->Play(true);
    }

    TypeLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TypeLabel"));
    TypeLabel->SetupAttachment(GetCapsuleComponent());
    TypeLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    TypeLabel->SetHorizontalAlignment(EHTA_Center);
    TypeLabel->SetWorldSize(24.0f);
    TypeLabel->SetTextRenderColor(FColor::Red);
    TypeLabel->SetVisibility(false); // The Chinese HUD identifies aimed-at targets.
}

void ALZEnemy::BeginPlay()
{
    Super::BeginPlay();
    Configure(EnemyType);
}

void ALZEnemy::Configure(EEnemyType NewType)
{
    EnemyType = NewType;
    if (EnemyType == EEnemyType::Raider)
    {
        MaxHealth = 90.0f;
        Health = MaxHealth;
        DetectionRange = 2400.0f;
        AttackRange = 900.0f;
        Damage = 12.0f;
        MoveSpeed = 150.0f;
        AttackCooldown = 1.5f;
        TypeLabel->SetTextRenderColor(FColor(255, 150, 60));
    }
    else
    {
        MaxHealth = 65.0f;
        Health = MaxHealth;
        DetectionRange = 1800.0f;
        AttackRange = 145.0f;
        Damage = 16.0f;
        MoveSpeed = 210.0f;
        AttackCooldown = 1.2f;
        TypeLabel->SetTextRenderColor(FColor(220, 60, 60));
    }
    RefreshHealthLabel();
    for (int32 MatIdx = 0; MatIdx < GetMesh()->GetNumMaterials(); ++MatIdx)
    {
        if (UMaterialInterface* BaseMat = GetMesh()->GetMaterial(MatIdx))
        {
            UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
            if (DynMat)
            {
                const FLinearColor EnemyTint = EnemyType == EEnemyType::Raider
                    ? FLinearColor(0.25f, 0.18f, 0.12f)
                    : FLinearColor(0.18f, 0.20f, 0.22f);
                DynMat->SetVectorParameterValue(TEXT("Tint"), EnemyTint);
                DynMat->SetVectorParameterValue(TEXT("Color"), EnemyTint);
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), EnemyTint);
                GetMesh()->SetMaterial(MatIdx, DynMat);
            }
        }
    }
    GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

void ALZEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>();
    if (GameMode && GameMode->IsRunOver())
    {
        GetCharacterMovement()->StopMovementImmediately();
        return;
    }
    ALZCharacter* Target = Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!Target || Target->GetHealth() <= 0.0f)
    {
        return;
    }

    // Preserve the opening tableau: infected remain visible behind the glass until the player is armed.
    if (!Target->HasMeleeWeapon() || !Target->HasFirearm())
    {
        GetCharacterMovement()->StopMovementImmediately();
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    ToTarget.Z = 0.0f;
    const float Distance = ToTarget.Size();
    const float EffectiveDetectionRange = Target->bIsCrouched ? (DetectionRange * 0.45f) : DetectionRange;
    if (Distance > EffectiveDetectionRange)
    {
        return;
    }

    // Line of sight check to respect office cubicle and low desk cover when sneaking
    if (Target->bIsCrouched)
    {
        FHitResult SightHit;
        FCollisionQueryParams SightParams(SCENE_QUERY_STAT(EnemySight), false, this);
        const FVector EyePos = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
        const FVector TargetEyePos = Target->GetActorLocation() + FVector(0.0f, 0.0f, 25.0f);
        if (GetWorld()->LineTraceSingleByChannel(SightHit, EyePos, TargetEyePos, ECC_Visibility, SightParams))
        {
            if (SightHit.GetActor() != Target)
            {
                return;
            }
        }
    }

    SetActorRotation(ToTarget.Rotation());
    if (Distance > AttackRange * 0.8f)
    {
        const FVector Step = ToTarget.GetSafeNormal() * MoveSpeed * DeltaSeconds;
        AddActorWorldOffset(Step, true);
    }
    if (Distance <= AttackRange && GetWorld()->GetTimeSeconds() >= NextAttackTime)
    {
        Attack(Target);
    }
}

void ALZEnemy::Attack(ALZCharacter* Target)
{
    NextAttackTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
    if (EnemyType == EEnemyType::Raider)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(RaiderTrace), true, this);
        const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
        const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor() == Target)
        {
            UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
        }
    }
    else
    {
        // Proximity alone must not let an infected attack through the observation glass or a wall.
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedMeleeTrace), false, this);
        const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
        const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
        const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
        if (!bBlocked || Hit.GetActor() == Target)
        {
            UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
        }
    }
}

float ALZEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health -= Applied;
    if (Health <= 0.0f)
    {
        if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
        {
            GameMode->NotifyEnemyKilled();
        }
        Destroy();
    }
    else
    {
        RefreshHealthLabel();
    }
    return Applied;
}

FString ALZEnemy::GetEnemyDisplayName() const
{
    return EnemyType == EEnemyType::Raider ? TEXT("掠夺者") : TEXT("感染者");
}

void ALZEnemy::RefreshHealthLabel()
{
    TypeLabel->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0f/%.0f"),
        *GetEnemyDisplayName(), FMath::Max(0.0f, Health), MaxHealth)));
}
