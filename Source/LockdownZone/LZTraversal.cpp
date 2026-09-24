#include "LZCharacter.h"
#include "LZGameMode.h"
#include "Camera/CameraComponent.h"
#include "LZMotionProfile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"

bool ALZCharacter::FindVaultPath(FVector& Lift,FVector& Across,FVector& End) const
{
    const auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
    if(bTraversing || bInventoryOpen || IsRunInactive() || bIsCrouched || !GM || !GM->IsCombatUnlocked() || !GetCharacterMovement()->IsMovingOnGround()) return false;
    const FVector Forward=FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
    const float Half=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius=GetCapsuleComponent()->GetScaledCapsuleRadius();
    const FVector Start=GetActorLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LZVault),false,this);
    FHitResult Obstacle;
    const FVector Probe=Start+FVector(0,0,75-Half);
    if(!GetWorld()->LineTraceSingleByChannel(Obstacle,Probe,Probe+Forward*165,ECC_Visibility,Params) || !Obstacle.GetActor() || !Obstacle.GetActor()->ActorHasTag(TEXT("LZVaultable"))) return false;
    if(FVector::DotProduct(Obstacle.ImpactNormal,Forward)>-.75f) return false;
    FVector Center,Extent; Obstacle.GetActor()->GetActorBounds(true,Center,Extent);
    const float Top=Center.Z+Extent.Z;
    const float Height=Top-(Start.Z-Half);
    const float Depth=FMath::Abs(Forward.X)*Extent.X+FMath::Abs(Forward.Y)*Extent.Y;
    if(Height<55 || Height>125 || Depth>90) return false;
    End=Start+Forward*(FVector::DotProduct(Center-Start,Forward)+Depth+Radius+28);
    FHitResult Floor;
    if(!GetWorld()->LineTraceSingleByChannel(Floor,End+FVector(0,0,100),End-FVector(0,0,Half+55),ECC_Visibility,Params) || Floor.ImpactNormal.Z<.7f) return false;
    End.Z=Floor.ImpactPoint.Z+Half+2;
    if(FMath::Abs(End.Z-Start.Z)>45) return false;
    Lift=Start; Lift.Z=Top+Half+12;
    Across=End; Across.Z=Lift.Z;
    const FCollisionShape Shape=FCollisionShape::MakeCapsule(Radius,Half);
    auto Clear=[&](FVector A,FVector B){FHitResult Hit;return !GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,Shape,Params);};
    return !GetWorld()->OverlapBlockingTestByChannel(End,FQuat::Identity,ECC_Pawn,Shape,Params) && Clear(Start,Lift) && Clear(Lift,Across) && Clear(Across,End);
}
bool ALZCharacter::CanVault() const { FVector A,B,C; return FindVaultPath(A,B,C); }
bool ALZCharacter::TryVault()
{
    if(!FindVaultPath(VaultLift,VaultAcross,VaultEnd)) return false;
    CancelWeaponActions();
    VaultStart=GetActorLocation(); VaultElapsed=0; bTraversing=true;
    ConsumeMovementInputVector(); GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    UpdateWeaponVisibility();
    if(!TraversalProfile) TraversalProfile=LoadObject<ULZMotionProfile>(nullptr,TEXT("/Game/Gameplay/DA_LZMotion.DA_LZMotion"));
    if(TraversalProfile && TraversalProfile->Mesh && TraversalProfile->Vault)
    {
        GetMesh()->SetSkeletalMeshAsset(TraversalProfile->Mesh);
        GetMesh()->SetRelativeLocation(FVector(0,0,-88));
        GetMesh()->SetWorldRotation(FRotator(0,GetControlRotation().Yaw-90,0));
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetVisibility(true);
        // GASP is full-body third-person animation. Keep it out of the FPS camera;
        // a dedicated first-person hand plant needs an authored arms rig/IK pass.
        GetMesh()->SetOwnerNoSee(true);
        GetMesh()->SetCastHiddenShadow(true);
        GetMesh()->PlayAnimation(TraversalProfile->Vault,false);
        GetMesh()->SetPlayRate(TraversalProfile->Vault->GetPlayLength()/1.1f);
    }
    return true;
}
void ALZCharacter::TickVault(float DeltaSeconds)
{
    if(IsRunInactive()) { EndVault(); return; }
    VaultElapsed+=DeltaSeconds;
    const float T=FMath::Clamp(VaultElapsed/1.1f,0.f,1.f);
    FirstPersonCamera->SetRelativeLocation(FVector(-10,0,64-85*FMath::Sin(T*PI)));
    auto Ease=[](float V){return V*V*(3-2*V);};
    FVector Target;
    if(T<.3f) Target=FMath::Lerp(VaultStart,VaultLift,Ease(T/.3f));
    else if(T<.75f) Target=FMath::Lerp(VaultLift,VaultAcross,Ease((T-.3f)/.45f));
    else Target=FMath::Lerp(VaultAcross,VaultEnd,Ease((T-.75f)/.25f));
    FHitResult Hit; SetActorLocation(Target,true,&Hit);
    if(Hit.bBlockingHit || T>=1) EndVault();
}
void ALZCharacter::EndVault()
{
    bTraversing=false; GetMesh()->SetVisibility(false);
    FirstPersonCamera->SetRelativeLocation(FVector(-10,0,64));
    GetMesh()->SetOwnerNoSee(false);
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    UpdateWeaponVisibility();
}


bool ALZCharacter::TryClimbRoute(FVector Lift, FVector Across, FVector End)
{
    if(bTraversing || bInventoryOpen || IsRunInactive()) return false;
    const bool WasCrouched=bIsCrouched;
    bCrouchToggled=true;
    GetCharacterMovement()->bWantsToCrouch=true;
    GetCharacterMovement()->Crouch(false);
    const FCollisionShape Shape=FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(),GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    FCollisionQueryParams Q(SCENE_QUERY_STAT(VentClimb),false,this);
    FHitResult Hit;
    auto Clear=[&](FVector A,FVector B){return !GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Pawn,Shape,Q);};
    if(!Clear(GetActorLocation(),Lift) || !Clear(Lift,Across) || !Clear(Across,End) || GetWorld()->OverlapBlockingTestByChannel(End,FQuat::Identity,ECC_Pawn,Shape,Q))
    {if(!WasCrouched){bCrouchToggled=false;GetCharacterMovement()->bWantsToCrouch=false;GetCharacterMovement()->UnCrouch(false);}return false;}
    CancelWeaponActions();VaultStart=GetActorLocation();VaultLift=Lift;VaultAcross=Across;VaultEnd=End;VaultElapsed=0;bTraversing=true;
    ConsumeMovementInputVector();GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();UpdateWeaponVisibility();
    return true;
}
