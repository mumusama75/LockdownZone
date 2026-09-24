#include "LZSliceQA.h"
#include "LZMotionProfile.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/StaticMeshActor.h"
#include "LZMotionAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

#include "LZBreakableGlass.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "LZExtraction.h"
#include "LZFlashlightPickup.h"
#include "LZGameMode.h"
#include "LZInteractable.h"
#include "LZInventoryTypes.h"
#include "LZLoot.h"
#include "LZPowerInteractable.h"
#include "LZPuzzleTerminal.h"
#include "LZWeaponPickup.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/PointLight.h"
#include "Components/LightComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
namespace
{
    // OpenLevel replaces every actor, so carry only the QA report across the single intentional restart.
    // No gameplay data is carried over and no restart is initiated except the simulated F5 input.
    struct FLZQARestartContinuation
    {
        bool bRestartRequested = false;
        bool bCompleted = false;
        int32 Assertions = 0;
        int32 Failures = 0;
        double Elapsed = 0.0;
        TArray<FString> ReportLines;
        TArray<FString> ScreenshotPaths;
    };
    FLZQARestartContinuation GSliceQARestart;
}
#endif

ALZSliceQA::ALZSliceQA()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ALZSliceQA::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    if (!FParse::Param(FCommandLine::Get(), TEXT("LZSliceQA")))
    {
        SetActorTickEnabled(false);
        return;
    }
    Player = Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>();
    StartedAt = GetWorld()->GetTimeSeconds();
    bRunning = true;
    if(FParse::Param(FCommandLine::Get(),TEXT("LZMotionOnly")))
    {
        IsolateEnemies();
        Player->AcquireWeapon(EPlayerWeapon::Melee);
        Player->AcquireWeapon(EPlayerWeapon::Firearm);
        Advance(EStep::MotionPrepare,2.0f);
        return;
    }
    if (GSliceQARestart.bCompleted)
    {
        bRunning = false;
        SetActorTickEnabled(false);
        return;
    }
    if (GSliceQARestart.bRestartRequested)
    {
        Assertions = GSliceQARestart.Assertions;
        Failures = GSliceQARestart.Failures;
        ReportLines = GSliceQARestart.ReportLines;
        ScreenshotPaths = GSliceQARestart.ScreenshotPaths;
        ElapsedBeforeRestart = GSliceQARestart.Elapsed;
        Check(true, TEXT("F5 input opened a new world and created a fresh QA actor"));
        Advance(EStep::VerifyRestart, 1.25f);
        return;
    }
    if (!Check(Player.IsValid() && GameMode.IsValid(), TEXT("runtime player and game mode exist")))
    {
        Advance(EStep::Finish);
        return;
    }
    for (TActorIterator<ALZWeaponPickup> It(GetWorld()); It; ++It)
    {
        if (It->GetInteractionPrompt(Player.Get()).Contains(TEXT("格洛克"))) PistolPickup = *It;
        else MeleePickup = *It;
    }
    for (TActorIterator<ALZLoot> It(GetWorld()); It; ++It)
    {
        const ELZInventoryItemType Type = It->GetInventoryType();
        if (Type == ELZInventoryItemType::Ammo) AmmoPickups.Add(*It);
        if (Type == ELZInventoryItemType::Medical) MedicalPickups.Add(*It);
        if (Type == ELZInventoryItemType::Scrap) ScrapPickup = *It;
        if (Type == ELZInventoryItemType::Rare)
        {
            if (!RarePickup.IsValid() || It->GetActorLocation().Y < RarePickup->GetActorLocation().Y)
            {
                OtherRarePickup = RarePickup;
                RarePickup = *It;
            }
            else OtherRarePickup = *It;
        }
    }
    AmmoPickups.Sort([](const TWeakObjectPtr<ALZLoot>& A, const TWeakObjectPtr<ALZLoot>& B)
    {
        return A->GetActorLocation().X < B->GetActorLocation().X;
    });
    for (TActorIterator<ALZPowerInteractable> It(GetWorld()); It; ++It)
    {
        if (It->GetInteractionPrompt(Player.Get()).Contains(TEXT("拾取15A"))) Fuse = *It;
        else Breaker = *It;
    }
    for (TActorIterator<ALZBreakableGlass> It(GetWorld()); It; ++It) Glass = *It;
    for (TActorIterator<ALZFlashlightPickup> It(GetWorld()); It; ++It) FlashlightPickup = *It;
    for (TActorIterator<ALZExtraction> It(GetWorld()); It; ++It) Extraction = *It;
    for (TActorIterator<ALZEnemy> It(GetWorld()); It; ++It) CombatTargets.Add(*It);
    CombatTargets.Sort([](const TWeakObjectPtr<ALZEnemy>& A, const TWeakObjectPtr<ALZEnemy>& B)
    {
        return A->GetActorLocation().X < B->GetActorLocation().X;
    });
    const bool bSceneReady = MeleePickup.IsValid() && PistolPickup.IsValid() && AmmoPickups.Num() >= 2 &&
        Fuse.IsValid() && Breaker.IsValid() && Glass.IsValid() && Extraction.IsValid() &&
        FlashlightPickup.IsValid() && CombatTargets.Num() >= 4 && RarePickup.IsValid() &&
        OtherRarePickup.IsValid() && ScrapPickup.IsValid() && MedicalPickups.Num() >= 2;
    Check(bSceneReady, TEXT("office slice contains weapons, flashlight, ammo, medicine, scrap, two rare parts and all objective actors"));
    if (!bSceneReady)
    {
        Advance(EStep::Finish);
        return;
    }
    Check(!Player->HasMeleeWeapon() && !Player->HasFirearm(), TEXT("opening starts unarmed"));
    Check(!Player->HasFlashlight(), TEXT("opening starts without a flashlight"));
    CheckFlashlightState(false, TEXT("unowned flashlight starts off"));
    {
        TArray<UStaticMeshComponent*> Meshes;
        Player->GetComponents<UStaticMeshComponent>(Meshes);
        for (UStaticMeshComponent* Mesh : Meshes)
            if (Mesh->GetFName() == FName(TEXT("WeaponBody"))) WeaponVisual = Mesh;
        Check(WeaponVisual.IsValid(), TEXT("first-person weapon component exists for recoil presentation checks"));
    }
    Check(RarePickup.IsValid(), TEXT("optional 500-value server parts exist for loot regression"));
    Check(Player->GetUsedBagSlots() == 0 && Player->GetLootValue() == 0 &&
        Player->GetInventoryEntries().IsEmpty() && !Player->IsInventoryOpen(), TEXT("opening inventory is empty and closed"));
    Check(Fuse->IsHidden() && !Fuse->GetActorEnableCollision(), TEXT("fuse is unavailable before blackout"));
    {
        int32 PuzzleTermCount = 0;
        for (TActorIterator<ALZPuzzleTerminal> It(GetWorld()); It; ++It) ++PuzzleTermCount;
        Check(PuzzleTermCount == 4, TEXT("four environmental puzzle terminals exist for in-world guidance"));
        Check(!GameMode->IsPuzzleComplete() && GameMode->GetPuzzleStep() == 0, TEXT("auxiliary security puzzle starts in armed offline state"));
    }
    SaveEnemySnapshot();
    Advance(EStep::OpeningIdle, 3.25f);
#else
    SetActorTickEnabled(false);
#endif
}

void ALZSliceQA::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    RestoreEnemies();
#endif
    Super::EndPlay(EndPlayReason);
}

