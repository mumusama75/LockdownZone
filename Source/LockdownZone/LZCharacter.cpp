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
        case ELZInventoryItemType::Axe: return TEXT("消防斧");
        case ELZInventoryItemType::Pistol: return TEXT("格洛克17手枪");
        case ELZInventoryItemType::Flashlight: return TEXT("战术手电筒");
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
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(48.0f);
    GetCharacterMovement()->MaxWalkSpeedCrouched = 180.0f;

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
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ALZCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Released, this, &ALZCharacter::StopCrouch);
    PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &ALZCharacter::StartCrouch);
    PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Released, this, &ALZCharacter::StopCrouch);
    PlayerInputComponent->BindKey(EKeys::C, IE_Pressed, this, &ALZCharacter::ToggleCrouchState);
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
    PlayerInputComponent->BindAction(TEXT("InventoryRotate"), IE_Pressed, this, &ALZCharacter::InventoryRotate);
    PlayerInputComponent->BindAction(TEXT("InventoryInteract"), IE_Pressed, this, &ALZCharacter::InventoryInteract);
    PlayerInputComponent->BindAction(TEXT("InventoryCancel"), IE_Pressed, this, &ALZCharacter::InventoryCancel);
}

void ALZCharacter::StartCrouch()
{
    if (bInventoryOpen || IsRunInactive()) return;
    bCrouchToggled = false;
    Crouch();
    if (GetCharacterMovement()) GetCharacterMovement()->Crouch(false);
}

void ALZCharacter::StopCrouch()
{
    if (bCrouchToggled) return;
    UnCrouch();
    if (GetCharacterMovement()) GetCharacterMovement()->UnCrouch(false);
}

void ALZCharacter::ToggleCrouchState()
{
    if (bInventoryOpen || IsRunInactive()) return;
    if (bIsCrouched)
    {
        bCrouchToggled = false;
        UnCrouch();
        if (GetCharacterMovement()) GetCharacterMovement()->UnCrouch(false);
    }
    else
    {
        bCrouchToggled = true;
        Crouch();
        if (GetCharacterMovement()) GetCharacterMovement()->Crouch(false);
    }
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
    if (bIsCrouched)
    {
        bCrouchToggled = false;
        UnCrouch();
        return;
    }
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
        bool bAlreadyInBag = false;
        for (const FLZInventoryEntry& Entry : InventoryEntries)
        {
            if (Entry.Type == ELZInventoryItemType::Axe) { bAlreadyInBag = true; break; }
        }
        if (!bAlreadyInBag)
        {
            TryStoreItem(ELZInventoryItemType::Axe, 1);
        }
        bHasMeleeWeapon = true;
    }
    else if (Weapon == EPlayerWeapon::Firearm)
    {
        bool bAlreadyInBag = false;
        for (const FLZInventoryEntry& Entry : InventoryEntries)
        {
            if (Entry.Type == ELZInventoryItemType::Pistol) { bAlreadyInBag = true; break; }
        }
        if (!bAlreadyInBag)
        {
            TryStoreItem(ELZInventoryItemType::Pistol, 1);
        }
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
    for (FLZInventoryEntry& Entry : PlannedEntries)
    {
        if (Entry.ItemId <= 0)
        {
            Entry.ItemId = NextItemId++;
        }
    }
    InventoryEntries = MoveTemp(PlannedEntries);
    SyncEquippedGear();
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
    bool bAlreadyInBag = false;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.Type == ELZInventoryItemType::Flashlight) { bAlreadyInBag = true; break; }
    }
    if (!bAlreadyInBag)
    {
        TryStoreItem(ELZInventoryItemType::Flashlight, 1);
    }
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
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        Total += Entry.Width * Entry.Height;
    }
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

void ALZCharacter::GetDefaultItemSize(ELZInventoryItemType Type, int32& OutW, int32& OutH)
{
    switch (Type)
    {
    case ELZInventoryItemType::Axe:
        OutW = 2; OutH = 6;
        break;
    case ELZInventoryItemType::Pistol:
        OutW = 2; OutH = 2;
        break;
    case ELZInventoryItemType::Flashlight:
        OutW = 2; OutH = 1;
        break;
    case ELZInventoryItemType::Medical:
        OutW = 1; OutH = 2;
        break;
    case ELZInventoryItemType::Rare:
        OutW = 2; OutH = 2;
        break;
    case ELZInventoryItemType::Ammo:
    case ELZInventoryItemType::Scrap:
    default:
        OutW = 1; OutH = 1;
        break;
    }
}

