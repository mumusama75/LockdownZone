#include "LZEnemy.h"
#include "LZHearingAI.h"
#include "LZChapter.h"
#include "LZMotionProfile.h"
#include "LZMotionAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/BodyInstance.h"

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
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass=ALZHearingController::StaticClass();
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,360,0);
    GetCharacterMovement()->MaxAcceleration=650;
    GetCharacterMovement()->BrakingDecelerationWalking=950;
    bUseControllerRotationYaw=false;

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
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -82.0f));
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
    MotionProfile=LoadObject<ULZMotionProfile>(nullptr,TEXT("/Game/Gameplay/DA_LZMotion.DA_LZMotion"));
    if(!MotionProfile) { MotionProfile=NewObject<ULZMotionProfile>(this); MotionProfile->InitializeTemplateFallback(); }
    ApplyMotionProfile(MotionProfile);
    GetMesh()->SetCanEverAffectNavigation(false);
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Configure(EnemyType);
    UE_LOG(LogTemp,Display,TEXT("LZ_MOTION_PROFILE %s"),*MotionProfile->SourceLabel);
}

void ALZEnemy::ApplyMotionProfile(ULZMotionProfile* Profile)
{
    if(!Profile || !Profile->Mesh || !Profile->Locomotion) return;
    MotionProfile=Profile;
    GetMesh()->EmptyOverrideMaterials();
    GetMesh()->SetSkeletalMeshAsset(Profile->Mesh);
    GetMesh()->SetRelativeLocation(Profile->MeshOffset);
    GetMesh()->SetRelativeRotation(Profile->MeshRotation);
    GetMesh()->SetAnimInstanceClass(ULZMotionAnimInstance::StaticClass());
    GetMesh()->InitAnim(true);
}

void ALZEnemy::Configure(EEnemyType NewType)
{
    EnemyType = NewType;
    const bool bZombie = EnemyType==EEnemyType::Infected && bUseInfectedVisuals;
    auto* DesiredProfile=LoadObject<ULZMotionProfile>(nullptr,bZombie?TEXT("/Game/Gameplay/DA_Zombie7"):TEXT("/Game/Gameplay/DA_LZMotion"));
    if(DesiredProfile && DesiredProfile!=MotionProfile) ApplyMotionProfile(DesiredProfile);
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
    for (int32 MatIdx = 0; !(MotionProfile && MotionProfile->bPreserveMaterials) && MatIdx < GetMesh()->GetNumMaterials(); ++MatIdx)
    {
        if (UMaterialInterface* BaseMat = GetMesh()->GetMaterial(MatIdx))
        {
            UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(BaseMat);
            if (!DynMat) DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
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
    const ALZGameMode* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
    ALZCharacter* Target=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    if(!GM || GM->IsRunOver() || !GM->IsCombatUnlocked() || !Target || Target->GetHealth()<=0)
    {
        GetCharacterMovement()->StopMovementImmediately();
        return;
    }
    UpdateMotionState(DeltaSeconds);
    if(IsIncapacitated()) return;
    if(GM->UsesHearingAI())return; // The hearing-only behavior tree owns chapter movement.
    const float Now=GetWorld()->GetTimeSeconds();
    const float Distance=FVector::Dist2D(Target->GetActorLocation(),GetActorLocation());
    FCollisionQueryParams Sight(SCENE_QUERY_STAT(LZEnemySight),false,this);
    FHitResult Hit;
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation()+FVector(0,0,50),
        Target->GetActorLocation()+FVector(0,0,Target->bIsCrouched?25:35),ECC_Visibility,Sight);
    const bool Visible=Distance<(Target->bIsCrouched?DetectionRange*.45f:DetectionRange) && (!Blocked || Hit.GetActor()==Target);
    if(Visible) { LastSeenPosition=Target->GetActorLocation(); LastSeenTime=Now; }
    if(Now-LastSeenTime>3.0f) return;
    if(Visible && Distance<=AttackRange)
    {
        const FRotator Facing=(Target->GetActorLocation()-GetActorLocation()).Rotation();
        SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0,Facing.Yaw,0),DeltaSeconds,360));
        if(Now>=NextAttackTime) Attack(Target);
        return;
    }
    FVector Destination=LastSeenPosition;
    if(Now>=NextPathTime)
    {
        ChasePath=UNavigationSystemV1::FindPathToLocationSynchronously(this,GetActorLocation(),Destination,this);
        PathIndex=1;
        NextPathTime=Now+.45f;
    }
    if(ChasePath && ChasePath->IsValid() && ChasePath->PathPoints.Num()>1)
    {
        while(PathIndex<ChasePath->PathPoints.Num()-1 && FVector::Dist2D(GetActorLocation(),ChasePath->PathPoints[PathIndex])<60) ++PathIndex;
        Destination=ChasePath->PathPoints[PathIndex];
    }
    else if(!Visible) return;
    AddMovementInput((Destination-GetActorLocation()).GetSafeNormal2D());
}