void ALZSliceQA::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    if (!bRunning) return;
    if (bIsolatingEnemies) IsolateEnemies(); // Includes the two enemies spawned by the blackout.
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - StartedAt > 120.0)
    {
        Check(false, TEXT("scenario exceeded 120 seconds"));
        CompleteRun();
        return;
    }
    if (Now < NextStepTime || FScreenshotRequest::IsScreenshotRequested()) return;
    if (Step != EStep::Finish && (!Player.IsValid() || !GameMode.IsValid()))
    {
        Check(false, TEXT("runtime player/game mode remained valid"));
        Advance(EStep::Finish);
        return;
    }
    switch (Step)
    {
    case EStep::OpeningIdle:
        CheckEnemySnapshot(TEXT("unarmed idle >=3 seconds"));
        CheckOpeningPresentation();
        if (PistolPickup.IsValid())
        {
            TArray<UStaticMeshComponent*> PickupMeshes;
            PistolPickup->GetComponents<UStaticMeshComponent>(PickupMeshes);
            UStaticMeshComponent* PistolMesh = nullptr;
            for (UStaticMeshComponent* Mesh : PickupMeshes)
            {
                if (Mesh->GetFName() == FName(TEXT("Mesh"))) PistolMesh = Mesh;
            }
            const float LongestSide = PistolMesh ? PistolMesh->Bounds.BoxExtent.GetMax() * 2.0f : 0.0f;
            Check(PistolMesh && FMath::IsNearlyEqual(LongestSide, 20.4f, 0.5f),
                FString::Printf(TEXT("pistol mesh longest world bound is %.2fcm (expected 20.4 +/-0.5cm)"), LongestSide));
        }
        {
            const float PierYPositions[] = { -400.0f, 652.0f };
            for (float Y : PierYPositions)
            {
                FHitResult Hit;
                FCollisionQueryParams Params(SCENE_QUERY_STAT(SliceQAOpeningSeal), false, Player.Get());
                const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit,
                    FVector(-2700.0f, Y, 150.0f), FVector(-2350.0f, Y, 150.0f), ECC_Visibility, Params);
                Check(bBlocked, FString::Printf(TEXT("opening window pier at Y=%.0f blocks a bypass sightline"), Y));
            }
        }
        // Verify Ctrl crouch & sneak mechanics
        Player->StartCrouch();
        Check(Player->bIsCrouched && Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() <= 50.0f &&
            Player->GetCharacterMovement()->MaxWalkSpeedCrouched <= 200.0f,
            TEXT("Ctrl crouch lowers capsule half-height to crouched envelope and enables 180cm/s sneak speed"));
        Player->StopCrouch();
        Check(!Player->bIsCrouched, TEXT("releasing crouch restores standing posture"));
        Capture(TEXT("01_Opening"));
        Advance(EStep::InventoryEmptyOpen, 0.2f);
        break;
    case EStep::InventoryEmptyOpen:
        PressInputKey(EKeys::B, TEXT("B opens the initial empty backpack"));
        Advance(EStep::InventoryEmptyView, 0.2f);
        break;
    case EStep::InventoryEmptyView:
        Check(Player->IsInventoryOpen() && Player->GetMaxBagSlots() == 36 && Player->GetInventoryEntries().IsEmpty(),
            TEXT("real B input opens a 36-slot empty 6x6 spatial inventory"));
        Capture(TEXT("01c_InventoryEmpty"));
        Advance(EStep::InventoryEmptyRight, 0.2f);
        break;
    case EStep::InventoryEmptyRight:
        Player->SetSelectedInventorySlot(0);
        PressInputKey(EKeys::Right, TEXT("right arrow selects the next backpack slot"));
        Advance(EStep::InventoryEmptyClose, 0.2f);
        break;
    case EStep::InventoryEmptyClose:
        Check(Player->GetSelectedInventorySlot() == 1, TEXT("right-arrow input selects empty slot two in the grid"));
        PressInputKey(EKeys::B, TEXT("B closes the initial backpack"));
        Advance(EStep::UnownedFlashlightInput, 0.2f);
        break;
    case EStep::UnownedFlashlightInput:
        Check(!Player->IsInventoryOpen(), TEXT("second B input closes the backpack before world interaction"));
        PressInputKey(EKeys::F, TEXT("F before flashlight pickup"));
        Advance(EStep::VerifyUnownedFlashlight, 0.2f);
        break;
    case EStep::VerifyUnownedFlashlight:
        Check(!Player->HasFlashlight(), TEXT("F does not grant an unowned flashlight"));
        CheckFlashlightState(false, TEXT("F before pickup cannot emit flashlight light"));
        Approach(FlashlightPickup.Get(), TEXT("flashlight pickup"));
        Advance(EStep::FlashlightPickupView, 0.25f);
        break;
    case EStep::FlashlightPickupView:
        Capture(TEXT("01b_FlashlightPickup"));
        Advance(EStep::InteractFlashlight, 0.2f);
        break;
    case EStep::InteractFlashlight:
        if (Check(FlashlightPickup.IsValid() && Player->FindInteractable() == FlashlightPickup.Get(),
            TEXT("actual E trace hits flashlight pickup")))
        {
            PressInputKey(EKeys::E, TEXT("E picks up the aimed flashlight"));
        }
        Advance(EStep::PickupFlashlight, 0.25f);
        break;
    case EStep::PickupFlashlight:
        Check(Player->HasFlashlight() && !FlashlightPickup.IsValid(), TEXT("real E input acquires and removes flashlight pickup"));
        CheckFlashlightState(true, TEXT("acquired flashlight switches on automatically"));
        Check(Player->GetUsedBagSlots() == 2 && Player->GetLootValue() == 0,
            TEXT("flashlight equipment occupies 2x1 cells (2 slots) and adds no loot value"));
        Check(!Player->HasMeleeWeapon() && !Player->HasFirearm() && Player->GetSelectedWeapon() == EPlayerWeapon::None,
            TEXT("flashlight alone leaves the player unarmed"));
        Check(!GameMode->IsObjectiveComplete() && !GameMode->IsOfficePowerRestored() && !GameMode->HasFuse(),
            TEXT("flashlight pickup does not unlock the powered exit or supply the fuse"));
        {
            const USpotLightComponent* Light = Player->FindComponentByClass<USpotLightComponent>();
            const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
            Check(Light && Camera && Light->GetAttachParent() == Camera,
                TEXT("flashlight spotlight follows the first-person camera"));
            Check(Light && Light->InnerConeAngle > 0.0f && Light->InnerConeAngle < Light->OuterConeAngle &&
                Light->OuterConeAngle <= 40.0f && Light->AttenuationRadius >= 1200.0f && Light->CastShadows,
                TEXT("flashlight has a focused inner/outer cone, useful range and shadow casting"));
        }
        SaveEnemySnapshot();
        Advance(EStep::FlashlightUnarmedIdle, 3.25f);
        break;
    case EStep::FlashlightUnarmedIdle:
        CheckEnemySnapshot(TEXT("flashlight-only unarmed idle >=3 seconds with normal enemy ticks"));
        PressInputKey(EKeys::F, TEXT("F switches acquired flashlight off"));
        Advance(EStep::FlashlightToggleOff, 0.2f);
        break;
    case EStep::FlashlightToggleOff:
        CheckFlashlightState(false, TEXT("real F input switches flashlight off"));
        PressInputKey(EKeys::F, TEXT("F switches acquired flashlight back on"));
        Advance(EStep::FlashlightToggleOn, 0.2f);
        break;
    case EStep::FlashlightToggleOn:
        CheckFlashlightState(true, TEXT("second real F input switches flashlight on"));
        Advance(EStep::PickupMelee, 0.2f);
        break;
    case EStep::PickupMelee:
        if (Approach(MeleePickup.Get(), TEXT("axe pickup"))) InteractWithAimed(MeleePickup.Get(), TEXT("axe pickup"));
        Check(Player->HasMeleeWeapon() && !Player->HasFirearm() && Player->GetUsedBagSlots() == 14,
            TEXT("only axe acquired through Interact, occupying 2x6 cells (total 14 slots)"));
        SaveEnemySnapshot();
        Advance(EStep::MeleeIdle, 3.25f);
        break;
    case EStep::MeleeIdle:
        CheckEnemySnapshot(TEXT("one weapon idle >=3 seconds"));
        Capture(TEXT("02_Axe"));
        Advance(EStep::PickupPistol);
        break;
    case EStep::PickupPistol:
        if (Approach(PistolPickup.Get(), TEXT("pistol pickup"))) InteractWithAimed(PistolPickup.Get(), TEXT("pistol pickup"));
        Check(Player->HasMeleeWeapon() && Player->HasFirearm() && Player->GetUsedBagSlots() == 18,
            TEXT("both weapons acquired through Interact, pistol occupying 2x2 cells (total 18 slots)"));
        Check(Player->GetAmmoInMagazine() == 3 && Player->GetReserveAmmo() == 0, TEXT("pistol starts with 3 rounds and zero reserve"));
        Player->QAFire();
        Check(Player->GetAmmoInMagazine() == 3 && FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.01f),
            TEXT("firing during pickup inspection consumes no ammunition and creates no recoil"));
        // Isolate scripted interaction/camera phases, without modifying health, damage, ammo or collision.
        // Opening immobility above and post-extraction immobility below run with normal enemy ticks.
        IsolateEnemies();
        UE_LOG(LogTemp, Display, TEXT("LZ_QA INFO temporary enemy tick isolation for deterministic interaction/camera phases"));
        Advance(EStep::PistolInspect, 1.65f);
        break;
    case EStep::PistolInspect:
        Capture(TEXT("03_Pistol"));
        Advance(EStep::PickupAmmo);
        break;
    case EStep::PickupAmmo:
        for (int32 Index = 0; Index < 2; ++Index)
        {
            ALZLoot* Ammo = AmmoPickups[Index].Get();
            if (Approach(Ammo, FString::Printf(TEXT("ammo box %d"), Index + 1)))
                InteractWithAimed(Ammo, FString::Printf(TEXT("ammo box %d"), Index + 1));
        }
        Check(Player->GetAmmoInMagazine() == 3 && Player->GetReserveAmmo() == 24 &&
            Player->GetUsedBagSlots() == 19 && CountInventoryItems(ELZInventoryItemType::Ammo) == 24,
            TEXT("two ammo boxes stack 24 reserve rounds in one real 1x1 backpack slot (total 19 slots)"));
        Player->QAReload();
        Check(Player->GetAmmoInMagazine() == 3, TEXT("reload does not transfer ammo instantly"));
        Player->QAFire();
        Check(Player->GetAmmoInMagazine() == 3 && FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.01f),
            TEXT("firing during reload consumes no ammunition and creates no recoil"));
        Advance(EStep::ReloadFinished, 1.65f);
        break;
    case EStep::ReloadFinished:
        Check(Player->GetAmmoInMagazine() == 17 && Player->GetReserveAmmo() == 10 && Player->GetUsedBagSlots() == 19,
            TEXT("normal reload finishes at 17/10 and retains the partial ammunition stack (total 19 slots)"));
        Capture(TEXT("04_Reloaded"));
        Advance(EStep::AimGlass);
        break;
    case EStep::AimGlass:
        if (Glass.IsValid()) PlaceAndAim(FVector(-2740.0f, 120.0f, 90.0f), Glass->GetActorLocation());
        Advance(EStep::FireGlass, 0.35f);
        break;
    case EStep::FireGlass:
        if (WeaponVisual.IsValid()) HipWeaponRestPose = WeaponVisual->GetRelativeTransform();
        HipViewRestRotation = Player->GetControlRotation();
        Player->QAFire();
        Check(Player->GetAmmoInMagazine() == 16 && Player->GetReserveAmmo() == 10, TEXT("normal fire consumes exactly one round"));
        HipShotRecoil = Player->GetRecoilPitch();
        Check(HipShotRecoil > 0.1f &&
            FMath::FindDeltaAngleDegrees(HipViewRestRotation.Pitch, Player->GetControlRotation().Pitch) > 0.1f &&
            WeaponVisual.IsValid() &&
            !WeaponVisual->GetRelativeTransform().Equals(HipWeaponRestPose, 0.01f),
            TEXT("normal hip fire immediately raises the view and displaces the weapon"));
        Advance(EStep::RecoilView, 0.035f);
        break;
    case EStep::RecoilView:
        Capture(TEXT("04b_HipFireRecoil"));
        Advance(EStep::GlassResult, 0.8f);
        break;
    case EStep::GlassResult:
        Check(!Glass.IsValid(), TEXT("normal weapon trace shatters glass in one shot"));
        Check(FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.03f) &&
            Player->GetControlRotation().Equals(HipViewRestRotation, 0.08f) && WeaponVisual.IsValid() &&
            WeaponVisual->GetRelativeTransform().Equals(HipWeaponRestPose, 0.1f),
            TEXT("recoil recovers to the hip-fire resting weapon pose without lasting camera pitch"));
        Capture(TEXT("05_ShatteredGlass"));
        Advance(EStep::AimEnemy);
        break;
    case EStep::AimEnemy:
        if (KillIndex == 0) Player->QAStartAim();
        ApproachEnemy(CombatTargets.IsValidIndex(KillIndex) ? CombatTargets[KillIndex].Get() : nullptr);
        Advance(EStep::DamageEnemy, 0.35f);
        break;
    case EStep::DamageEnemy:
        if (CombatTargets.IsValidIndex(KillIndex) && CombatTargets[KillIndex].IsValid())
        {
            ALZEnemy* Enemy = CombatTargets[KillIndex].Get();
            PlaceAndAim(Player->GetActorLocation(), Enemy->GetActorLocation() + FVector(0,0,35));
            const float Before = Enemy->GetEnemyHealth();
            Player->QAFire();
            if (KillIndex == 0)
            {
                Check(Player->GetRecoilPitch() > 0.05f && Player->GetRecoilPitch() < HipShotRecoil,
                    TEXT("ADS pistol fire has positive recoil lighter than hip fire"));
            }
            Check(IsValid(Enemy) && FMath::IsNearlyEqual(Enemy->GetEnemyHealth(), Before - 34.0f),
                FString::Printf(TEXT("normal pistol trace deals 34 damage to infected %d"), KillIndex + 1));
            Check(Player->GetAmmoInMagazine() == 15 - KillIndex * 2,
                FString::Printf(TEXT("first shot at infected %d consumes one round"), KillIndex + 1));
            if (KillIndex == 0) Capture(TEXT("05b_InfectedHit"));
        }
        else Check(false, FString::Printf(TEXT("infected %d exists before damage"), KillIndex + 1));
        Advance(EStep::ReaimEnemy, 0.7f);
        break;
    case EStep::ReaimEnemy:
        Check(FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.03f),
            FString::Printf(TEXT("infected %d follow-up shot waits for recoil recovery"), KillIndex + 1));
        if (KillIndex == 0) Player->QAStopAim();
        // Recheck the real sightline after view recoil/ADS instead of assuming the old aim is still valid.
        ApproachEnemy(CombatTargets.IsValidIndex(KillIndex) ? CombatTargets[KillIndex].Get() : nullptr);
        Advance(EStep::KillEnemy, 0.2f);
        break;
    case EStep::KillEnemy:
        if (CombatTargets.IsValidIndex(KillIndex) && CombatTargets[KillIndex].IsValid())
        {
            if (CombatTargets[KillIndex].IsValid())
            PlaceAndAim(Player->GetActorLocation(), CombatTargets[KillIndex]->GetActorLocation()+FVector(0,0,35));
        Player->QAFire();
        }
        Check(Player->GetAmmoInMagazine() == 14 - KillIndex * 2,
            FString::Printf(TEXT("second shot at infected %d consumes one round"), KillIndex + 1));
        Check(!CombatTargets[KillIndex].IsValid(), FString::Printf(TEXT("infected %d dies after second normal pistol trace"), KillIndex + 1));
        ++KillIndex;
        Check(GameMode->GetEnemiesKilled() == KillIndex, FString::Printf(TEXT("kill counter is %d"), KillIndex));
        Check(GameMode->IsOfficeBlackout() == (KillIndex >= 4), FString::Printf(TEXT("blackout threshold after kill %d"), KillIndex));
        Advance(KillIndex < 4 ? EStep::AimEnemy : EStep::Blackout, 0.7f);
        break;
    case EStep::Blackout:
        Check(Fuse.IsValid() && !Fuse->IsHidden() && Fuse->GetActorEnableCollision(), TEXT("blackout reveals a collidable fuse"));
        Extraction->Interact(Player.Get()); // State gate is checked separately from the still-closed physical door.
        Check(!GameMode->IsRunOver() && !GameMode->WasExtractionSuccessful(), TEXT("extraction refused before restoring power"));
        Breaker->Interact(Player.Get());
        Check(!GameMode->IsOfficePowerRestored(), TEXT("breaker refuses repair without fuse"));
        PlaceAndAim(FVector(-500.0f, 0.0f, 90.0f), FVector(1700.0f, 0.0f, 180.0f));
        Advance(EStep::ServerView, 1.0f);
        break;
    case EStep::ServerView:
        Capture(TEXT("06_Blackout"));
        Advance(EStep::ServerWideSetup);
        break;
    case EStep::ServerWideSetup:
        PlaceAndAim(FVector(1550.0f, -1080.0f, 90.0f), FVector(2100.0f, -1710.0f, 130.0f));
        PressInputKey(EKeys::F, TEXT("F disables flashlight for the dark-room comparison"));
        Advance(EStep::FlashlightDarkOff, 0.7f);
        break;
    case EStep::FlashlightDarkOff:
        CheckFlashlightState(false, TEXT("dark-room comparison begins with flashlight off"));
        if (const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>())
        {
            FlashlightComparisonLocation = Camera->GetComponentLocation();
            FlashlightComparisonRotation = Camera->GetComponentRotation();
        }
        Capture(TEXT("06c_FlashlightOff"));
        Advance(EStep::FlashlightDarkEnable, 0.2f);
        break;
    case EStep::FlashlightDarkEnable:
        // Capture() is deferred until the frame ends: toggle only after the off image has completed.
        PressInputKey(EKeys::F, TEXT("F enables flashlight at the unchanged dark-room camera"));
        Advance(EStep::FlashlightDarkOn, 0.7f);
        break;
    case EStep::FlashlightDarkOn:
        CheckFlashlightState(true, TEXT("dark-room comparison ends with flashlight on"));
        if (const UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>())
        {
            Check(Camera->GetComponentLocation().Equals(FlashlightComparisonLocation, 0.1f) &&
                Camera->GetComponentRotation().Equals(FlashlightComparisonRotation, 0.05f),
                TEXT("flashlight off/on screenshots use the same camera position and orientation"));
        }
        else Check(false, TEXT("camera exists for flashlight off/on comparison"));
        Capture(TEXT("06d_FlashlightOn"));
        Advance(EStep::ServerWideCapture);
        break;
    case EStep::ServerWideCapture:
        Capture(TEXT("06b_ServerRoomWide"));
        Advance(EStep::AimFuse);
        break;
    case EStep::AimFuse:
        Approach(Fuse.Get(), TEXT("fuse in server room"));
        Capture(TEXT("07_ServerRoomFuse"));
        Advance(EStep::CollectFuse);
        break;
    case EStep::CollectFuse:
        InteractWithAimed(Fuse.Get(), TEXT("fuse in server room"));
        Check(GameMode->HasFuse(), TEXT("visible fuse is reachable and collected"));
        Advance(EStep::AimRareLoot);
        break;
    case EStep::AimRareLoot:
        Approach(RarePickup.Get(), TEXT("optional server parts"));
        Capture(TEXT("07b_OptionalServerParts"));
        Advance(EStep::CollectRareLoot);
        break;
    case EStep::CollectRareLoot:
        InteractWithAimed(RarePickup.Get(), TEXT("optional server parts"));
        Check(Player->GetUsedBagSlots() == 23 && Player->GetLootValue() == 500 && Player->GetReserveAmmo() == 10,
            TEXT("server parts occupy four 2x2 slots beside weapons and ammunition, total 23 slots"));
        for (const FLZInventoryEntry& Entry : Player->GetInventoryEntries())
        {
            if (Entry.Type == ELZInventoryItemType::Rare)
            {
                Check(Entry.Width == 2 && Entry.Height == 2 && Entry.SlotsPerItem == 4 &&
                    Player->GetInventoryItemAtCell(Entry.PosX, Entry.PosY) == &Entry &&
                    Player->GetInventoryItemAtCell(Entry.PosX + 1, Entry.PosY + 1) == &Entry,
                    TEXT("all cells of 2x2 rare parts resolve to the same inventory entry"));
            }
        }
        Check(!RarePickup.IsValid(), TEXT("collected server-parts pickup is removed"));
        PressInputKey(EKeys::B, TEXT("B opens the backpack containing weapons, ammunition and server parts"));
        Advance(EStep::InventoryLootView, 0.2f);
        break;
    case EStep::InventoryLootView:
        if (!Check(Player->IsInventoryOpen(), TEXT("loot backpack is open for the inventory regression")))
        {
            Advance(EStep::Finish);
            break;
        }
        Capture(TEXT("07c_InventoryLoot"));
        InventoryBlockedPosition = Player->GetActorLocation();
        InventoryBlockedRotation = Player->GetControlRotation();
        InventoryProbeWorldTime = Now;
        PressInputKey(EKeys::LeftMouseButton, TEXT("attempted fire while backpack is open"));
        PressInputKey(EKeys::R, TEXT("attempted reload while backpack is open"));
        PressInputKey(EKeys::One, TEXT("attempted weapon switch while backpack is open"));
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
        {
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Pressed, 1.0f));
            Player->AddControllerYawInput(45.0f);
        }
        Advance(EStep::InventoryBlockedInput, 0.3f);
        break;
    case EStep::InventoryBlockedInput:
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::W, IE_Released, 0.0f));
        Check(Player->GetAmmoInMagazine() == 8 && Player->GetReserveAmmo() == 10 &&
            Player->GetSelectedWeapon() == EPlayerWeapon::Firearm && FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.01f),
            TEXT("open backpack blocks firing, reloading and weapon switching without spending ammunition"));
        Check(Player->GetActorLocation().Equals(InventoryBlockedPosition, 0.2f) &&
            Player->GetControlRotation().Equals(InventoryBlockedRotation, 0.05f),
            TEXT("open backpack blocks held movement input and controller look input"));
        Check(!UGameplayStatics::IsGamePaused(this) && Now - InventoryProbeWorldTime >= 0.25,
            TEXT("opening the backpack leaves world time advancing"));
        PressInputKey(EKeys::B, TEXT("B closes backpack to collect world supplies"));
        InventorySupplyIndex = 0;
        Advance(EStep::InventoryCollectSupplies, 0.2f);
        break;
    case EStep::InventoryCollectSupplies:
        {
            if (!Check(!Player->IsInventoryOpen(), TEXT("world supplies are collected with backpack closed")))
            {
                Advance(EStep::Finish);
                break;
            }
            ALZLoot* Supply = InventorySupplyIndex < 2 ? MedicalPickups[InventorySupplyIndex].Get() : ScrapPickup.Get();
            const FString Description = FString::Printf(TEXT("inventory supply %d"), InventorySupplyIndex + 1);
            if (!Approach(Supply, Description))
            {
                Advance(EStep::Finish);
                break;
            }
            PressInputKey(EKeys::E, Description + TEXT(" real pickup input"));
            Advance(EStep::InventorySupplyResult, 0.2f);
        }
        break;
    case EStep::InventorySupplyResult:
        {
            const bool bPickupRemoved = InventorySupplyIndex < 2 ? !MedicalPickups[InventorySupplyIndex].IsValid() : !ScrapPickup.IsValid();
            const int32 ExpectedSlots = (InventorySupplyIndex == 0) ? 25 : ((InventorySupplyIndex == 1) ? 27 : 28);
            if (!Check(bPickupRemoved && Player->GetUsedBagSlots() == ExpectedSlots,
                FString::Printf(TEXT("world supply %d enters the spatial backpack and its pickup is removed"), InventorySupplyIndex + 1)))
            {
                Advance(EStep::Finish);
                break;
            }
            if (++InventorySupplyIndex < 3)
            {
                Advance(EStep::InventoryCollectSupplies, 0.1f);
                break;
            }
            Check(CountInventoryItems(ELZInventoryItemType::Medical) == 2 &&
                CountInventoryItems(ELZInventoryItemType::Scrap) == 1 && Player->GetLootValue() == 620 &&
                FMath::IsNearlyEqual(Player->GetHealth(), 100.0f),
                TEXT("two medical kits remain stored beside the 120-value scrap in the spatial backpack"));
            // Fill remaining 8 slots with QA filler scrap to test rejection when 36/36 capacity is reached
            QAFillerItemIds.Reset();
            while (Player->GetUsedBagSlots() < 36)
            {
                if (Player->TryStoreItem(ELZInventoryItemType::Scrap, 1))
                {
                    const FLZInventoryEntry& LastEntry = Player->GetInventoryEntries().Last();
                    QAFillerItemIds.Add(LastEntry.ItemId);
                }
                else
                {
                    break;
                }
            }
            Check(Player->GetUsedBagSlots() == 36 && QAFillerItemIds.Num() == 8,
                TEXT("36-slot backpack is fully occupied for capacity boundary regression"));
            if (!Approach(OtherRarePickup.Get(), TEXT("rare parts rejected by the full backpack")))
            {
                Advance(EStep::Finish);
                break;
            }
            PressInputKey(EKeys::E, TEXT("real E attempts to collect rare parts with all 36 slots occupied"));
            Advance(EStep::InventoryFullRejected, 0.2f);
        }
        break;
    case EStep::InventoryFullRejected:
        Check(OtherRarePickup.IsValid() && Player->GetUsedBagSlots() == 36 && Player->GetLootValue() == 1580 &&
            !Player->CanStoreItem(ELZInventoryItemType::Rare),
            TEXT("full backpack refuses the world pickup without destroying it or adding its value"));
        Check(Player->GetInventoryStatusText().Contains(TEXT("不足")) || Player->GetInventoryStatusText().Contains(TEXT("满")),
            TEXT("failed world pickup records visible backpack-capacity feedback"));
        Check(!Player->TryStoreItem(ELZInventoryItemType::Ammo, 21) && Player->GetReserveAmmo() == 10 &&
            Player->GetUsedBagSlots() == 36,
            TEXT("rejected 21-round addition is atomic: full bag keeps the existing ten-round stack unchanged"));
        PressInputKey(EKeys::B, TEXT("B opens the full backpack with pickup-failure feedback"));
        Advance(EStep::InventoryFullView, 0.2f);
        break;
    case EStep::InventoryFullView:
        Check(Player->IsInventoryOpen() && Player->GetUsedBagSlots() == 36, TEXT("full 36-slot backpack remains open"));
        Capture(TEXT("07d_InventoryFull"));
        Advance(EStep::InventoryMedicalFullHealth, 0.2f);
        break;
    case EStep::InventoryMedicalFullHealth:
        if (!SelectInventoryItem(ELZInventoryItemType::Medical))
        {
            Advance(EStep::Finish);
            break;
        }
        PressInputKey(EKeys::E, TEXT("E attempts to use a stored medical kit at full health"));
        Advance(EStep::InventoryMedicalNoUse, 0.2f);
        break;
    case EStep::InventoryMedicalNoUse:
        Check(CountInventoryItems(ELZInventoryItemType::Medical) == 2 && Player->GetUsedBagSlots() == 36 &&
            FMath::IsNearlyEqual(Player->GetHealth(), 100.0f),
            TEXT("full-health medical use does not consume a stored kit"));
        UE_LOG(LogTemp, Display, TEXT("LZ_QA INFO applying ordinary 35 damage to verify medical use while the open backpack does not pause gameplay"));
        UGameplayStatics::ApplyDamage(Player.Get(), 35.0f, nullptr, this, nullptr);
        Check(FMath::IsNearlyEqual(Player->GetHealth(), 65.0f), TEXT("player can receive ordinary damage with backpack open"));
        PressInputKey(EKeys::E, TEXT("E uses the selected stored medical kit after taking damage"));
        Advance(EStep::InventoryMedicalUsed, 0.2f);
        break;
    case EStep::InventoryMedicalUsed:
        Check(FMath::IsNearlyEqual(Player->GetHealth(), 100.0f) && CountInventoryItems(ELZInventoryItemType::Medical) == 1 &&
            Player->GetUsedBagSlots() == 34,
            TEXT("real inventory E restores 35 health, consumes one kit and releases its 1x2 slots"));
        if (!SelectInventoryItem(ELZInventoryItemType::Medical))
        {
            Advance(EStep::Finish);
            break;
        }
        InventoryWorldLootCount = CountWorldLoot();
        PressInputKey(EKeys::Delete, TEXT("Delete discards the remaining stored medical kit"));
        Advance(EStep::InventoryMedicalDiscarded, 0.2f);
        break;
    case EStep::InventoryMedicalDiscarded:
        Check(CountInventoryItems(ELZInventoryItemType::Medical) == 0 && Player->GetUsedBagSlots() == 32 &&
            CountWorldLoot() == InventoryWorldLootCount,
            TEXT("Delete removes the entire medical entry and creates no recoverable world pickup"));
        // Remove QA capacity test filler items
        for (int32 FillerId : QAFillerItemIds)
        {
            Player->DiscardItemById(FillerId);
        }
        QAFillerItemIds.Reset();
        Check(Player->GetUsedBagSlots() == 24 && Player->GetLootValue() == 620,
            TEXT("clearing capacity test filler items restores normal progression state (24 slots, 620 loot value)"));
        Check(OtherRarePickup.IsValid() && Player->CanStoreItem(ELZInventoryItemType::Rare),
            TEXT("discarded medical slots leave room for the still-aimed world rare parts"));
        if (!SelectInventoryItem(ELZInventoryItemType::Ammo))
        {
            Advance(EStep::Finish);
            break;
        }
        PressInputKey(EKeys::E, TEXT("inventory E on ammunition must not interact with the world rare parts"));
        Advance(EStep::InventoryWorldBlocked, 0.2f);
        break;
    case EStep::InventoryWorldBlocked:
        Check(OtherRarePickup.IsValid() && Player->GetUsedBagSlots() == 24 && Player->GetLootValue() == 620 &&
            Player->GetReserveAmmo() == 10,
            TEXT("inventory E does not collect the aimed world pickup even when capacity is now available"));
        if (!SelectInventoryItem(ELZInventoryItemType::Scrap))
        {
            Advance(EStep::Finish);
            break;
        }
        PressInputKey(EKeys::Delete, TEXT("Delete discards the inventory-regression scrap"));
        Advance(EStep::InventoryScrapDiscarded, 0.2f);
        break;
    case EStep::InventoryScrapDiscarded:
        Check(CountInventoryItems(ELZInventoryItemType::Scrap) == 0 && Player->GetUsedBagSlots() == 23 &&
            Player->GetLootValue() == 500 && Player->GetReserveAmmo() == 10 && CountWorldLoot() == InventoryWorldLootCount,
            TEXT("discarding scrap removes its 120 value and preserves only ammunition plus the original 500-value parts"));
        PressInputKey(EKeys::B, TEXT("B closes inventory before normal reload slot-release checks"));
        Advance(EStep::InventoryReloadPrepare, 0.2f);
        break;
    case EStep::InventoryReloadPrepare:
        Check(!Player->IsInventoryOpen(), TEXT("inventory closes before continuing normal weapon play"));
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
            Check(!PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("closing backpack releases its movement and look locks"));
        PlaceAndAim(Player->GetActorLocation(), Player->GetActorLocation() - FVector(0.0f, 0.0f, 300.0f));
        PressInputKey(EKeys::R, TEXT("normal R reload draws ammunition from the inventory entry"));
        Advance(EStep::InventoryReloadFirst, 1.65f);
        break;
    case EStep::InventoryReloadFirst:
        Check(Player->GetAmmoInMagazine() == 17 && Player->GetReserveAmmo() == 1 && Player->GetUsedBagSlots() == 23,
            TEXT("reload from 8/10 leaves a real one-round partial stack at 17/1 (23 slots)"));
        // This checks inventory-stack consumption, independent of OS mouse focus
        // in a hidden unattended game window. Exercise the ordinary weapon action.
        PlaceAndAim(Player->GetActorLocation(), Player->GetActorLocation()-FVector(0,0,300));
        Player->QAFire();
        Advance(EStep::InventoryReloadShot, 0.7f);
        break;
    case EStep::InventoryReloadShot:
        Check(Player->GetAmmoInMagazine() == 16 && Player->GetReserveAmmo() == 1 && GameMode->GetEnemiesKilled() == 4,
            TEXT("floor-directed shot consumes one round without changing the completed combat phase"));
        PressInputKey(EKeys::R, TEXT("normal R reload consumes the final inventory round"));
        Advance(EStep::InventoryReloadReleased, 1.65f);
        break;
    case EStep::InventoryReloadReleased:
        Check(Player->GetAmmoInMagazine() == 17 && Player->GetReserveAmmo() == 0 &&
            CountInventoryItems(ELZInventoryItemType::Ammo) == 0 && Player->GetUsedBagSlots() == 22 &&
            Player->GetLootValue() == 500,
            TEXT("consuming the final reserve round deletes the empty stack and releases its backpack slot (22 slots)"));
        Check(Player->HasMeleeWeapon() && Player->HasFirearm() && Player->HasFlashlight() && GameMode->HasFuse(),
            TEXT("inventory operations preserve separately carried weapons, flashlight and quest fuse"));
        Advance(EStep::AimBreaker, 0.2f);
        break;
    case EStep::AimBreaker:
        Approach(Breaker.Get(), TEXT("main breaker"));
        Capture(TEXT("08_MainBreaker"));
        Advance(EStep::RestorePower);
        break;
    case EStep::RestorePower:
        InteractWithAimed(Breaker.Get(), TEXT("main breaker"));
        Check(GameMode->IsOfficePowerRestored() && GameMode->IsObjectiveComplete(), TEXT("installing fuse restores power and completes objective"));
        PlaceAndAim(FVector(2450.0f, 1150.0f, 90.0f), FVector(3000.0f, 1500.0f, 180.0f));
        Advance(EStep::PowerView, 1.0f);
        break;
    case EStep::PowerView:
        Capture(TEXT("09_PowerRestored"));
        Advance(EStep::OfficeWideSetup);
        break;
    case EStep::OfficeWideSetup:
        PlaceAndAim(FVector(-2100.0f, -630.0f, 90.0f), FVector(500.0f, 150.0f, 150.0f));
        Advance(EStep::OfficeWideCapture, 0.7f);
        break;
    case EStep::OfficeWideCapture:
        Capture(TEXT("09b_OfficeRestoredWide"));
        Advance(EStep::AimExit);
        break;
    case EStep::AimExit:
        Approach(Extraction.Get(), TEXT("powered exit"));
        Capture(TEXT("10_Exit"));
        Advance(EStep::Extract);
        break;
    case EStep::Extract:
        InteractWithAimed(Extraction.Get(), TEXT("powered exit"));
        Check(GameMode->IsRunOver() && GameMode->WasExtractionSuccessful(), TEXT("restored exit completes successful extraction"));
        Check(Player->GetUsedBagSlots() == 22 && Player->GetLootValue() == 500,
            TEXT("successful extraction retains the 500-value four-slot parts and weapons after reserve ammunition was consumed (22 slots)"));
        RestoreEnemies();
        SaveEnemySnapshot();
        Advance(EStep::SettlementIdle, 3.25f);
        break;
    case EStep::SettlementIdle:
        CheckEnemySnapshot(TEXT("completed run idle >=3 seconds with normal enemy ticks"));
        Check(!Player->IsInventoryOpen(), TEXT("extraction settlement has no open backpack overlay"));
        CheckFlashlightState(false, TEXT("extraction settlement disables flashlight emission"));
        Capture(TEXT("11_ExtractionSuccess"));
        Advance(EStep::RequestRestart, 1.0f);
        break;
    case EStep::RequestRestart:
        {
            APlayerController* PC = Cast<APlayerController>(Player->GetController());
            if (!Check(PC != nullptr, TEXT("player controller exists for real F5 input")))
            {
                Advance(EStep::Finish);
                break;
            }
            GSliceQARestart.bRestartRequested = true;
            const bool bAccepted = PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5, IE_Pressed, 1.0f));
            PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5, IE_Released, 0.0f));
            Check(bAccepted, TEXT("player input accepts F5 after extraction"));
            GSliceQARestart.Assertions = Assertions;
            GSliceQARestart.Failures = Failures;
            GSliceQARestart.Elapsed = Now - StartedAt;
            GSliceQARestart.ReportLines = ReportLines;
            GSliceQARestart.ScreenshotPaths = ScreenshotPaths;
            Advance(EStep::AwaitRestart, 8.0f);
        }
        break;
    case EStep::AwaitRestart:
        Check(false, TEXT("F5 did not reload the level within 8 seconds"));
        Advance(EStep::Finish);
        break;
    case EStep::VerifyRestart:
        if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
        {
            Check(!Viewport->GetEngineShowFlags()->ShaderComplexity,
                TEXT("F5 restart preserves normal rendering instead of shader-complexity debug mode"));
        }
        Check(!Player->HasMeleeWeapon() && !Player->HasFirearm() && Player->GetSelectedWeapon() == EPlayerWeapon::None,
            TEXT("F5 new run starts unarmed"));
        Check(FMath::IsNearlyEqual(Player->GetHealth(), 100.0f), TEXT("F5 new run restores 100 health"));
        Check(Player->GetAmmoInMagazine() == 0 && Player->GetReserveAmmo() == 0, TEXT("F5 new run clears ammunition"));
        Check(!Player->HasFlashlight() && FMath::IsNearlyZero(Player->GetRecoilPitch(), 0.01f),
            TEXT("F5 new run clears flashlight ownership and recoil"));
        CheckFlashlightState(false, TEXT("F5 new run leaves flashlight off"));
        {
            int32 FlashlightCount = 0;
            for (TActorIterator<ALZFlashlightPickup> It(GetWorld()); It; ++It) ++FlashlightCount;
            Check(FlashlightCount == 1, TEXT("F5 respawns one collectible flashlight"));
        }
        Check(Player->GetUsedBagSlots() == 0 && Player->GetLootValue() == 0 &&
            Player->GetInventoryEntries().IsEmpty() && !Player->IsInventoryOpen(), TEXT("F5 new run clears inventory entries and closes the backpack"));
        Check(GameMode->GetEnemiesKilled() == 0, TEXT("F5 new run clears kill count"));
        Check(!GameMode->IsOfficeBlackout() && !GameMode->HasFuse() && !GameMode->IsOfficePowerRestored(),
            TEXT("F5 new run clears blackout, fuse and restored-power state"));
        Check(!GameMode->IsRunOver() && !GameMode->WasExtractionSuccessful() && !GameMode->IsObjectiveComplete(),
            TEXT("F5 new run clears outcome and objective state"));
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
        {
            Check(!PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("F5 new run restores movement and look input"));
        }
        CheckOpeningPresentation();
        Capture(TEXT("12_RestartedOpening"));
        Advance(EStep::DesignRegression, 1.0f);
        break;
    case EStep::DesignRegression:
        CheckOfficeDesign();
        PlaceAndAim(FVector(-1800,1100,90),FVector(800,1200,180));
        Advance(EStep::DesignGallery,1.0f);
        break;
    case EStep::DesignGallery:
        Capture(TEXT("13_AdminGallery"));
        Advance(EStep::DesignServiceSetup,.3f);
        break;
    case EStep::DesignServiceSetup:
        PlaceAndAim(FVector(-2100,-1080,90),FVector(700,-1250,160));
        Advance(EStep::DesignService,1.0f);
        break;
    case EStep::DesignService:
        Capture(TEXT("14_SupportGallery"));
        Advance(EStep::DesignOverviewSetup,.3f);
        break;
    case EStep::DesignOverviewSetup:
        PlaceAndAim(FVector(3330,-1540,90),FVector(2350,-1540,150));
        Advance(EStep::DesignOverview,1.0f);
        break;
    case EStep::DesignOverview:
        Capture(TEXT("15_ServerLoop"));
        Advance(EStep::MotionPrepare,1.0f);
        break;
    case EStep::MotionPrepare:
        IsolateEnemies();
        PlaceAndAim(FVector(1450,0,90),FVector(1000,0,100));
        MotionEnemy=GetWorld()->SpawnActor<ALZEnemy>(FVector(1000,0,90),FRotator(0,120,0));
        MotionOrigin=MotionEnemy->GetActorLocation();
        Check(MotionEnemy->GetMotionProfile() && MotionEnemy->GetMotionProfile()->SourceLabel.Contains(TEXT("Game Animation Sample")),TEXT("enemy uses imported official GASP profile"));
        Check(Cast<ULZMotionAnimInstance>(MotionEnemy->GetMesh()->GetAnimInstance())!=nullptr,TEXT("native locomotion animation instance active"));
        Advance(EStep::MotionChase,.65f); break;
    case EStep::MotionChase:
        Check(FVector::Dist2D(MotionOrigin,MotionEnemy->GetActorLocation())>30,TEXT("enemy accelerates and chases using CharacterMovement"));
        Check(MotionEnemy->GetVelocity().Size2D()>50,TEXT("locomotion receives real velocity"));
        Check(FVector::DotProduct(MotionEnemy->GetActorForwardVector(),(Player->GetActorLocation()-MotionEnemy->GetActorLocation()).GetSafeNormal2D())>.8f,TEXT("enemy turns toward pursuit direction"));
        {
            auto* Path=UNavigationSystemV1::FindPathToLocationSynchronously(this,MotionEnemy->GetActorLocation(),Player->GetActorLocation());
            Check(Path && Path->IsValid() && Path->PathPoints.Num()>1,TEXT("office has a generated navigable path"));
        }
        Capture(TEXT("16_GASP_Chase"));
        UGameplayStatics::ApplyDamage(MotionEnemy.Get(),5,Player->GetController(),Player.Get(),nullptr);
        Check(MotionEnemy->GetMotionState()==ELZEnemyMotionState::Staggered,TEXT("light hit interrupts locomotion"));
        MotionOrigin=MotionEnemy->GetActorLocation();
        Advance(EStep::MotionStagger,.2f); break;
    case EStep::MotionStagger:
        Check(FVector::Dist2D(MotionOrigin,MotionEnemy->GetActorLocation())<1,TEXT("stagger stops displacement"));
        Capture(TEXT("17_GASP_Stagger"));
        Advance(EStep::MotionRecover,.3f); break;
    case EStep::MotionRecover:
        Check(MotionEnemy->GetMotionState()==ELZEnemyMotionState::Locomotion,TEXT("stagger returns to locomotion"));
        UGameplayStatics::ApplyDamage(MotionEnemy.Get(),45,Player->GetController(),Player.Get(),nullptr);
        Check(MotionEnemy->GetMotionState()==ELZEnemyMotionState::KnockedDown,TEXT("heavy nonlethal hit knocks down"));
        SnapshotHealth=Player->GetHealth();
        Advance(EStep::MotionDown,.7f); break;
    case EStep::MotionDown:
        Check(MotionEnemy->IsIncapacitated() && Player->GetHealth()==SnapshotHealth,TEXT("downed enemy cannot attack"));
        Check(MotionEnemy->GetMesh()->IsSimulatingPhysics(),TEXT("knockdown uses physical ragdoll"));
        Capture(TEXT("18_GASP_Knockdown"));
        Advance(EStep::MotionGetUp,1.6f); break;
    case EStep::MotionGetUp:
        Check(MotionEnemy->GetMotionState()==ELZEnemyMotionState::GettingUp,TEXT("knockdown transitions to official get-up animation"));
        Check(!MotionEnemy->GetMesh()->IsSimulatingPhysics(),TEXT("get-up restores animation control"));
        Capture(TEXT("19_GASP_GetUp"));
        Advance(EStep::MotionRecovered,4.9f); break;
    case EStep::MotionRecovered:
        Check(MotionEnemy->GetMotionState()==ELZEnemyMotionState::Locomotion,TEXT("get-up finishes and re-enables chase"));
        MotionEnemy->Destroy();
        {
            auto* Victim=GetWorld()->SpawnActor<ALZEnemy>(FVector(1000,0,90),FRotator::ZeroRotator);
            UGameplayStatics::ApplyDamage(Victim,55,Player->GetController(),Player.Get(),nullptr);
            const int32 BeforeKills=GameMode->GetEnemiesKilled();
            UGameplayStatics::ApplyDamage(Victim,34,Player->GetController(),Player.Get(),nullptr);
            Check(Victim->IsActorBeingDestroyed() && GameMode->GetEnemiesKilled()==BeforeKills+1,TEXT("downed enemy remains damageable and counts exactly one kill"));
        }
        Advance(EStep::VaultPrepare,.3f); break;
    case EStep::VaultPrepare:
        PlaceAndAim(FVector(-1850,-1040,90),FVector(-1850,-1450,150));
        Advance(EStep::VaultStart,.5f); break;
    case EStep::VaultStart:
        Check(Player->CanVault(),TEXT("meeting shortcut has free headroom and landing"));
        {
            auto* NavPath=UNavigationSystemV1::FindPathToLocationSynchronously(this,Player->GetActorLocation(),FVector(-1850,-1350,90));
            Check(NavPath && NavPath->IsValid() && NavPath->PathPoints.Num()>2,TEXT("walking route goes around barrier through the meeting door"));
            auto* Block=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-1850,-1300,100),FRotator::ZeroRotator);
            auto* SM=Block->GetStaticMeshComponent(); SM->SetMobility(EComponentMobility::Movable);
            SM->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            SM->SetWorldScale3D(FVector(1,1,2)); SM->SetCollisionProfileName(TEXT("BlockAll"));
            Check(!Player->TryVault(),TEXT("occupied landing rejects vault"));
            Block->SetActorLocation(FVector(-1850,-1150,245)); SM->SetWorldScale3D(FVector(2,2,.2f));
            Check(!Player->TryVault(),TEXT("low ceiling rejects vault"));
            Block->Destroy();
        }
        Player->ToggleInventory();
        Check(!Player->TryVault(),TEXT("inventory blocks vault"));
        Player->ToggleInventory();
        Check(Player->TryVault(),TEXT("space action starts authored barrier traversal"));
        MotionAmmo=Player->GetAmmoInMagazine();
        Player->QAFire(); Player->QAReload(); Player->ToggleInventory();
        Check(Player->GetAmmoInMagazine()==MotionAmmo && !Player->IsInventoryOpen(),TEXT("vault blocks fire reload and backpack"));
        Advance(EStep::VaultView,.55f); break;
    case EStep::VaultView:
        Check(Player->IsTraversing() && Player->GetActorLocation().Z>140,TEXT("vault follows raised collision-safe path over barrier"));
        Capture(TEXT("20_GASP_Vault"));
        Advance(EStep::VaultEnd,.9f); break;
    case EStep::VaultEnd:
        Check(!Player->IsTraversing() && Player->GetActorLocation().Y<-1280,TEXT("vault lands inside meeting room and releases movement"));
        Check(!Player->CanVault(),TEXT("out-of-range barrier cannot trigger another vault"));
        Capture(TEXT("21_MeetingShortcutLanding"));
        Advance(EStep::VaultBodyStart,.5f); break;
    case EStep::VaultBodyStart:
        PlaceAndAim(FVector(-1850,-1040,90),FVector(-1850,-1450,150));
        Check(Player->TryVault(),TEXT("vault can be used again after landing"));
        MotionCamera=GetWorld()->SpawnActor<ACameraActor>(FVector(-1500,-1100,230),FRotator::ZeroRotator);
        MotionCamera->SetActorRotation((FVector(-1850,-1200,150)-MotionCamera->GetActorLocation()).Rotation());
        Cast<APlayerController>(Player->GetController())->SetViewTarget(MotionCamera.Get());
        Advance(EStep::VaultBodyView,.55f); break;
    case EStep::VaultBodyView:
        Check(Player->GetMesh()->GetSingleNodeInstance() && Player->GetMesh()->GetSingleNodeInstance()->GetCurrentTime()>.5f,TEXT("official vault body animation advances during traversal"));
        Capture(TEXT("22_GASP_VaultBody_DebugCamera"));
        Advance(EStep::VaultBodyEnd,.8f); break;
    case EStep::VaultBodyEnd:
        Cast<APlayerController>(Player->GetController())->SetViewTarget(Player.Get());
        MotionCamera->Destroy();
        Advance(EStep::Finish,.3f); break;
    case EStep::Finish:
        CompleteRun();
        break;
    }