TArray<int32> ALZCharacter::GetOverlappingItemIds(int32 TargetX, int32 TargetY, int32 W, int32 H, int32 IgnoreItemId) const
{
    TArray<int32> Overlaps;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.ItemId == IgnoreItemId && IgnoreItemId != 0)
        {
            continue;
        }
        const bool bOverlapX = (TargetX < Entry.PosX + Entry.Width) && (TargetX + W > Entry.PosX);
        const bool bOverlapY = (TargetY < Entry.PosY + Entry.Height) && (TargetY + H > Entry.PosY);
        if (bOverlapX && bOverlapY)
        {
            Overlaps.Add(Entry.ItemId);
        }
    }
    return Overlaps;
}

void ALZCharacter::GetHeldTargetPos(int32 HoverX, int32 HoverY, int32& OutTargetX, int32& OutTargetY) const
{
    const int32 SafeHoverX = FMath::Clamp(HoverX, 0, InventoryGridWidth - 1);
    const int32 SafeHoverY = FMath::Clamp(HoverY, 0, InventoryGridHeight - 1);
    OutTargetX = FMath::Clamp(SafeHoverX - HeldGrabOffsetX, 0, FMath::Max(0, InventoryGridWidth - HeldWidth));
    OutTargetY = FMath::Clamp(SafeHoverY - HeldGrabOffsetY, 0, FMath::Max(0, InventoryGridHeight - HeldHeight));
}

bool ALZCharacter::CanPlaceItem(int32 TargetX, int32 TargetY, int32 W, int32 H, int32 IgnoreItemId) const
{
    if (TargetX < 0 || TargetY < 0 || (TargetX + W) > InventoryGridWidth || (TargetY + H) > InventoryGridHeight)
    {
        return false;
    }
    return GetOverlappingItemIds(TargetX, TargetY, W, H, IgnoreItemId).IsEmpty();
}

bool ALZCharacter::AutoFindPlacement(int32 W, int32 H, int32& OutX, int32& OutY) const
{
    for (int32 Y = 0; Y <= InventoryGridHeight - H; ++Y)
    {
        for (int32 X = 0; X <= InventoryGridWidth - W; ++X)
        {
            if (CanPlaceItem(X, Y, W, H))
            {
                OutX = X;
                OutY = Y;
                return true;
            }
        }
    }
    if (W != H && (H <= InventoryGridWidth && W <= InventoryGridHeight))
    {
        for (int32 Y = 0; Y <= InventoryGridHeight - W; ++Y)
        {
            for (int32 X = 0; X <= InventoryGridWidth - H; ++X)
            {
                if (CanPlaceItem(X, Y, H, W))
                {
                    OutX = X;
                    OutY = Y;
                    return true;
                }
            }
        }
    }
    return false;
}

const FLZInventoryEntry* ALZCharacter::GetInventoryItemAtCell(int32 X, int32 Y) const
{
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.CoversCell(X, Y)) return &Entry;
    }
    return nullptr;
}

FLZInventoryEntry* ALZCharacter::GetInventoryItemAtCellMutable(int32 X, int32 Y)
{
    for (FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.CoversCell(X, Y)) return &Entry;
    }
    return nullptr;
}

const FLZInventoryEntry* ALZCharacter::GetInventoryItemAtSlot(int32 Slot) const
{
    if (Slot < 0 || Slot >= (InventoryGridWidth * InventoryGridHeight)) return nullptr;
    const int32 X = Slot % InventoryGridWidth;
    const int32 Y = Slot / InventoryGridWidth;
    return GetInventoryItemAtCell(X, Y);
}

const FLZInventoryEntry* ALZCharacter::GetInventoryItemById(int32 ItemId) const
{
    if (ItemId == 0) return nullptr;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.ItemId == ItemId) return &Entry;
    }
    return nullptr;
}

