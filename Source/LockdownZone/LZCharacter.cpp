#include "LZCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LZGameMode.h"
#include "LZInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const TCHAR* LZInventoryItemDisplayName(ELZInventoryItemType Type)
    {
        switch (Type)
        {
        case ELZInventoryItemType::Ammo: return TEXT("9毫米弹药");
        case ELZInventoryItemType::Medical: return TEXT("医疗包");
        case ELZInventoryItemType::Scrap: return TEXT("电子零件");
        case ELZInventoryItemType::Rare: return TEXT("服务器备件");
        default: return TEXT("物品");
        }
    }
}

ALZCharacter::ALZCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
    GetCharacterMovement()->MaxWalkSpeed = 480.0f;
    GetCharacterMovement()->JumpZVelocity = 520.0f;
    GetCharacterMovement()->AirControl = 0.25f;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetFieldOfView(90.0f);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    WeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBody"));
    WeaponBody->SetupAttachment(FirstPersonCamera);
    WeaponBody->SetRelativeLocation(FVector(48.0f, 22.0f, -22.0f));
    WeaponBody->SetRelativeScale3D(FVector(0.55f, 0.10f, 0.10f));
    WeaponBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponBody->CastShadow = false;

    WeaponBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBarrel"));
    WeaponBarrel->SetupAttachment(FirstPersonCamera);
    WeaponBarrel->SetRelativeLocation(FVector(85.0f, 22.0f, -18.0f));
    WeaponBarrel->SetRelativeScale3D(FVector(0.30f, 0.035f, 0.035f));
    WeaponBarrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponBarrel->CastShadow = false;

    WeaponGrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponGrip"));
    WeaponGrip->SetupAttachment(FirstPersonCamera);
    WeaponGrip->SetRelativeLocation(FVector(51.0f, 22.0f, -31.0f));
    WeaponGrip->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
    WeaponGrip->SetRelativeScale3D(FVector(0.20f, 0.095f, 0.28f));
    WeaponGrip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponGrip->CastShadow = false;

    WeaponMagazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMagazine"));
    WeaponMagazine->SetupAttachment(FirstPersonCamera);
    WeaponMagazine->SetRelativeLocation(FVector(50.0f, 22.0f, -36.0f));
    WeaponMagazine->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
    WeaponMagazine->SetRelativeScale3D(FVector(0.13f, 0.075f, 0.21f));
    WeaponMagazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMagazine->CastShadow = false;

    FrontSight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontSight"));
    FrontSight->SetupAttachment(FirstPersonCamera);
    FrontSight->SetRelativeLocation(FVector(89.0f, 22.0f, -12.0f));
    FrontSight->SetRelativeScale3D(FVector(0.035f, 0.025f, 0.04f));
    FrontSight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FrontSight->CastShadow = false;

    if (CubeAsset.Succeeded())
    {
        WeaponBody->SetStaticMesh(CubeAsset.Object);
        WeaponBarrel->SetStaticMesh(CubeAsset.Object);
        WeaponGrip->SetStaticMesh(CubeAsset.Object);
        WeaponMagazine->SetStaticMesh(CubeAsset.Object);
        FrontSight->SetStaticMesh(CubeAsset.Object);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PistolAsset(
        TEXT("/Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol"));
    if (PistolAsset.Succeeded())
    {
        WeaponBody->SetStaticMesh(PistolAsset.Object);
        WeaponBody->SetRelativeLocation(FVector(62.0f, 20.0f, -27.0f));
        WeaponBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        WeaponBody->SetRelativeScale3D(FVector(20.4f / (PistolAsset.Object->GetBounds().BoxExtent.GetMax() * 2.0f)));
    }

    MeleeHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeleeHandle"));
    MeleeHandle->SetupAttachment(FirstPersonCamera);
    MeleeHandle->SetRelativeLocation(FVector(58.0f, 23.0f, -28.0f));
    MeleeHandle->SetRelativeRotation(FRotator(0.0f, 38.0f, -18.0f));
    MeleeHandle->SetRelativeScale3D(FVector(0.042f, 0.042f, 0.72f));
    MeleeHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeleeHandle->CastShadow = false;

    MeleeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeleeHead"));
    MeleeHead->SetupAttachment(MeleeHandle);
    MeleeHead->SetAbsolute(false, false, true);
    MeleeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 44.0f));
    MeleeHead->SetRelativeScale3D(FVector(0.22f, 0.045f, 0.14f));
    MeleeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeleeHead->CastShadow = false;
    if (CubeAsset.Succeeded())
    {
        MeleeHandle->SetStaticMesh(CubeAsset.Object);
        MeleeHead->SetStaticMesh(CubeAsset.Object);
    }

    // Keep close first-person meshes out of the scene flashlight's lighting channel.
    UStaticMeshComponent* FirstPersonMeshes[] = {
        WeaponBody, WeaponBarrel, WeaponGrip, WeaponMagazine, FrontSight, MeleeHandle, MeleeHead
    };
    for (UStaticMeshComponent* FirstPersonMesh : FirstPersonMeshes)
    {
        FirstPersonMesh->SetLightingChannels(false, true, false);
    }

    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
    MuzzleLight->SetupAttachment(FirstPersonCamera);
    MuzzleLight->SetRelativeLocation(FVector(112.0f, 22.0f, -18.0f));
    MuzzleLight->SetIntensity(1800.0f);
    MuzzleLight->SetAttenuationRadius(260.0f);
    MuzzleLight->SetLightColor(FLinearColor(1.0f, 0.42f, 0.08f));
    MuzzleLight->SetVisibility(false);

    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
    Flashlight->SetupAttachment(FirstPersonCamera);
    Flashlight->SetRelativeLocation(FVector(18.0f, 8.0f, -8.0f));
    Flashlight->SetRelativeRotation(FRotator::ZeroRotator);
    Flashlight->SetMobility(EComponentMobility::Movable);
    Flashlight->SetUseInverseSquaredFalloff(true);
    Flashlight->SetIntensityUnits(ELightUnits::Candelas);
    // Calibrated to this slice's fixed EV100=-5 exposure to avoid close-range desk overexposure while retaining corridor throw.
    Flashlight->SetIntensity(1.60f);
    Flashlight->SetLightingChannels(true, false, false);
    Flashlight->SetAttenuationRadius(1800.0f);
    Flashlight->SetInnerConeAngle(10.0f);
    Flashlight->SetOuterConeAngle(28.0f);
    Flashlight->SetSourceRadius(2.5f);
    Flashlight->SetUseTemperature(true);
    Flashlight->SetTemperature(4800.0f);
    Flashlight->SetCastShadows(true);
    Flashlight->SetShadowBias(0.25f);
    Flashlight->SetVisibility(false);

    WeaponFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WeaponFillLight"));
    WeaponFillLight->SetupAttachment(FirstPersonCamera);
    WeaponFillLight->SetRelativeLocation(FVector(15.0f, 0.0f, 15.0f));
    WeaponFillLight->SetMobility(EComponentMobility::Movable);
    WeaponFillLight->SetUseInverseSquaredFalloff(true);
    WeaponFillLight->SetIntensityUnits(ELightUnits::Unitless);
    WeaponFillLight->SetIntensity(25.0f);
    WeaponFillLight->SetAttenuationRadius(140.0f);
    WeaponFillLight->SetLightColor(FLinearColor(0.82f, 0.88f, 1.0f));
    WeaponFillLight->SetLightingChannels(false, true, false);
    WeaponFillLight->SetCastShadows(false);
    WeaponFillLight->SetVisibility(false);

    WeaponBody->SetVisibility(false);
    WeaponBarrel->SetVisibility(false);
    WeaponGrip->SetVisibility(false);
    WeaponMagazine->SetVisibility(false);
    FrontSight->SetVisibility(false);
    MeleeHandle->SetVisibility(false, true);
    // Attachment children are not registered during construction: propagation alone
    // can leave the axe head visible in front of the unarmed camera.
    MeleeHead->SetVisibility(false);
}