#endif
}

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
void ALZSliceQA::Advance(EStep NextStep, float Delay)
{
    Step = NextStep;
    NextStepTime = GetWorld()->GetTimeSeconds() + Delay;
}

bool ALZSliceQA::Check(bool bCondition, const FString& Description)
{
    ++Assertions;
    const FString Line = FString::Printf(TEXT("LZ_QA %s %s"), bCondition ? TEXT("PASS") : TEXT("FAIL"), *Description);
    ReportLines.Add(Line);
    if (bCondition)
    {
        UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
    }
    else
    {
        ++Failures;
        UE_LOG(LogTemp, Error, TEXT("%s"), *Line);
    }
    return bCondition;
}

void ALZSliceQA::Capture(const FString& Name)
{
    const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/WindowsEditor"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Filename = Directory / (TEXT("SliceQA_") + Name + TEXT(".png"));
    IFileManager::Get().Delete(*Filename, false, true);
    ScreenshotPaths.Add(Filename);
    FScreenshotRequest::RequestScreenshot(Filename, true, false);
    UE_LOG(LogTemp, Display, TEXT("LZ_QA SCREENSHOT %s"), *Filename);
}

void ALZSliceQA::PlaceAndAim(const FVector& Position, const FVector& Target)
{
    Player->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
    Player->GetCharacterMovement()->StopMovementImmediately();
    UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
    if (Camera)
    {
        const FRotator Rotation = (Target - Camera->GetComponentLocation()).Rotation();
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
        {
            PC->SetControlRotation(Rotation);
            if (PC->PlayerCameraManager) PC->PlayerCameraManager->SetGameCameraCutThisFrame();
        }
        // Sync the camera for an immediate real FindInteractable trace before the next camera manager update.
        Camera->SetWorldRotation(Rotation);
    }
}