FLZInventoryEntry* ALZCharacter::GetInventoryItemByIdMutable(int32 ItemId)
{
    if (ItemId == 0) return nullptr;
    for (FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.ItemId == ItemId) return &Entry;
    }
    return nullptr;
}

bool ALZCharacter::CanCleanSwapWith(int32 TargetX, int32 TargetY, int32 OtherItemId) const
{
    if (HeldItemId == 0 || OtherItemId == 0) return false;
    const FLZInventoryEntry* OtherEntry = GetInventoryItemById(OtherItemId);
    const FLZInventoryEntry* HeldEntry = GetInventoryItemById(HeldItemId);
    if (!OtherEntry || !HeldEntry) return false;

    // 1. Target bounds check for HeldEntry
    if (TargetX < 0 || TargetY < 0 ||
        TargetX + HeldWidth > InventoryGridWidth ||
        TargetY + HeldHeight > InventoryGridHeight)
    {
        return false;
    }

    // 2. Original bounds check for OtherEntry
    if (HeldOriginalX < 0 || HeldOriginalY < 0 ||
        HeldOriginalX + OtherEntry->Width > InventoryGridWidth ||
        HeldOriginalY + OtherEntry->Height > InventoryGridHeight)
    {
        return false;
    }

    // 3. Check if OtherEntry at HeldOriginal position collides with any third item
    for (const FLZInventoryEntry& E : InventoryEntries)
    {
        if (E.ItemId == HeldItemId || E.ItemId == OtherItemId) continue;
        const bool bOverlapX = (HeldOriginalX < E.PosX + E.Width) && (HeldOriginalX + OtherEntry->Width > E.PosX);
        const bool bOverlapY = (HeldOriginalY < E.PosY + E.Height) && (HeldOriginalY + OtherEntry->Height > E.PosY);
        if (bOverlapX && bOverlapY)
        {
            return false;
        }
    }

    // 4. Check if HeldEntry at Target position collides with any third item
    for (const FLZInventoryEntry& E : InventoryEntries)
    {
        if (E.ItemId == HeldItemId || E.ItemId == OtherItemId) continue;
        const bool bOverlapX = (TargetX < E.PosX + E.Width) && (TargetX + HeldWidth > E.PosX);
        const bool bOverlapY = (TargetY < E.PosY + E.Height) && (TargetY + HeldHeight > E.PosY);
        if (bOverlapX && bOverlapY)
        {
            return false;
        }
    }

    return true;
}

void ALZCharacter::SetCursorPos(int32 NewX, int32 NewY)
{
    CursorX = FMath::Clamp(NewX, 0, InventoryGridWidth - 1);
    CursorY = FMath::Clamp(NewY, 0, InventoryGridHeight - 1);
}

void ALZCharacter::SetSelectedInventorySlot(int32 Slot)
{
    const int32 Clamped = FMath::Clamp(Slot, 0, (InventoryGridWidth * InventoryGridHeight) - 1);
    CursorX = Clamped % InventoryGridWidth;
    CursorY = Clamped / InventoryGridWidth;
}

