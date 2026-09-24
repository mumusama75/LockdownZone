#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZInteractable.h"
#include "LZChapter.generated.h"
UENUM() enum class ELZChapterNode:uint8 { Crowbar,Desk,EmptyDesk,Cabinet,DoorLock,RecordsDoor,ArchiveDoor,Fuse=8,Breaker,Elevator,Throwable,Flashlight,Medical,Pistol,Ammo,Vehicle,DoorB,DoorC,DoorD };
UENUM() enum class ELZElevatorState:uint8 { Offline,Ready,Arriving,Jammed,Open,Descending,Basement,Escaped,QTE,AutoEnter,Closing };
UCLASS()
class LOCKDOWNZONE_API ALZChapterNode:public ALZInteractable
{
 GENERATED_BODY()
public:
 void Configure(ELZChapterNode Kind,FVector Size,FColor Color);
 virtual void Interact(ALZCharacter* Player) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* Player) const override;
 virtual float TakeDamage(float Amount, const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
 UPROPERTY() ELZChapterNode Kind;
 UPROPERTY() class ALZChapter* Chapter;
 int32 Hits=0;
 bool bUsed=false;
 bool bOpen=false;
 FVector ClosedPosition;
 void ToggleDoor(bool bVertical=false);
 void SetGlow(float Value);
};
UCLASS()
class LOCKDOWNZONE_API ALZChapter:public AActor
{
 GENERATED_BODY()
public:
 friend class ALZGarage;
 UPROPERTY() class ALZElevatorFinale* Finale;
 UPROPERTY() class ALZVentNetwork* VentNetwork;
 UPROPERTY() class ALZOfficeOpening* Opening;
 UPROPERTY() class ALZGarage* Garage;
 UPROPERTY() class ALZAccessDoor* AdminDoor;
 UPROPERTY() class ALZIntercom* AdminRadio;
 UPROPERTY() class ALZAccessCard* AdminCard;
 UPROPERTY() TSet<FName> AccessPermissions;
 bool HasAccess(FName Id) const {return AccessPermissions.Contains(Id);}
 void GrantAccess(FName Id){AccessPermissions.Add(Id);}
 void ShowFeedback(const FString& Message){Feedback=Message;FeedbackUntil=GetWorld()->GetTimeSeconds()+2.8f;}
 UPROPERTY() class ALZFlashlightPickup* ArchiveFlashlight;
 ALZChapter();
 virtual void BeginPlay() override;
 virtual void Tick(float Delta) override;
 void Use(ALZChapterNode* Node,ALZCharacter* Player);
 void Strike(ALZChapterNode* Node,ALZCharacter* Player);
 void Noise(FVector Location,float Radius,FName Sound=TEXT("Clang"),FName Category=NAME_None);
 bool QTE(bool bE,ALZCharacter* Player);
 bool IsUnlocked() const {return bDoorBreached;}
 bool IsJammed() const {return ElevatorState==ELZElevatorState::Jammed;}
 bool HasCrowbar() const {return bCrowbar;}
 FString GetFeedback() const;
 FString ElevatorPrompt() const;
 ELZElevatorState GetElevatorState() const {return ElevatorState;}
 bool bCrowbar=false,bKey=false,bAxe=false,bDoorBreached=false,bFuse=false,bFuseInstalled=false;
 bool bBOpen=false,bCUnlocked=false;
 int32 QTESteps=0;
 UPROPERTY() ELZElevatorState ElevatorState=ELZElevatorState::Offline;
 UPROPERTY() TArray<ALZChapterNode*> Nodes;
 ALZChapterNode* FindNode(ELZChapterNode Kind) const;
private:
 UPROPERTY() class ALZGameMode* GM;
 UPROPERTY() AActor* RecordsGate;
 UPROPERTY() AActor* ArchiveGate;
 UPROPERTY() AActor* ElevatorGate;
 UPROPERTY() ALZChapterNode* Panel;
 UPROPERTY() ALZChapterNode* Lift;
 UPROPERTY() class UPointLightComponent* Spark;
 UPROPERTY() class USoundAttenuation* Attenuation;
 UPROPERTY() TArray<AActor*> Sparks;
 UPROPERTY() TArray<AActor*> VentGuideSparks;
 UPROPERTY() class UPointLightComponent* VentGuideLight;
 UPROPERTY() class UTextRenderComponent* FloorDisplay;
 float StateTime=0,NextFootstep=0,NextSpark=0,NextQTE=0,NextUse=0,FeedbackUntil=0;
 FString Feedback;
 FVector LastStepPosition=FVector::ZeroVector;
 float StepDistance=0;
 FVector DoorSound=FVector(-2480,-610,110);
 FVector LiftLocation=FVector(3480,0,110);
 void Say(const TCHAR* Text);
 ALZChapterNode* Add(ELZChapterNode Kind,FVector Pos,FVector Size,FColor Color);
 void Build();
 void BuildDarkPantry();
 void BuildAdminStealthRoute();
 void SetElevator(ELZElevatorState State);
};