bool ALZSliceQA::Approach(ALZInteractable* Target, const FString& Description)
{
    if (!Check(IsValid(Target), Description + TEXT(" actor exists"))) return false;
    FVector AimPoint = Target->GetActorLocation();
    if (const UStaticMeshComponent* Mesh = Target->FindComponentByClass<UStaticMeshComponent>()) AimPoint = Mesh->Bounds.Origin;
    const FVector OriginalLocation = Player->GetActorLocation();
    const FVector FromTarget = (OriginalLocation - AimPoint).GetSafeNormal2D();
    const float InitialAngle = FMath::Atan2(FromTarget.Y, FromTarget.X);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SliceQAStand), false, Player.Get());
    const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
    const float Distances[] = { 170.0f, 220.0f, 120.0f, 270.0f };
    for (float Distance : Distances)
    {
        for (int32 Index = 0; Index < 16; ++Index)
        {
            const float Angle = InitialAngle + Index * UE_PI / 8.0f;
            const FVector Candidate(AimPoint.X + FMath::Cos(Angle) * Distance,
                AimPoint.Y + FMath::Sin(Angle) * Distance, Capsule->GetScaledCapsuleHalfHeight() + 2.0f);
            if (GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, Shape, Params)) continue;
            PlaceAndAim(Candidate, AimPoint);
            if (Player->FindInteractable() == Target)
            {
                return Check(true, Description + TEXT(" has a clear <=350cm interaction trace from a free standing position"));
            }
        }
    }
    PlaceAndAim(OriginalLocation, AimPoint);
    return Check(false, Description + TEXT(" is not reachable by FindInteractable from any tested free standing position"));
}