bool ALZCharacter::PlanInventoryStorage(TArray<FLZInventoryEntry>& Entries, ELZInventoryItemType Type, int32 Quantity) const
{
    if (Quantity <= 0) return false;
    int32 DefaultW = 1, DefaultH = 1;
    GetDefaultItemSize(Type, DefaultW, DefaultH);

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

    auto CanPlaceInList = [](const TArray<FLZInventoryEntry>& List, int32 TX, int32 TY, int32 W, int32 H) -> bool
    {
        if (TX < 0 || TY < 0 || (TX + W) > InventoryGridWidth || (TY + H) > InventoryGridHeight) return false;
        for (const FLZInventoryEntry& E : List)
        {
            const bool bOverlapX = (TX < E.PosX + E.Width) && (TX + W > E.PosX);
            const bool bOverlapY = (TY < E.PosY + E.Height) && (TY + H > E.PosY);
            if (bOverlapX && bOverlapY) return false;
        }
        return true;
    };

    auto AutoFindInList = [&](const TArray<FLZInventoryEntry>& List, int32 W, int32 H, int32& OutX, int32& OutY, int32& OutW, int32& OutH) -> bool
    {
        for (int32 Y = 0; Y <= InventoryGridHeight - H; ++Y)
        {
            for (int32 X = 0; X <= InventoryGridWidth - W; ++X)
            {
                if (CanPlaceInList(List, X, Y, W, H))
                {
                    OutX = X; OutY = Y; OutW = W; OutH = H;
                    return true;
                }
            }
        }
        if (W != H && (H <= InventoryGridWidth && W <= InventoryGridHeight))
        {
            for (int32 Y = 0; Y <= InventoryGridHeight - W; ++Y)
            {
                for (int32 X = 0; X <= InventoryGridWidth - H; ++X)
                {
                    if (CanPlaceInList(List, X, Y, H, W))
                    {
                        OutX = X; OutY = Y; OutW = H; OutH = W;
                        return true;
                    }
                }
            }
        }
        return false;
    };

    const int32 StackLimit = (Type == ELZInventoryItemType::Ammo) ? AmmoStackLimit : 1;
    while (Remaining > 0)
    {
        int32 PlaceX = INDEX_NONE, PlaceY = INDEX_NONE, PlaceW = DefaultW, PlaceH = DefaultH;
        if (!AutoFindInList(Entries, DefaultW, DefaultH, PlaceX, PlaceY, PlaceW, PlaceH))
        {
            return false;
        }
        FLZInventoryEntry NewEntry;
        NewEntry.Type = Type;
        NewEntry.Quantity = FMath::Min(Remaining, StackLimit);
        NewEntry.PosX = PlaceX;
        NewEntry.PosY = PlaceY;
        NewEntry.Width = PlaceW;
        NewEntry.Height = PlaceH;
        NewEntry.ItemId = 0;
        NewEntry.SyncLegacyFields();
        Entries.Add(NewEntry);
        Remaining -= NewEntry.Quantity;
    }
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
        InventoryStatusText = FString::Printf(TEXT("背包空间不足：无法容纳%s，请按B整理背包。"),
            LZInventoryItemDisplayName(Type));
        return false;
    }
    for (FLZInventoryEntry& Entry : PlannedEntries)
    {
        if (Entry.ItemId <= 0)
        {
            Entry.ItemId = NextItemId++;
        }
    }
    InventoryEntries = MoveTemp(PlannedEntries);
    SyncEquippedGear();
    InventoryStatusText = FString::Printf(TEXT("已收纳%s ×%d · 背包占用%d/%d格"),
        LZInventoryItemDisplayName(Type), Quantity, GetUsedBagSlots(), GetMaxBagSlots());
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

void ALZCharacter::PickUpItemAtCursor()
{
    PickUpItemAtCell(CursorX, CursorY);
}

void ALZCharacter::PickUpItemAtCell(int32 X, int32 Y)
{
    if (!bInventoryOpen || IsRunInactive()) return;
    if (HeldItemId != 0)
    {
        PlaceHeldItemAtCell(X, Y);
        return;
    }
    const FLZInventoryEntry* Found = GetInventoryItemAtCell(X, Y);
    if (!Found)
    {
        InventoryStatusText = TEXT("该网格为空");
        return;
    }
    HeldItemId = Found->ItemId;
    HeldWidth = Found->Width;
    HeldHeight = Found->Height;
    HeldOriginalX = Found->PosX;
    HeldOriginalY = Found->PosY;
    HeldOriginalWidth = Found->Width;
    HeldOriginalHeight = Found->Height;
    HeldGrabOffsetX = FMath::Clamp(X - Found->PosX, 0, HeldWidth - 1);
    HeldGrabOffsetY = FMath::Clamp(Y - Found->PosY, 0, HeldHeight - 1);
    InventoryStatusText = FString::Printf(TEXT("已拿起%s [%d×%d] · [R]旋转 [点击/松开]放置 [右键]取消 [Del]丢弃"),
        LZInventoryItemDisplayName(Found->Type), HeldWidth, HeldHeight);
}

bool ALZCharacter::PlaceHeldItemAtCursor()
{
    return PlaceHeldItemAtCell(CursorX, CursorY);
}

