#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LZMotionProfile.generated.h"
class UBlendSpace;
class UAnimSequence;
class USkeletalMesh;
UCLASS(BlueprintType)
class LOCKDOWNZONE_API ULZMotionProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SourceLabel = TEXT("UE5 template fallback - GASP pending");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) USkeletalMesh* Mesh = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UBlendSpace* Locomotion = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* Stagger = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* Knockdown = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* GetUp = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* Death = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* Vault = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) UAnimSequence* Attack = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector MeshOffset = FVector(0,0,-82);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator MeshRotation = FRotator(0,-90,0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PhysicsRoot = TEXT("pelvis");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPreserveMaterials = false;
    UFUNCTION(BlueprintCallable) void BuildCustomLocomotion(UAnimSequence* Idle, UAnimSequence* Walk, UAnimSequence* Run);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float KnockdownSeconds = 1.8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float GetUpSeconds = 1.0f;
    UFUNCTION(BlueprintCallable) void BuildLocomotionSamples();
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSpeedOnXAxis = false;
    void InitializeTemplateFallback();
};