bool ALZSliceQA::InteractWithAimed(ALZInteractable* Target, const FString& Description)
{
    if (!Check(IsValid(Target) && Player->FindInteractable() == Target, Description + TEXT(" actual interaction trace hits target"))) return false;
    Target->Interact(Player.Get());
    return true;
}

bool ALZSliceQA::PressInputKey(const FKey& Key, const FString& Description)
{
    APlayerController* PC = Player.IsValid() ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!PC) return Check(false, Description + TEXT(" requires a player controller"));
    const bool bAccepted = PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.0f));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.0f));
    return Check(bAccepted, Description + TEXT(" is accepted by the normal input path"));
}

void ALZSliceQA::CheckFlashlightState(bool bExpectedOn, const FString& Description)
{
    const USpotLightComponent* Light = Player.IsValid() ? Player->FindComponentByClass<USpotLightComponent>() : nullptr;
    Check(Player.IsValid() && Light && Player->IsFlashlightOn() == bExpectedOn &&
        Light->IsVisible() == bExpectedOn && (!bExpectedOn || Light->Intensity > 0.0f), Description);
}

int32 ALZSliceQA::CountInventoryItems(ELZInventoryItemType Type) const
{
    int32 Quantity = 0;
    for (const FLZInventoryEntry& Entry : Player->GetInventoryEntries())
        if (Entry.Type == Type) Quantity += Entry.Quantity;
    return Quantity;
}