bool ALZCharacter::PlaceHeldItemAtCell(int32 X, int32 Y)
{
    if (!bInventoryOpen || IsRunInactive() || HeldItemId == 0) return false;

    FLZInventoryEntry* HeldEntry = nullptr;
    for (FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.ItemId == HeldItemId)
        {
            HeldEntry = &Entry;
            break;
        }
    }
    if (!HeldEntry)
    {
        HeldItemId = 0;
        HeldGrabOffsetX = 0;
        HeldGrabOffsetY = 0;
        return false;
    }

    int32 TargetX = 0, TargetY = 0;
    GetHeldTargetPos(X, Y, TargetX, TargetY);

    TArray<int32> OverlapIds = GetOverlappingItemIds(TargetX, TargetY, HeldWidth, HeldHeight, HeldItemId);

    if (OverlapIds.IsEmpty())
    {
        HeldEntry->PosX = TargetX;
        HeldEntry->PosY = TargetY;
        HeldEntry->Width = HeldWidth;
        HeldEntry->Height = HeldHeight;
        HeldEntry->SyncLegacyFields();

        InventoryStatusText = FString::Printf(TEXT("已将%s放置于 (%c%d)"),
            LZInventoryItemDisplayName(HeldEntry->Type), 'A' + TargetY, TargetX + 1);

        HeldItemId = 0;
        HeldGrabOffsetX = 0;
        HeldGrabOffsetY = 0;
        SyncEquippedGear();
        return true;
    }

    if (OverlapIds.Num() == 1)
    {
        FLZInventoryEntry* OtherEntry = nullptr;
        for (FLZInventoryEntry& Entry : InventoryEntries)
        {
            if (Entry.ItemId == OverlapIds[0])
            {
                OtherEntry = &Entry;
                break;
            }
        }

        if (OtherEntry)
        {
            // Case 1A: Ammo Stacking
            if (HeldEntry->Type == ELZInventoryItemType::Ammo && OtherEntry->Type == ELZInventoryItemType::Ammo)
            {
                const int32 SpaceAvailable = AmmoStackLimit - OtherEntry->Quantity;
                if (SpaceAvailable > 0)
                {
                    const int32 Transfer = FMath::Min(SpaceAvailable, HeldEntry->Quantity);
                    OtherEntry->Quantity += Transfer;
                    OtherEntry->SyncLegacyFields();
                    HeldEntry->Quantity -= Transfer;
                    HeldEntry->SyncLegacyFields();

                    if (HeldEntry->Quantity <= 0)
                    {
                        const int32 DeadId = HeldEntry->ItemId;
                        InventoryEntries.RemoveAll([DeadId](const FLZInventoryEntry& E) { return E.ItemId == DeadId; });
                        HeldItemId = 0;
                        HeldGrabOffsetX = 0;
                        HeldGrabOffsetY = 0;
                        InventoryStatusText = FString::Printf(TEXT("已合并弹药：当前堆叠 %d 发"), OtherEntry->Quantity);
                    }
                    else
                    {
                        InventoryStatusText = FString::Printf(TEXT("已装入 %d 发弹药，手中剩余 %d 发"), Transfer, HeldEntry->Quantity);
                    }
                    SyncEquippedGear();
                    return true;
                }
            }

            // Case 1B: Clean Swap
            if (CanCleanSwapWith(TargetX, TargetY, OtherEntry->ItemId))
            {
                OtherEntry->PosX = HeldOriginalX;
                OtherEntry->PosY = HeldOriginalY;
                OtherEntry->SyncLegacyFields();

                HeldEntry->PosX = TargetX;
                HeldEntry->PosY = TargetY;
                HeldEntry->Width = HeldWidth;
                HeldEntry->Height = HeldHeight;
                HeldEntry->SyncLegacyFields();

                InventoryStatusText = FString::Printf(TEXT("已交换 %s 与 %s 的位置"),
                    LZInventoryItemDisplayName(HeldEntry->Type), LZInventoryItemDisplayName(OtherEntry->Type));

                HeldItemId = 0;
                HeldGrabOffsetX = 0;
                HeldGrabOffsetY = 0;
                SyncEquippedGear();
                return true;
            }
        }
    }

    InventoryStatusText = TEXT("无法在此放置：位置受阻");
    return false;
}

