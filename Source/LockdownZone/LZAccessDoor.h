#pragma once
#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZAccessDoor.generated.h"
UENUM() enum class ELZAccessDoorState:uint8 {Closed,Opening,Open,Closing};
UCLASS()
class LOCKDOWNZONE_API ALZAccessDoor : public ALZInteractable
{
 GENERATED_BODY()
public:
 ALZAccessDoor();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 bool IsOpen() const {return State==ELZAccessDoorState::Open;}
 bool IsClosed() const {return State==ELZAccessDoorState::Closed;}
 bool IsOccupied() const;
 bool SoundApproach(FVector Listener,FVector Sound,FVector& Approach) const;
 void Knock(FVector From);
 UPROPERTY(EditAnywhere) FName RequiredAccess=TEXT("AdminAccess");
 UPROPERTY(EditAnywhere) float HoldSeconds=7,ClearDelay=1.2f,TravelSeconds=1.1f,SafetyDepth=210;
 UPROPERTY(VisibleAnywhere) ELZAccessDoorState State=ELZAccessDoorState::Closed;
 float GetOpenFraction() const {return Fraction;}
 float GetOpenedAt() const {return OpenedAt;}
 int32 KnockCount=0;
private:
 UPROPERTY() class USceneComponent* LeftLeaf;
 UPROPERTY() class USceneComponent* RightLeaf;
 UPROPERTY() class UBoxComponent* Barrier;
 float Fraction=0,OpenedAt=0,ClearSince=-1,NextUse=0,NextKnock=0;
 void Open();
 void Block(bool Enabled);
};