bool ALZSliceQA::SelectInventoryItem(ELZInventoryItemType Type)
{
    int32 TargetX = INDEX_NONE;
    int32 TargetY = INDEX_NONE;
    for (const FLZInventoryEntry& Entry : Player->GetInventoryEntries())
    {
        if (Entry.Type == Type)
        {
            TargetX = Entry.PosX;
            TargetY = Entry.PosY;
            break;
        }
    }
    if (!Check(Player->IsInventoryOpen() && TargetX >= 0 && TargetY >= 0,
        TEXT("requested inventory item exists in an open backpack"))) return false;
    for (int32 Attempt = 0; Attempt < 20 && (Player->GetCursorX() != TargetX || Player->GetCursorY() != TargetY); ++Attempt)
    {
        if (Player->GetCursorY() < TargetY) Player->InventoryDown();
        else if (Player->GetCursorY() > TargetY) Player->InventoryUp();
        else if (Player->GetCursorX() < TargetX) Player->InventoryRight();
        else if (Player->GetCursorX() > TargetX) Player->InventoryLeft();
    }
    return Check(Player->GetCursorX() == TargetX && Player->GetCursorY() == TargetY,
        TEXT("inventory grid navigation selects the requested item's cell"));
}

int32 ALZSliceQA::CountWorldLoot() const
{
    int32 Count = 0;
    for (TActorIterator<ALZLoot> It(GetWorld()); It; ++It) ++Count;
    return Count;
}

