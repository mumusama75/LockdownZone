#pragma once
#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZIntercom.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZIntercom : public ALZInteractable
{
 GENERATED_BODY()
public:
 ALZIntercom();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 FString AudibleSubtitle(const ALZCharacter* Player) const;
 bool IsPowered() const {return bPowered;}
 bool IsPlaying() const;
 int32 EmissionCount=0;
 float LastEmissionAt=-100,NextCallAt=0;
 UPROPERTY(EditAnywhere) float CallInterval=14,SoundRadius=3800;
private:
 UPROPERTY() class UAudioComponent* Speaker;
 UPROPERTY() class UStaticMeshComponent* Indicator;
 bool bPowered=true;
};
UCLASS()
class LOCKDOWNZONE_API ALZAccessCard : public ALZInteractable
{
 GENERATED_BODY()
public:
 ALZAccessCard();
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 UPROPERTY(EditAnywhere) FName Permission=TEXT("AdminAccess");
};