void ALZCharacter::BeginPlay()
{
    Super::BeginPlay();
    UpdateWeaponVisibility();
    SetFlashlightEnabled(false);
    UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicMaterial)
    {
        UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.055f, 0.07f, 0.08f));
        if (WeaponBody->GetStaticMesh() && WeaponBody->GetStaticMesh()->GetPathName().Contains(TEXT("BasicShapes")))
        {
            WeaponBody->SetMaterial(0, BodyMaterial);
        }
        UMaterialInstanceDynamic* BarrelMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
        BarrelMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.30f, 0.16f, 0.05f));
        WeaponBarrel->SetMaterial(0, BarrelMaterial);
        WeaponGrip->SetMaterial(0, BodyMaterial);
        WeaponMagazine->SetMaterial(0, BarrelMaterial);
        FrontSight->SetMaterial(0, BarrelMaterial);

        UMaterialInterface* DarkMetal = LoadObject<UMaterialInterface>(
            nullptr, TEXT("/Game/Art/ZeroTower/Materials/M_ZT_DarkMetal.M_ZT_DarkMetal"));
        UMaterialInterface* PaintedSteel = LoadObject<UMaterialInterface>(
            nullptr, TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel"));
        if (DarkMetal)
        {
            MeleeHandle->SetMaterial(0, DarkMetal);
        }
        else
        {
            MeleeHandle->SetMaterial(0, BarrelMaterial);
        }
        if (PaintedSteel)
        {
            UMaterialInstanceDynamic* AxeHeadMaterial = UMaterialInstanceDynamic::Create(PaintedSteel, this);
            AxeHeadMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.82f, 0.08f, 0.06f));
            AxeHeadMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.08f, 0.06f));
            MeleeHead->SetMaterial(0, AxeHeadMaterial);
        }
        else
        {
            MeleeHead->SetMaterial(0, BodyMaterial);
        }
    }
}

void ALZCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsRunInactive())
    {
        SetInventoryOpen(false);
        SetFlashlightEnabled(false);
        if (bAiming || bReloading || bInspectingWeapon || RecoilVisualAmount > 0.0f || RecoilPitch > 0.0f)
        {
            CancelWeaponActions();
        }
        return;
    }
    // Recovery is independent of the animation branches below; their early returns cannot strand recoil.
    UpdateRecoil(DeltaSeconds);
    if (bReloading)
    {
        ReloadElapsed += DeltaSeconds;
        const float Alpha = FMath::Clamp(ReloadElapsed / ReloadDuration, 0.0f, 1.0f);
        const float Arc = FMath::Sin(Alpha * PI);
        const float Drop = FMath::Sin(FMath::Clamp(Alpha * 1.7f, 0.0f, 1.0f) * PI * 0.5f);
        WeaponBody->SetRelativeLocation(FVector(62.0f, 20.0f, -27.0f - 22.0f * Arc));
        WeaponBarrel->SetRelativeLocation(FVector(85.0f, 22.0f, -18.0f - 22.0f * Arc));
        WeaponGrip->SetRelativeLocation(FVector(51.0f, 22.0f, -31.0f - 22.0f * Arc));
        FrontSight->SetRelativeLocation(FVector(89.0f, 22.0f, -12.0f - 22.0f * Arc));
        WeaponMagazine->SetRelativeLocation(FVector(50.0f, 22.0f, -36.0f - 42.0f * Drop));
        WeaponBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 38.0f * Arc));
        WeaponBarrel->SetRelativeRotation(FRotator(0.0f, 0.0f, 38.0f * Arc));
        if (Alpha >= 1.0f)
        {
            FinishReload();
        }
        return;
    }
    if (!bInspectingWeapon)
    {
        return;
    }

    InspectElapsed += DeltaSeconds;
    const float Alpha = FMath::Clamp(InspectElapsed / 1.35f, 0.0f, 1.0f);
    const float Arc = FMath::Sin(Alpha * PI);
    if (SelectedWeapon == EPlayerWeapon::Firearm)
    {
        WeaponBody->SetRelativeRotation(FRotator(-12.0f * Arc, -90.0f + 135.0f * Arc, 20.0f * Arc));
        WeaponBarrel->SetRelativeRotation(FRotator(-12.0f * Arc, 135.0f * Arc, 20.0f * Arc));
        WeaponGrip->SetRelativeRotation(FRotator(-12.0f * Arc, 135.0f * Arc, -12.0f + 20.0f * Arc));
        WeaponMagazine->SetRelativeRotation(FRotator(-12.0f * Arc, 135.0f * Arc, -12.0f + 20.0f * Arc));
        FrontSight->SetRelativeRotation(FRotator(-12.0f * Arc, 135.0f * Arc, 20.0f * Arc));
    }
    else if (SelectedWeapon == EPlayerWeapon::Melee)
    {
        MeleeHandle->SetRelativeRotation(FRotator(-20.0f * Arc, 38.0f + 150.0f * Arc, -18.0f));
    }
    if (Alpha >= 1.0f)
    {
        bInspectingWeapon = false;
        WeaponBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        WeaponBarrel->SetRelativeRotation(FRotator::ZeroRotator);
        WeaponGrip->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
        WeaponMagazine->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
        FrontSight->SetRelativeRotation(FRotator::ZeroRotator);
        MeleeHandle->SetRelativeRotation(FRotator(0.0f, 38.0f, -18.0f));
    }
}

void ALZCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ALZCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ALZCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ALZCharacter::StartJump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ALZCharacter::StartFire);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &ALZCharacter::StartAim);
    PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &ALZCharacter::StopAim);
    PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &ALZCharacter::Reload);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ALZCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("WeaponMelee"), IE_Pressed, this, &ALZCharacter::SelectMeleeWeapon);
    PlayerInputComponent->BindAction(TEXT("WeaponFirearm"), IE_Pressed, this, &ALZCharacter::SelectFirearm);
    PlayerInputComponent->BindAction(TEXT("RestartRun"), IE_Pressed, this, &ALZCharacter::RestartRun);
    PlayerInputComponent->BindAction(TEXT("ToggleFlashlight"), IE_Pressed, this, &ALZCharacter::ToggleFlashlight);
    PlayerInputComponent->BindAction(TEXT("ToggleInventory"), IE_Pressed, this, &ALZCharacter::ToggleInventory);
    PlayerInputComponent->BindAction(TEXT("InventoryLeft"), IE_Pressed, this, &ALZCharacter::InventoryLeft);
    PlayerInputComponent->BindAction(TEXT("InventoryRight"), IE_Pressed, this, &ALZCharacter::InventoryRight);
    PlayerInputComponent->BindAction(TEXT("InventoryUp"), IE_Pressed, this, &ALZCharacter::InventoryUp);
    PlayerInputComponent->BindAction(TEXT("InventoryDown"), IE_Pressed, this, &ALZCharacter::InventoryDown);
    PlayerInputComponent->BindAction(TEXT("InventoryDiscard"), IE_Pressed, this, &ALZCharacter::InventoryDiscard);
}

void ALZCharacter::MoveForward(float Value)
{
    if (!bInventoryOpen && !IsRunInactive() && Controller && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FRotationMatrix(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X), Value);
    }
}

void ALZCharacter::MoveRight(float Value)
{
    if (!bInventoryOpen && !IsRunInactive() && Controller && !FMath::IsNearlyZero(Value))
    {
        AddMovementInput(FRotationMatrix(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y), Value);
    }
}