bool ALZSliceQA::ApproachEnemy(ALZEnemy* Target)
{
    const FString Description = FString::Printf(TEXT("infected %d"), KillIndex + 1);
    if (!Check(IsValid(Target), Description + TEXT(" exists for real pistol trace"))) return false;
    const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
    const FVector OriginalLocation = Player->GetActorLocation();
    const FVector FromTarget = (OriginalLocation - AimPoint).GetSafeNormal2D();
    const float InitialAngle = FMath::Atan2(FromTarget.Y, FromTarget.X);
    FCollisionQueryParams StandParams(SCENE_QUERY_STAT(SliceQACombatStand), false, Player.Get());
    FCollisionQueryParams ShotParams(SCENE_QUERY_STAT(SliceQACombatSightline), true, Player.Get());
    const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
    UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
    if (!Check(Camera != nullptr, Description + TEXT(" player camera exists"))) return false;
    const float Distances[] = { 220.0f, 180.0f, 260.0f, 320.0f };
    for (float Distance : Distances)
    {
        for (int32 Index = 0; Index < 16; ++Index)
        {
            const float Angle = InitialAngle + Index * UE_PI / 8.0f;
            const FVector Candidate(AimPoint.X + FMath::Cos(Angle) * Distance,
                AimPoint.Y + FMath::Sin(Angle) * Distance, Capsule->GetScaledCapsuleHalfHeight() + 2.0f);
            if (GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, ECC_Pawn, Shape, StandParams)) continue;
            PlaceAndAim(Candidate, AimPoint);
            FHitResult Hit;
            const FVector Start = Camera->GetComponentLocation();
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Camera->GetForwardVector() * 8000.0f,
                    ECC_Visibility, ShotParams) && Hit.GetActor() == Target)
            {
                return Check(true, Description + TEXT(" is the first weapon-trace hit from a free standing position"));
            }
        }
    }
    PlaceAndAim(OriginalLocation, AimPoint);
    return Check(false, Description + TEXT(" has no unobstructed weapon trace from tested free standing positions"));
}

void ALZSliceQA::SaveEnemySnapshot()
{
    SnapshotEnemies.Reset();
    SnapshotPositions.Reset();
    for (TActorIterator<ALZEnemy> It(GetWorld()); It; ++It)
    {
        SnapshotEnemies.Add(*It);
        SnapshotPositions.Add(It->GetActorLocation());
    }
    SnapshotHealth = Player->GetHealth();
}

void ALZSliceQA::CheckEnemySnapshot(const FString& Description)
{
    bool bStationary = true;
    for (int32 Index = 0; Index < SnapshotEnemies.Num(); ++Index)
    {
        if (!SnapshotEnemies[Index].IsValid() || FVector::DistSquared2D(SnapshotPositions[Index], SnapshotEnemies[Index]->GetActorLocation()) > 1.0f)
            bStationary = false;
    }
    Check(bStationary, Description + TEXT(" leaves every enemy XY unchanged within 1cm"));
    Check(FMath::IsNearlyEqual(Player->GetHealth(), SnapshotHealth), Description + TEXT(" causes no player damage"));
}

void ALZSliceQA::CheckOpeningPresentation()
{
    TArray<UStaticMeshComponent*> Meshes;
    Player->GetComponents<UStaticMeshComponent>(Meshes);
    const FName ExpectedNames[] = { FName(TEXT("MeleeHead")), FName(TEXT("WeaponBody")) };
    for (const FName& ExpectedName : ExpectedNames)
    {
        UStaticMeshComponent* MatchingMesh = nullptr;
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            if (Mesh->GetFName() == ExpectedName) MatchingMesh = Mesh;
        }
        Check(MatchingMesh && !MatchingMesh->IsVisible(),
            FString::Printf(TEXT("unarmed first-person %s is explicitly invisible"), *ExpectedName.ToString()));
    }
}

void ALZSliceQA::IsolateEnemies()
{
    bIsolatingEnemies = true;
    for (TActorIterator<ALZEnemy> It(GetWorld()); It; ++It)
    {
        if (*It==MotionEnemy.Get()) continue;
        TWeakObjectPtr<ALZEnemy> Key(*It);
        if (!SavedEnemyTickStates.Contains(Key)) SavedEnemyTickStates.Add(Key, It->IsActorTickEnabled());
        It->SetActorTickEnabled(false);
        It->GetCharacterMovement()->StopMovementImmediately();
        It->ConsumeMovementInputVector();
    }
}

void ALZSliceQA::RestoreEnemies()
{
    bIsolatingEnemies = false;
    for (const TPair<TWeakObjectPtr<ALZEnemy>, bool>& Pair : SavedEnemyTickStates)
        if (Pair.Key.IsValid()) Pair.Key->SetActorTickEnabled(Pair.Value);
    SavedEnemyTickStates.Reset();
}