void ALZCharacter::RotateInventoryItem()
{
    if (!bInventoryOpen || IsRunInactive()) return;

    if (HeldItemId != 0)
    {
        Swap(HeldWidth, HeldHeight);
        Swap(HeldGrabOffsetX, HeldGrabOffsetY);
        HeldGrabOffsetX = FMath::Clamp(HeldGrabOffsetX, 0, HeldWidth - 1);
        HeldGrabOffsetY = FMath::Clamp(HeldGrabOffsetY, 0, HeldHeight - 1);
        InventoryStatusText = FString::Printf(TEXT("已旋转装备方向: %d×%d"), HeldWidth, HeldHeight);
        return;
    }

    FLZInventoryEntry* Hovered = GetInventoryItemAtCellMutable(CursorX, CursorY);
    if (!Hovered)
    {
        InventoryStatusText = TEXT("当前选格无物品可旋转");
        return;
    }

    const int32 NewW = Hovered->Height;
    const int32 NewH = Hovered->Width;

    if (CanPlaceItem(Hovered->PosX, Hovered->PosY, NewW, NewH, Hovered->ItemId))
    {
        Hovered->Width = NewW;
        Hovered->Height = NewH;
        Hovered->SyncLegacyFields();
        InventoryStatusText = FString::Printf(TEXT("已原地旋转%s: %d×%d"),
            LZInventoryItemDisplayName(Hovered->Type), Hovered->Width, Hovered->Height);
        SyncEquippedGear();
    }
    else
    {
        InventoryStatusText = FString::Printf(TEXT("原位空间受阻无法旋转：请点击拿起%s后再旋转"),
            LZInventoryItemDisplayName(Hovered->Type));
    }
}

void ALZCharacter::RotateHeldItem()
{
    RotateInventoryItem();
}

void ALZCharacter::CancelHeldItem()
{
    if (HeldItemId == 0) return;
    for (FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.ItemId == HeldItemId)
        {
            Entry.PosX = HeldOriginalX;
            Entry.PosY = HeldOriginalY;
            Entry.Width = HeldOriginalWidth;
            Entry.Height = HeldOriginalHeight;
            Entry.SyncLegacyFields();
            break;
        }
    }
    HeldItemId = 0;
    HeldGrabOffsetX = 0;
    HeldGrabOffsetY = 0;
    SyncEquippedGear();
    InventoryStatusText = TEXT("已取消移动，物品放回原位");
}

bool ALZCharacter::DiscardItemById(int32 ItemId)
{
    if (!bInventoryOpen || IsRunInactive() || ItemId == 0) return false;
    int32 FoundIndex = INDEX_NONE;
    for (int32 Idx = 0; Idx < InventoryEntries.Num(); ++Idx)
    {
        if (InventoryEntries[Idx].ItemId == ItemId)
        {
            FoundIndex = Idx;
            break;
        }
    }
    if (FoundIndex == INDEX_NONE) return false;
    const FLZInventoryEntry Discarded = InventoryEntries[FoundIndex];
    InventoryEntries.RemoveAt(FoundIndex);
    if (HeldItemId == ItemId)
    {
        HeldItemId = 0;
        HeldGrabOffsetX = 0;
        HeldGrabOffsetY = 0;
    }
    SyncEquippedGear();
    InventoryStatusText = FString::Printf(TEXT("已丢弃%s ×%d：释放%d格空间"),
        LZInventoryItemDisplayName(Discarded.Type), Discarded.Quantity, Discarded.Width * Discarded.Height);
    return true;
}

bool ALZCharacter::DiscardItemAtCell(int32 X, int32 Y)
{
    const FLZInventoryEntry* Item = GetInventoryItemAtCell(X, Y);
    if (!Item)
    {
        InventoryStatusText = TEXT("当前网格没有物品可丢弃");
        return false;
    }
    return DiscardItemById(Item->ItemId);
}