void ALZCharacter::StartJump()
{
    if (!bInventoryOpen && !IsRunInactive()) Jump();
}

void ALZCharacter::StartFire()
{
    if (bInventoryOpen || IsRunInactive()) return;
    if (SelectedWeapon == EPlayerWeapon::None || bInspectingWeapon || bReloading)
    {
        return;
    }

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector ShotDirection = FirstPersonCamera->GetForwardVector();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), true, this);
    if (SelectedWeapon == EPlayerWeapon::Melee)
    {
        const FVector End = Start + ShotDirection * 240.0f;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor())
        {
            UGameplayStatics::ApplyPointDamage(Hit.GetActor(), 55.0f, ShotDirection,
                Hit, GetController(), this, nullptr);
        }
        MeleeHandle->SetRelativeRotation(FRotator(-35.0f, -25.0f, 45.0f));
        FTimerHandle ResetHandle;
        GetWorldTimerManager().SetTimer(ResetHandle, this, &ALZCharacter::ResetMeleePose, 0.18f, false);
        return;
    }

    if (AmmoInMagazine <= 0)
    {
        Reload();
        return;
    }

    --AmmoInMagazine;
    MuzzleLight->SetVisibility(true);
    GetWorldTimerManager().SetTimerForNextTick(this, &ALZCharacter::HideMuzzleFlash);
    const FVector End = Start + ShotDirection * 8000.0f;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor())
    {
        UGameplayStatics::ApplyPointDamage(Hit.GetActor(), WeaponDamage, ShotDirection,
            Hit, GetController(), this, nullptr);
    }
    // Resolve this round along the pre-kick aim. Only a consumed firearm round reaches this point.
    ApplyShotRecoil();
}

void ALZCharacter::HideMuzzleFlash()
{
    MuzzleLight->SetVisibility(false);
}

void ALZCharacter::ResetMeleePose()
{
    MeleeHandle->SetRelativeRotation(FRotator(0.0f, 38.0f, -18.0f));
}

void ALZCharacter::StartAim()
{
    if (bInventoryOpen || IsRunInactive() || SelectedWeapon != EPlayerWeapon::Firearm || bInspectingWeapon || bReloading)
    {
        return;
    }
    bAiming = true;
    FirstPersonCamera->SetFieldOfView(70.0f);
    ApplyWeaponRecoilPose();
    WeaponBarrel->SetRelativeLocation(FVector(86.0f, 5.0f, -15.0f));
}

void ALZCharacter::StopAim()
{
    bAiming = false;
    FirstPersonCamera->SetFieldOfView(90.0f);
    ApplyWeaponRecoilPose();
    WeaponBarrel->SetRelativeLocation(FVector(85.0f, 22.0f, -18.0f));
}

void ALZCharacter::Reload()
{
    if (bInventoryOpen || IsRunInactive()) return;
    if (!bHasFirearm || SelectedWeapon != EPlayerWeapon::Firearm || bInspectingWeapon || bReloading ||
        AmmoInMagazine >= MagazineSize || GetReserveAmmo() <= 0)
    {
        return;
    }
    ResetRecoil();
    StopAim();
    bReloading = true;
    ReloadElapsed = 0.0f;
}

void ALZCharacter::FinishReload()
{
    const int32 Needed = MagazineSize - AmmoInMagazine;
    const int32 Loaded = ConsumeStoredAmmo(Needed);
    AmmoInMagazine += Loaded;
    bReloading = false;
    ReloadElapsed = 0.0f;
    WeaponBody->SetRelativeLocation(FVector(62.0f, 20.0f, -27.0f));
    WeaponBarrel->SetRelativeLocation(FVector(85.0f, 22.0f, -18.0f));
    WeaponGrip->SetRelativeLocation(FVector(51.0f, 22.0f, -31.0f));
    WeaponMagazine->SetRelativeLocation(FVector(50.0f, 22.0f, -36.0f));
    FrontSight->SetRelativeLocation(FVector(89.0f, 22.0f, -12.0f));
    WeaponBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    WeaponBarrel->SetRelativeRotation(FRotator::ZeroRotator);
}

void ALZCharacter::AcquireWeapon(EPlayerWeapon Weapon)
{
    if (bInventoryOpen || IsRunInactive()) return;
    CancelWeaponActions();
    if (Weapon == EPlayerWeapon::Melee)
    {
        bHasMeleeWeapon = true;
    }
    else if (Weapon == EPlayerWeapon::Firearm)
    {
        bHasFirearm = true;
        AmmoInMagazine = 3;
    }
    SelectedWeapon = Weapon;
    InspectElapsed = 0.0f;
    bInspectingWeapon = true;
    UpdateWeaponVisibility();
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        GameMode->NotifyWeaponCollected(Weapon);
    }
}

void ALZCharacter::SelectMeleeWeapon()
{
    if (!bInventoryOpen && !IsRunInactive() && bHasMeleeWeapon && SelectedWeapon != EPlayerWeapon::Melee)
    {
        CancelWeaponActions();
        SelectedWeapon = EPlayerWeapon::Melee;
        UpdateWeaponVisibility();
    }
}

