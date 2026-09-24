#pragma once
#include "CoreMinimal.h"
#include "LZInteractable.h"
#include "LZPickupTruck.generated.h"
UCLASS(Blueprintable)
class ALZPickupTruck:public ALZInteractable {
 GENERATED_BODY()
public:
 ALZPickupTruck();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 virtual void Interact(class ALZCharacter* P) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* P) const override;
 UFUNCTION(BlueprintCallable,Category="Garage") bool Enter(class ALZCharacter* P);
 UFUNCTION(BlueprintCallable,Category="Garage") bool Exit();
 bool FindExit(FVector& Place) const;
 UFUNCTION(BlueprintPure,Category="Garage") float Speed() const;
 float DoorAngle=45;
 int GroundWheels=0,Impacts=0;
 UPROPERTY(BlueprintReadOnly,Category="Garage") bool Driving=false;
 UPROPERTY() class ALZGarageSlice* Slice;
 UPROPERTY() class UBoxComponent* Chassis;
 UPROPERTY() class USceneComponent* DoorHinge;
 UPROPERTY() class ACameraActor* DriveCamera;
 UPROPERTY() class ALZCharacter* Driver;
 void SetTestInput(float Forward,float Turn,bool Brake=false);
 void LookYaw(float Value);
 void LookPitch(float Value);
 void ClearTestInput(){TestInput=false;}
 FVector DoorPosition() const;
private:
 UPROPERTY() class UPointLightComponent* CabinLight;
 UPROPERTY() TArray<class UStaticMeshComponent*> Wheels;
 float LookYawOffset=0,LookPitchOffset=0;
 void UpdateDriverLook();
 float Throttle=0,Steer=0,SteerActual=0,CloseAt=0,NextNoise=0;
 bool Handbrake=false,TestInput=false;
 TMap<TWeakObjectPtr<AActor>,float> LastImpact;
 FVector Previous;
 void Forward(float Value){if(!TestInput)Throttle=Value;}
 void Turn(float Value){if(!TestInput)Steer=Value;}
 void BrakeOn(){Handbrake=true;}
 void BrakeOff(){Handbrake=false;}
 void ExitInput(){Exit();}
 void RetryInput();
 void DrivePhysics(float Delta);
 void SweepVictims(float Delta);
};