void ALZEnemy::EnterMotionState(ELZEnemyMotionState State)
{
    if(State==ELZEnemyMotionState::KnockedDown)
    {
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
        GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        GetMesh()->SetSimulatePhysics(true);
        GetMesh()->WakeAllRigidBodies();
        GetMesh()->AddImpulse(-GetActorForwardVector()*25000+FVector(0,0,8000),MotionProfile->PhysicsRoot);
    }
    if(State==ELZEnemyMotionState::GettingUp)
    {
        GetMesh()->SnapshotPose(RecoveryPose);
        GetMesh()->SetSimulatePhysics(false);
        GetMesh()->AttachToComponent(GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);
        GetMesh()->SetRelativeLocation(MotionProfile->MeshOffset);
        GetMesh()->SetRelativeRotation(MotionProfile->MeshRotation);
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
    MotionState=State; StateElapsed=0;
    GetCharacterMovement()->StopMovementImmediately();
    ConsumeMovementInputVector();
    if(State==ELZEnemyMotionState::Locomotion)
    {
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        NextAttackTime=GetWorld()->GetTimeSeconds()+.4f;
    }
    else GetCharacterMovement()->DisableMovement();
}
void ALZEnemy::UpdateMotionState(float DeltaSeconds)
{
    StateElapsed+=DeltaSeconds;
    if(MotionState==ELZEnemyMotionState::Staggered && StateElapsed>=.35f) EnterMotionState(ELZEnemyMotionState::Locomotion);
    if(MotionState==ELZEnemyMotionState::KnockedDown && StateElapsed>=MotionProfile->KnockdownSeconds) EnterMotionState(ELZEnemyMotionState::GettingUp);
    if(MotionState==ELZEnemyMotionState::GettingUp && StateElapsed>=MotionProfile->GetUpSeconds) EnterMotionState(ELZEnemyMotionState::Locomotion);
}
UAnimSequence* ALZEnemy::GetActionPose(float& Time,float& Weight) const
{
    Time=0; Weight=0;
    if(!MotionProfile) return nullptr;
    UAnimSequence* Clip=nullptr;
    const float AttackElapsed=GetWorld()->GetTimeSeconds()-AttackStarted;
    if(MotionState==ELZEnemyMotionState::Locomotion && MotionProfile->Attack && AttackElapsed<.85f)
    { Clip=MotionProfile->Attack;Time=AttackElapsed*Clip->GetPlayLength()/.85f;Weight=1; }
    if(MotionState==ELZEnemyMotionState::Staggered) { Clip=MotionProfile->Stagger; Time=StateElapsed; Weight=1; }
    if(MotionState==ELZEnemyMotionState::KnockedDown) { Clip=MotionProfile->Knockdown; Time=StateElapsed; Weight=1; }
    if(MotionState==ELZEnemyMotionState::GettingUp)
    {
        Clip=MotionProfile->GetUp;
        if(Clip) { Time=StateElapsed; Weight=1; }
        else { Clip=MotionProfile->Knockdown; Time=Clip?Clip->GetPlayLength():0; Weight=1-FMath::Clamp(StateElapsed/MotionProfile->GetUpSeconds,0.f,1.f); }
    }
    if(Clip) Time=FMath::Min(Time,Clip->GetPlayLength());
    else if(MotionState!=ELZEnemyMotionState::GettingUp) Weight=0;
    return Clip;
}
void ALZEnemy::SpawnCorpse()
{
    AActor* Corpse=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),GetActorTransform());
    auto* CorpseMesh=NewObject<USkeletalMeshComponent>(Corpse);
    Corpse->SetRootComponent(CorpseMesh); Corpse->AddInstanceComponent(CorpseMesh);
    CorpseMesh->SetSkeletalMeshAsset(GetMesh()->GetSkeletalMeshAsset());
    CorpseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CorpseMesh->SetCanEverAffectNavigation(false); CorpseMesh->RegisterComponent();
    CorpseMesh->SetWorldTransform(GetMesh()->GetComponentTransform());
    for(int32 I=0;I<GetMesh()->GetNumMaterials();++I) CorpseMesh->SetMaterial(I,GetMesh()->GetMaterial(I));
    if(MotionProfile && MotionProfile->Death) CorpseMesh->PlayAnimation(MotionProfile->Death,false);
    else
    {
        CorpseMesh->SetCollisionProfileName(TEXT("Ragdoll"));
        CorpseMesh->SetSimulatePhysics(true);
        if(auto* Physics=GetMesh()->GetPhysicsAsset())
            for(const auto& Setup:Physics->SkeletalBodySetups)
                if(Setup)
                    if(auto* SourceBody=GetMesh()->GetBodyInstance(Setup->BoneName))
                        if(auto* DestBody=CorpseMesh->GetBodyInstance(Setup->BoneName))
                            DestBody->SetBodyTransform(SourceBody->GetUnrealWorldTransform(),ETeleportType::TeleportPhysics);

        CorpseMesh->AddImpulse(-GetActorForwardVector()*20000,MotionProfile?MotionProfile->PhysicsRoot:FName(TEXT("pelvis")));
    }
    Corpse->Tags.Add(TEXT("LZCorpse")); Corpse->SetLifeSpan(12);
}