void ALZCharacter::SelectFirearm()
{
    if (!bInventoryOpen && !IsRunInactive() && bHasFirearm && SelectedWeapon != EPlayerWeapon::Firearm)
    {
        CancelWeaponActions();
        SelectedWeapon = EPlayerWeapon::Firearm;
        UpdateWeaponVisibility();
    }
}

void ALZCharacter::UpdateWeaponVisibility()
{
    const bool bShowFirearm = SelectedWeapon == EPlayerWeapon::Firearm;
    WeaponBody->SetVisibility(bShowFirearm);
    WeaponBarrel->SetVisibility(false);
    WeaponGrip->SetVisibility(false);
    WeaponMagazine->SetVisibility(false);
    FrontSight->SetVisibility(false);
    const bool bShowMelee = SelectedWeapon == EPlayerWeapon::Melee;
    MeleeHandle->SetVisibility(bShowMelee, true);
    MeleeHead->SetVisibility(bShowMelee);
    WeaponFillLight->SetVisibility(bShowFirearm || bShowMelee);
}

FString ALZCharacter::GetSelectedWeaponName() const
{
    switch (SelectedWeapon)
    {
    case EPlayerWeapon::Melee: return TEXT("消防斧");
    case EPlayerWeapon::Firearm: return TEXT("格洛克17 · 9毫米");
    default: return TEXT("无武器");
    }
}

ALZInteractable* ALZCharacter::FindInteractable(float Range) const
{
    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * Range;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        return Cast<ALZInteractable>(Hit.GetActor());
    }
    return nullptr;
}

void ALZCharacter::Interact()
{
    if (IsRunInactive()) return;
    if (bInventoryOpen)
    {
        UseSelectedInventoryItem();
        return;
    }
    if (ALZInteractable* Target = FindInteractable())
    {
        Target->Interact(this);
    }
}

bool ALZCharacter::AddLoot(int32 Value, int32 Slots, int32 AmmoAmount, int32 HealAmount)
{
    // Compatibility for legacy callers. Commit one storage plan, never a second value/ammo counter.
    if (IsRunInactive() || Value < 0 || Slots < 0 || AmmoAmount < 0 || HealAmount < 0) return false;
    TArray<FLZInventoryEntry> PlannedEntries = InventoryEntries;
    if (AmmoAmount > 0 && !PlanInventoryStorage(PlannedEntries, ELZInventoryItemType::Ammo, AmmoAmount)) return false;
    if (HealAmount > 0)
    {
        if (HealAmount % 35 != 0 || !PlanInventoryStorage(PlannedEntries, ELZInventoryItemType::Medical, HealAmount / 35)) return false;
    }
    if (Value > 0)
    {
        if (Value % 500 == 0 && Slots == 2 * (Value / 500))
        {
            if (!PlanInventoryStorage(PlannedEntries, ELZInventoryItemType::Rare, Value / 500)) return false;
        }
        else if (Value % 120 == 0 && Slots == Value / 120)
        {
            if (!PlanInventoryStorage(PlannedEntries, ELZInventoryItemType::Scrap, Value / 120)) return false;
        }
        else return false;
    }
    else if (Slots != 0) return false;
    InventoryEntries = MoveTemp(PlannedEntries);
    InventoryStatusText = TEXT("物资已放入背包");
    return true;
}

float ALZCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (const ALZGameMode* GM = GetWorld()->GetAuthGameMode<ALZGameMode>(); GM && GM->IsRunOver()) return 0.0f;
    const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    Health = FMath::Max(0.0f, Health - Applied);
    if (Health <= 0.0f)
    {
        SetInventoryOpen(false);
        SetFlashlightEnabled(false);
        CancelWeaponActions();
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);
        }
        if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
        {
            GameMode->HandlePlayerDeath();
        }
    }
    return Applied;
}

void ALZCharacter::RestartRun()
{
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        GameMode->RestartRun();
    }
}

bool ALZCharacter::IsRunInactive() const
{
    if (Health <= 0.0f) return true;
    const ALZGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALZGameMode>() : nullptr;
    return GameMode && GameMode->IsRunOver();
}

void ALZCharacter::AcquireFlashlight()
{
    if (IsRunInactive()) return;
    bHasFlashlight = true;
    SetFlashlightEnabled(true);
}

void ALZCharacter::ToggleFlashlight()
{
    if (!bHasFlashlight || IsRunInactive())
    {
        SetFlashlightEnabled(false);
        return;
    }
    SetFlashlightEnabled(!bFlashlightOn);
}

void ALZCharacter::SetFlashlightEnabled(bool bEnabled)
{
    bFlashlightOn = bEnabled && bHasFlashlight && !IsRunInactive();
    if (Flashlight) Flashlight->SetVisibility(bFlashlightOn);
}

float ALZCharacter::ApplyCameraRecoilDelta(float PitchDelta, float YawDelta)
{
    if (!Controller) return 0.0f;
    FRotator Rotation = Controller->GetControlRotation();
    const float BeforePitch = FRotator::NormalizeAxis(Rotation.Pitch);
    float PitchMin = -89.0f;
    float PitchMax = 89.0f;
    if (const APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (PC->PlayerCameraManager)
        {
            PitchMin = PC->PlayerCameraManager->ViewPitchMin;
            PitchMax = PC->PlayerCameraManager->ViewPitchMax;
        }
    }
    Rotation.Pitch = FMath::Clamp(BeforePitch + PitchDelta, PitchMin, PitchMax);
    Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + YawDelta);
    Controller->SetControlRotation(Rotation);
    // Make the immediate shot kick visible to the camera and subsequent traces this frame.
    FirstPersonCamera->SetWorldRotation(Rotation);
    return Rotation.Pitch - BeforePitch;
}