bool ALZCharacter::DiscardSelectedInventoryItem()
{
    if (!bInventoryOpen || IsRunInactive()) return false;
    if (HeldItemId != 0) return DiscardItemById(HeldItemId);
    return DiscardItemAtCell(CursorX, CursorY);
}

bool ALZCharacter::UseSelectedInventoryItem()
{
    if (!bInventoryOpen || IsRunInactive()) return false;
    const FLZInventoryEntry* Selected = (HeldItemId != 0) ? GetInventoryItemById(HeldItemId) : GetInventoryItemAtCell(CursorX, CursorY);
    if (!Selected)
    {
        InventoryStatusText = TEXT("当前格没有物品");
        return false;
    }
    if (Selected->Type != ELZInventoryItemType::Medical)
    {
        InventoryStatusText = Selected->Type == ELZInventoryItemType::Ammo
            ? TEXT("关闭背包后按R：从备用弹药中装填弹匣")
            : TEXT("该装备无需使用，在背包中即生效");
        return false;
    }
    if (Health >= MaxHealth)
    {
        InventoryStatusText = TEXT("生命值已满：医疗包未消耗");
        return false;
    }
    const float RestoredHealth = FMath::Min(35.0f, MaxHealth - Health);
    const int32 ItemIdToUse = Selected->ItemId;
    Health += RestoredHealth;
    DiscardItemById(ItemIdToUse);
    InventoryStatusText = FString::Printf(TEXT("已使用医疗包：恢复%.0f生命值"), RestoredHealth);
    return true;
}

void ALZCharacter::SyncEquippedGear()
{
    bHasMeleeWeapon = false;
    bHasFirearm = false;
    bHasFlashlight = false;
    for (const FLZInventoryEntry& Entry : InventoryEntries)
    {
        if (Entry.Type == ELZInventoryItemType::Axe) bHasMeleeWeapon = true;
        else if (Entry.Type == ELZInventoryItemType::Pistol) bHasFirearm = true;
        else if (Entry.Type == ELZInventoryItemType::Flashlight) bHasFlashlight = true;
    }
    if (!bHasMeleeWeapon && SelectedWeapon == EPlayerWeapon::Melee)
    {
        SelectedWeapon = bHasFirearm ? EPlayerWeapon::Firearm : EPlayerWeapon::None;
        UpdateWeaponVisibility();
    }
    if (!bHasFirearm && SelectedWeapon == EPlayerWeapon::Firearm)
    {
        SelectedWeapon = bHasMeleeWeapon ? EPlayerWeapon::Melee : EPlayerWeapon::None;
        UpdateWeaponVisibility();
    }
    if (!bHasFlashlight && bFlashlightOn)
    {
        SetFlashlightEnabled(false);
    }
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
        if (InventoryStatusText.IsEmpty()) InventoryStatusText = TEXT("鼠标点击或方向键选格，R旋转，E/空格拿起放置");
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            InventoryInputController = PC;
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);
            bInventoryInputLockApplied = true;
            PC->bShowMouseCursor = true;
        }
    }
    else
    {
        if (HeldItemId != 0)
        {
            CancelHeldItem();
        }
        if (bInventoryInputLockApplied)
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
    }
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
    CursorX = FMath::Clamp(CursorX + ColumnDelta, 0, InventoryGridWidth - 1);
    CursorY = FMath::Clamp(CursorY + RowDelta, 0, InventoryGridHeight - 1);
}

void ALZCharacter::InventoryLeft() { MoveInventorySelection(-1, 0); }
void ALZCharacter::InventoryRight() { MoveInventorySelection(1, 0); }
void ALZCharacter::InventoryUp() { MoveInventorySelection(0, -1); }
void ALZCharacter::InventoryDown() { MoveInventorySelection(0, 1); }
void ALZCharacter::InventoryRotate() { RotateHeldItem(); }
void ALZCharacter::InventoryInteract()
{
    if (HeldItemId != 0) PlaceHeldItemAtCursor();
    else PickUpItemAtCursor();
}
void ALZCharacter::InventoryCancel()
{
    if (HeldItemId != 0) CancelHeldItem();
    else ToggleInventory();
}

void ALZCharacter::InventoryDiscard()
{
    DiscardSelectedInventoryItem();
}