void ALZSliceQA::CheckOfficeDesign()
{
    IsolateEnemies();
    Check(!GameMode->IsCombatUnlocked(), TEXT("restart clears the latched tutorial combat gate"));
    // Full inventory equipment pickups must be atomic and remain in the world.
    for(int32 I=0; I<36; ++I) Player->TryStoreItem(ELZInventoryItemType::Scrap);
    Check(Player->GetUsedBagSlots()==36,TEXT("equipment capacity test fills all 36 cells"));
    for(TActorIterator<ALZWeaponPickup> It(GetWorld()); It; ++It)
    {
        It->Interact(Player.Get());
        Check(!It->IsActorBeingDestroyed(),TEXT("full bag preserves world weapon pickup"));
    }
    for(TActorIterator<ALZFlashlightPickup> It(GetWorld()); It; ++It)
    {
        It->Interact(Player.Get());
        Check(!It->IsActorBeingDestroyed(),TEXT("full bag preserves world flashlight pickup"));
    }
    Check(!Player->HasMeleeWeapon() && !Player->HasFirearm() && !Player->HasFlashlight(),TEXT("rejected equipment grants no ownership or ammunition"));
    Player->ToggleInventory();
    auto Entries=Player->GetInventoryEntries();
    for(const auto& E:Entries) Player->DiscardItemById(E.ItemId);
    Player->ToggleInventory();
    Player->AcquireWeapon(EPlayerWeapon::Melee);
    Player->AcquireWeapon(EPlayerWeapon::Firearm);
    Check(GameMode->IsCombatUnlocked(),TEXT("both weapons latch combat open"));
    Player->ToggleInventory();
    const FLZInventoryEntry Axe=Player->GetInventoryEntries()[0];
    Player->PickUpItemAtCell(Axe.PosX,Axe.PosY);
    Player->RotateHeldItem();
    Check(Player->GetHeldWidth()==6 && Player->GetHeldHeight()==2,TEXT("held axe rotates from 2x6 to 6x2"));
    Player->PlaceHeldItemAtCell(5,5);
    const FLZInventoryEntry* Rotated=Player->GetInventoryItemById(Axe.ItemId);
    Check(Rotated && Rotated->PosX>=0 && Rotated->PosY>=0 && Rotated->PosX+Rotated->Width<=6 && Rotated->PosY+Rotated->Height<=6,
        TEXT("edge drop clamps rotated equipment fully inside the backpack"));
    Player->CancelHeldItem();
    Check(Player->GetUsedBagSlots()==16 && Player->HasMeleeWeapon(),TEXT("cancel invalid placement preserves equipment and occupied cells"));
    Player->DiscardItemById(Axe.ItemId);
    Player->ToggleInventory();
    Check(GameMode->IsCombatUnlocked() && !Player->HasMeleeWeapon(),TEXT("discarding axe cannot reset the tutorial or freeze combat"));
    PlaceAndAim(FVector(1450,0,90),FVector(1000,0,90));
    ALZEnemy* Probe=GetWorld()->SpawnActor<ALZEnemy>(FVector(1000,0,90),FRotator::ZeroRotator);
    const FVector Before=Probe->GetActorLocation();
    Probe->Tick(.1f);
    Probe->GetCharacterMovement()->TickComponent(.1f,LEVELTICK_All,nullptr);
    Check(FVector::Dist2D(Before,Probe->GetActorLocation())>1,TEXT("enemy actually moves after the axe is discarded"));
    Probe->Destroy();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OfficeRouteSweep),false,Player.Get());
    for(TActorIterator<APawn> It(GetWorld());It;++It) Params.AddIgnoredActor(*It);
    for(TActorIterator<ALZInteractable> It(GetWorld());It;++It) Params.AddIgnoredActor(*It);
    auto Clear = [&](FVector A,FVector B)
    {
        A.Z=B.Z=90;
        FHitResult Hit;
        const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Params);
        if(Blocked) UE_LOG(LogTemp,Warning,TEXT("LZ_ROUTE obstruction %s component=%s at=%s"),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.Location.ToString());
        return !Blocked;
    };
    auto Route = [&](const TCHAR* Name,TArray<FVector> Points)
    {
        bool Good=true;
        for(int32 I=1;I<Points.Num();++I) Good=Clear(Points[I-1],Points[I]) && Good;
        Check(Good,FString(Name)+TEXT(" is continuously clear for a standing 84x176cm capsule"));
    };
    Route(TEXT("central office spine"),{FVector(-2300,-610,90),FVector(-2300,-200,90),FVector(3300,-200,90)});
    Route(TEXT("admin public gallery"),{FVector(-2200,600,90),FVector(-2200,1100,90),FVector(1900,1100,90),FVector(1900,0,90)});
    Route(TEXT("southern support gallery"),{FVector(-2250,-600,90),FVector(-2250,-1080,90),FVector(1000,-1080,90)});
    Route(TEXT("server east loop to electrical room"),{FVector(1850,0,90),FVector(1850,-1050,90),FVector(2800,-1050,90),FVector(2800,-1540,90),FVector(3350,-1540,90),FVector(3350,600,90),FVector(3000,600,90),FVector(3000,1150,90)});
    Route(TEXT("manager doorway"),{FVector(-1100,1100,90),FVector(-1100,1540,90)});
    Route(TEXT("meeting doorway"),{FVector(-1450,-1080,90),FVector(-1450,-1450,90)});
    Route(TEXT("pantry doorway"),{FVector(300,-1080,90),FVector(300,-1450,90)});
    Route(TEXT("archive doorway"),{FVector(500,1100,90),FVector(500,1560,90)});
    Check(!Clear(FVector(1000,-1100,90),FVector(1300,-1100,90)),TEXT("server inside-release shortcut starts physically locked"));
    Check(!Clear(FVector(-1400,760,90),FVector(-1300,760,90)),TEXT("structural column blocks the player capsule"));
    float DeskBefore=0; APointLight* Desk=nullptr;
    for(TActorIterator<APointLight> It(GetWorld());It;++It)
        if(It->GetActorLocation().Equals(FVector(-3100,490,155),1)) { Desk=*It; DeskBefore=Desk->GetLightComponent()->Intensity; }
    for(int32 I=0;I<4;++I) GameMode->NotifyEnemyKilled();
    GameMode->CollectFuse();
    Check(Clear(FVector(1000,-1100,90),FVector(1300,-1100,90)),TEXT("collecting fuse opens a physically traversable return shortcut"));
    GameMode->TryRestoreOfficePower();
    Check(Desk && FMath::IsNearlyEqual(DeskBefore,Desk->GetLightComponent()->Intensity),TEXT("power restoration preserves original desk-light intensity"));
    auto EnemyCount=[&](){int32 N=0;for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)++N;return N;};
    const int32 InitialEnemies=EnemyCount();
    GameMode->TryActivatePuzzleNode(EPuzzleNode::Cooling);
    GameMode->TryActivatePuzzleNode(EPuzzleNode::Cooling);
    Check(GameMode->GetPuzzleStep()==0 && EnemyCount()==InitialEnemies+1,TEXT("repeated wrong SOP inputs reset progress with at most one alarm reinforcement"));
    Check(!Clear(FVector(-450,1150,90),FVector(-450,1550,90)),TEXT("optional records room is physically locked before SOP"));
    const int32 LootBefore=CountWorldLoot();
    const EPuzzleNode Order[]={EPuzzleNode::Clue,EPuzzleNode::Generator,EPuzzleNode::Cooling,EPuzzleNode::Purifier};
    for(EPuzzleNode Node:Order)
    {
        ALZPuzzleTerminal* Terminal=nullptr;
        for(TActorIterator<ALZPuzzleTerminal> It(GetWorld());It;++It) if(It->GetNode()==Node) Terminal=*It;
        const FString Name=FString::Printf(TEXT("SOP terminal %d"),static_cast<int32>(Node));
        if(Approach(Terminal,Name)) InteractWithAimed(Terminal,Name);
    }
    Check(GameMode->IsPuzzleComplete() && CountWorldLoot()==LootBefore+2,TEXT("complete SOP unlocks records and creates exactly two rewards"));
    Check(Clear(FVector(-450,1150,90),FVector(-450,1550,90)),TEXT("unlocked records reward is physically reachable"));
    GameMode->TryActivatePuzzleNode(EPuzzleNode::Purifier);
    Check(CountWorldLoot()==LootBefore+2,TEXT("completed SOP cannot duplicate rewards"));
    IsolateEnemies();
}

void ALZSliceQA::CompleteRun()
{
    RestoreEnemies();
    for (const FString& Filename : ScreenshotPaths)
        Check(IFileManager::Get().FileSize(*Filename) > 0, TEXT("screenshot saved: ") + FPaths::GetCleanFilename(Filename));
    const FString Summary = FString::Printf(TEXT("LZ_QA SUMMARY %s assertions=%d failures=%d elapsed=%.2fs"),
        Failures == 0 ? TEXT("PASS") : TEXT("FAIL"), Assertions, Failures,
        ElapsedBeforeRestart + GetWorld()->GetTimeSeconds() - StartedAt);
    ReportLines.Add(Summary);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Summary);
    const FString ReportPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("LZSliceQA_Report.txt"));
    FFileHelper::SaveStringToFile(FString::Join(ReportLines, TEXT("\n")), *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8);
    bRunning = false;
    GSliceQARestart.bCompleted = true;
    SetActorTickEnabled(false);
    if (FParse::Param(FCommandLine::Get(), TEXT("LZQAExit")))
        FPlatformMisc::RequestExitWithStatus(false, Failures == 0 ? 0 : 1, TEXT("LZSliceQA completed"));
}
#endif