void ALZCharacter::ApplyShotRecoil()
{
    const float PitchKick = bAiming ? 0.80f : 1.45f;
    const float YawKick = (bAiming ? 0.08f : 0.18f) * ((RecoilShotCounter++ & 1u) ? -1.0f : 1.0f);
    const float PitchToApply = FMath::Min(PitchKick, FMath::Max(0.0f, MaxRecoilPitch - RecoilPitch));
    const float NewYaw = FMath::Clamp(RecoilYaw + YawKick, -0.7f, 0.7f);
    if (Controller)
    {
        RecoilPitch += ApplyCameraRecoilDelta(PitchToApply, NewYaw - RecoilYaw);
        RecoilYaw = NewYaw;
    }
    RecoilVisualAmount = FMath::Min(RecoilVisualAmount + (bAiming ? 0.55f : 1.0f), 1.8f);
    RecoilStartPitch = RecoilPitch;
    RecoilStartYaw = RecoilYaw;
    RecoilStartVisualAmount = RecoilVisualAmount;
    RecoilElapsed = 0.0f;
    ApplyWeaponRecoilPose();
}

void ALZCharacter::UpdateRecoil(float DeltaSeconds)
{
    if (RecoilVisualAmount <= 0.0f && FMath::IsNearlyZero(RecoilPitch) && FMath::IsNearlyZero(RecoilYaw)) return;
    RecoilElapsed = FMath::Min(RecoilElapsed + DeltaSeconds, RecoilRecoveryDuration);
    const float Alpha = RecoilElapsed / RecoilRecoveryDuration;
    const float Remaining = 1.0f - Alpha * Alpha * (3.0f - 2.0f * Alpha);
    const float NewPitch = RecoilStartPitch * Remaining;
    const float NewYaw = RecoilStartYaw * Remaining;
    // Remove only our outstanding offsets from the current look rotation, preserving new mouse input.
    ApplyCameraRecoilDelta(NewPitch - RecoilPitch, NewYaw - RecoilYaw);
    RecoilPitch = NewPitch;
    RecoilYaw = NewYaw;
    RecoilVisualAmount = RecoilStartVisualAmount * Remaining;
    ApplyWeaponRecoilPose();
}

void ALZCharacter::ApplyWeaponRecoilPose()
{
    if (SelectedWeapon != EPlayerWeapon::Firearm || bReloading || bInspectingWeapon) return;
    const FVector BaseLocation = bAiming ? FVector(65.0f, 2.0f, -22.0f) : FVector(62.0f, 20.0f, -27.0f);
    WeaponBody->SetRelativeLocation(BaseLocation + FVector(-4.5f, 0.0f, 0.8f) * RecoilVisualAmount);
    const FQuat UpwardKick = FRotator(7.0f * RecoilVisualAmount, 0.0f, 0.0f).Quaternion();
    WeaponBody->SetRelativeRotation(UpwardKick * FRotator(0.0f, -90.0f, 0.0f).Quaternion());
}

void ALZCharacter::ResetRecoil()
{
    if (!FMath::IsNearlyZero(RecoilPitch) || !FMath::IsNearlyZero(RecoilYaw))
    {
        ApplyCameraRecoilDelta(-RecoilPitch, -RecoilYaw);
    }
    RecoilPitch = RecoilYaw = RecoilVisualAmount = 0.0f;
    RecoilStartPitch = RecoilStartYaw = RecoilStartVisualAmount = RecoilElapsed = 0.0f;
    ApplyWeaponRecoilPose();
}

void ALZCharacter::CancelWeaponActions()
{
    bReloading = false;
    ReloadElapsed = 0.0f;
    bInspectingWeapon = false;
    InspectElapsed = 0.0f;
    ResetRecoil();
    StopAim();
    WeaponBody->SetRelativeLocation(FVector(62.0f, 20.0f, -27.0f));
    WeaponBody->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    WeaponBarrel->SetRelativeLocation(FVector(85.0f, 22.0f, -18.0f));
    WeaponBarrel->SetRelativeRotation(FRotator::ZeroRotator);
    WeaponGrip->SetRelativeLocation(FVector(51.0f, 22.0f, -31.0f));
    WeaponGrip->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
    WeaponMagazine->SetRelativeLocation(FVector(50.0f, 22.0f, -36.0f));
    WeaponMagazine->SetRelativeRotation(FRotator(0.0f, 0.0f, -12.0f));
    FrontSight->SetRelativeLocation(FVector(89.0f, 22.0f, -12.0f));
    FrontSight->SetRelativeRotation(FRotator::ZeroRotator);
    ResetMeleePose();
    HideMuzzleFlash();
}

int32 ALZCharacter::GetReserveAmmo() const
{
    int32 Total = 0;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.Type == ELZInventoryItemType::Ammo) Total += Entry.Quantity;
    }
    return Total;
}