void ALZEnemy::Attack(ALZCharacter* Target)
{
    if(IsIncapacitated()) return;
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
            AttackStarted=GetWorld()->GetTimeSeconds();
            UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, nullptr);
        }
    }
}

float ALZEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    const auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
    if(IsDead() || DamageAmount<=0 || (GM && GM->IsRunOver())) return 0;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health -= Applied;
    if (Health <= 0.0f)
    {
        EnterMotionState(ELZEnemyMotionState::Dead);
        SpawnCorpse();
        if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
        {
            GameMode->NotifyEnemyKilled();
        }
        Destroy();
    }
    else
    {
        // Downed enemies remain vulnerable; repeated hits cannot reset their recovery timer.
        if(MotionState==ELZEnemyMotionState::Locomotion || MotionState==ELZEnemyMotionState::Staggered)
            EnterMotionState(Applied>=45?ELZEnemyMotionState::KnockedDown:ELZEnemyMotionState::Staggered);
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

void ALZEnemy::HearNoise(FVector Position,float Radius)
{if(auto* C=Cast<ALZHearingController>(GetController()))C->ReceiveSound(Position,Radius,TEXT("Clang"));}
FVector ALZEnemy::GetHeardLocation() const
{if(auto* C=Cast<ALZHearingController>(GetController()))return C->GetLastSound();return FVector::ZeroVector;}
bool ALZEnemy::HasHeardNoise() const
{if(auto* C=Cast<ALZHearingController>(GetController()))return C->HasSoundMemory();return false;}
void ALZEnemy::TryContactAttack(ALZCharacter* P)
{if(GetWorld()->GetTimeSeconds()>=NextAttackTime)Attack(P);}
void ALZEnemy::PushFrom(FVector Position)
{
 if(IsDead() || MotionState==ELZEnemyMotionState::KnockedDown || MotionState==ELZEnemyMotionState::GettingUp)return;
 EnterMotionState(ELZEnemyMotionState::Staggered);
 FHitResult Hit;SetActorLocation(GetActorLocation()+(GetActorLocation()-Position).GetSafeNormal2D()*110,true,&Hit);
 NextAttackTime=GetWorld()->GetTimeSeconds()+1;
}