int32 ALZCharacter::GetUsedBagSlots() const
{
    int32 Total = 0;
    for (const FLZInventoryEntry& Entry : InventoryEntries) Total += Entry.SlotsPerItem;
    return Total;
}

int32 ALZCharacter::GetLootValue() const
{
    int32 Total = 0;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.Type == ELZInventoryItemType::Scrap) Total += 120 * Entry.Quantity;
        else if (Entry.Type == ELZInventoryItemType::Rare) Total += 500 * Entry.Quantity;
    }
    return Total;
}

const FLZInventoryEntry* ALZCharacter::GetInventoryItemAtSlot(int32 Slot) const
{
    if (Slot < 0 || Slot >= InventorySlotCount) return nullptr;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Slot >= Entry.StartSlot && Slot < Entry.StartSlot + Entry.SlotsPerItem) return &Entry;
    }
    return nullptr;
}

bool ALZCharacter::PlanInventoryStorage(TArray<FLZInventoryEntry>& Entries, ELZInventoryItemType Type, int32 Quantity) const
{
    if (Quantity <= 0) return false;
    switch (Type)
    {
    case ELZInventoryItemType::Ammo:
    case ELZInventoryItemType::Medical:
    case ELZInventoryItemType::Scrap:
    case ELZInventoryItemType::Rare:
        break;
    default:
        return false;
    }
    const int32 StackLimit = Type == ELZInventoryItemType::Ammo ? AmmoStackLimit : 1;
    if (Quantity > InventorySlotCount * StackLimit) return false;

    int32 Remaining = Quantity;
    if (Type == ELZInventoryItemType::Ammo)
    {
        for (FLZInventoryEntry& Entry : Entries)
        {
            if (Entry.Type != Type || Entry.Quantity >= AmmoStackLimit) continue;
            const int32 Added = FMath::Min(Remaining, AmmoStackLimit - Entry.Quantity);
            Entry.Quantity += Added;
            Remaining -= Added;
            if (Remaining == 0) return true;
        }
    }

    const int32 RequiredSlots = Type == ELZInventoryItemType::Rare ? 2 : 1;
    while (Remaining > 0)
    {
        bool Occupied[InventorySlotCount] = { false };
        for (const FLZInventoryEntry& Entry : Entries)
        {
            for (int32 Offset = 0; Offset < Entry.SlotsPerItem; ++Offset)
            {
                const int32 Slot = Entry.StartSlot + Offset;
                if (Slot >= 0 && Slot < InventorySlotCount) Occupied[Slot] = true;
            }
        }
        int32 Placement = INDEX_NONE;
        for (int32 Slot = 0; Slot <= InventorySlotCount - RequiredSlots; ++Slot)
        {
            // A two-cell item cannot straddle the right edge of the three-column grid.
            if (Slot / InventoryColumnCount != (Slot + RequiredSlots - 1) / InventoryColumnCount) continue;
            bool bFree = true;
            for (int32 Offset = 0; Offset < RequiredSlots; ++Offset) bFree &= !Occupied[Slot + Offset];
            if (bFree)
            {
                Placement = Slot;
                break;
            }
        }
        if (Placement == INDEX_NONE) return false;

        FLZInventoryEntry NewEntry;
        NewEntry.Type = Type;
        NewEntry.Quantity = FMath::Min(Remaining, StackLimit);
        NewEntry.StartSlot = Placement;
        NewEntry.SlotsPerItem = RequiredSlots;
        Entries.Add(NewEntry);
        Remaining -= NewEntry.Quantity;
    }
    Entries.Sort([](const FLZInventoryEntry& A, const FLZInventoryEntry& B) { return A.StartSlot < B.StartSlot; });
    return true;
}

bool ALZCharacter::CanStoreItem(ELZInventoryItemType Type, int32 Quantity) const
{
    TArray<FLZInventoryEntry> PlannedEntries = InventoryEntries;
    return PlanInventoryStorage(PlannedEntries, Type, Quantity);
}

bool ALZCharacter::TryStoreItem(ELZInventoryItemType Type, int32 Quantity)
{
    if (IsRunInactive()) return false;
    TArray<FLZInventoryEntry> PlannedEntries = InventoryEntries;
    if (!PlanInventoryStorage(PlannedEntries, Type, Quantity))
    {
        InventoryStatusText = Type == ELZInventoryItemType::Rare
            ? TEXT("背包空间不足：服务器备件需要同一行两个连续空格。按B整理背包。")
            : TEXT("背包空间不足：物品未拾取，按B使用或丢弃物品后再试。");
        return false;
    }
    InventoryEntries = MoveTemp(PlannedEntries);
    InventoryStatusText = FString::Printf(TEXT("已收纳%s ×%d · 占用%d/6格"),
        LZInventoryItemDisplayName(Type), Quantity, GetUsedBagSlots());
    return true;
}

int32 ALZCharacter::ConsumeStoredAmmo(int32 RequestedRounds)
{
    int32 Remaining = FMath::Max(0, RequestedRounds);
    const int32 Requested = Remaining;
    for (int32 Index = 0; Index < InventoryEntries.Num() && Remaining > 0;)
    {
        FLZInventoryEntry& Entry = InventoryEntries[Index];
        if (Entry.Type != ELZInventoryItemType::Ammo)
        {
            ++Index;
            continue;
        }
        const int32 Loaded = FMath::Min(Remaining, Entry.Quantity);
        Entry.Quantity -= Loaded;
        Remaining -= Loaded;
        if (Entry.Quantity == 0) InventoryEntries.RemoveAt(Index);
        else ++Index;
    }
    return Requested - Remaining;
}

void ALZCharacter::SetInventoryOpen(bool bOpen)
{
    if (bOpen == bInventoryOpen || (bOpen && IsRunInactive())) return;
    bInventoryOpen = bOpen;
    if (bOpen)
    {
        CancelWeaponActions();
        StopJumping();
        ConsumeMovementInputVector();
        GetCharacterMovement()->StopMovementImmediately();
        SelectedInventorySlot = FMath::Clamp(SelectedInventorySlot, 0, InventorySlotCount - 1);
        if (InventoryStatusText.IsEmpty()) InventoryStatusText = TEXT("方向键选格，E使用医疗包");
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            // Ignore-input flags are reference-counted. Own exactly one layer, leaving game-over locks intact.
            InventoryInputController = PC;
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);
            bInventoryInputLockApplied = true;
            PC->bShowMouseCursor = true;
        }
    }
    else if (bInventoryInputLockApplied)
    {
        if (APlayerController* PC = InventoryInputController.Get())
        {
            PC->SetIgnoreMoveInput(false);
            PC->SetIgnoreLookInput(false);
            PC->bShowMouseCursor = false;
        }
        InventoryInputController.Reset();
        bInventoryInputLockApplied = false;
    }
    // The Canvas inventory stays in GameOnly input: B, E, Delete, F and F5 keep their normal bindings.
}

void ALZCharacter::ToggleInventory()
{
    if (IsRunInactive())
    {
        SetInventoryOpen(false);
        return;
    }
    SetInventoryOpen(!bInventoryOpen);
}

void ALZCharacter::MoveInventorySelection(int32 ColumnDelta, int32 RowDelta)
{
    if (!bInventoryOpen || IsRunInactive()) return;
    const int32 Column = FMath::Clamp(SelectedInventorySlot % InventoryColumnCount + ColumnDelta, 0, InventoryColumnCount - 1);
    const int32 Row = FMath::Clamp(SelectedInventorySlot / InventoryColumnCount + RowDelta, 0, InventorySlotCount / InventoryColumnCount - 1);
    SelectedInventorySlot = Row * InventoryColumnCount + Column;
}

void ALZCharacter::InventoryLeft() { MoveInventorySelection(-1, 0); }
void ALZCharacter::InventoryRight() { MoveInventorySelection(1, 0); }
void ALZCharacter::InventoryUp() { MoveInventorySelection(0, -1); }
void ALZCharacter::InventoryDown() { MoveInventorySelection(0, 1); }
void ALZCharacter::InventoryDiscard() { DiscardSelectedInventoryItem(); }

bool ALZCharacter::UseSelectedInventoryItem()
{
    if (!bInventoryOpen || IsRunInactive()) return false;
    const FLZInventoryEntry* Selected = GetInventoryItemAtSlot(SelectedInventorySlot);
    if (!Selected)
    {
        InventoryStatusText = TEXT("当前格没有物品");
        return false;
    }
    if (Selected->Type != ELZInventoryItemType::Medical)
    {
        InventoryStatusText = Selected->Type == ELZInventoryItemType::Ammo
            ? TEXT("关闭背包后按R：从弹药堆栈中装填手枪")
            : TEXT("战利品无需使用，成功撤离后结算价值");
        return false;
    }
    if (Health >= MaxHealth)
    {
        InventoryStatusText = TEXT("生命已满：医疗包未消耗");
        return false;
    }
    const float RestoredHealth = FMath::Min(35.0f, MaxHealth - Health);
    const int32 StartSlot = Selected->StartSlot;
    Health += RestoredHealth;
    InventoryEntries.RemoveAll([StartSlot](const FLZInventoryEntry& Entry) { return Entry.StartSlot == StartSlot; });
    InventoryStatusText = FString::Printf(TEXT("已使用医疗包：恢复%.0f生命，释放1格空间"), RestoredHealth);
    return true;
}

bool ALZCharacter::DiscardSelectedInventoryItem()
{
    if (!bInventoryOpen || IsRunInactive()) return false;
    const FLZInventoryEntry* Selected = GetInventoryItemAtSlot(SelectedInventorySlot);
    if (!Selected)
    {
        InventoryStatusText = TEXT("当前格没有可丢弃的物品");
        return false;
    }
    const FLZInventoryEntry Discarded = *Selected;
    InventoryEntries.RemoveAll([&Discarded](const FLZInventoryEntry& Entry) { return Entry.StartSlot == Discarded.StartSlot; });
    InventoryStatusText = FString::Printf(TEXT("已丢弃%s ×%d：释放%d格，当前战利品价值%d（不可找回）"),
        LZInventoryItemDisplayName(Discarded.Type), Discarded.Quantity, Discarded.SlotsPerItem, GetLootValue());
    return true;
}
