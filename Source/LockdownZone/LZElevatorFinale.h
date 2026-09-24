#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZInteractable.h"
#include "LZElevatorFinale.generated.h"
UENUM() enum class ELZFinaleStage:uint8 { Dormant,Jammed,Align,Force,Enter,Turn,Close,Pinch,Sever,Descending,Arrival,Done,Failed };
UCLASS(Config=Game,DefaultConfig)
class LOCKDOWNZONE_API ULZElevatorFinaleSettings:public UObject {
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere) float ForceSeconds=3.6f;
 UPROPERTY(Config,EditAnywhere) float AlignSeconds=.55f;
 UPROPERTY(Config,EditAnywhere) float EntrySeconds=1.6f;
 UPROPERTY(Config,EditAnywhere) float TurnSeconds=.85f;
 UPROPERTY(Config,EditAnywhere) float DescentSeconds=4.f;
};
UCLASS()
class LOCKDOWNZONE_API ALZElevatorSeam:public ALZInteractable {
 GENERATED_BODY()
public:
 ALZElevatorSeam();
 UPROPERTY() class ALZElevatorFinale* Finale;
 virtual void Interact(class ALZCharacter* P) override;
 virtual FString GetInteractionPrompt(const ALZCharacter* P) const override;
};
UCLASS()
class LOCKDOWNZONE_API ALZElevatorFinale:public AActor {
 GENERATED_BODY()
public:
 ALZElevatorFinale();
 void Initialize(class ALZChapter* InChapter,AActor* OldDoor);
 void Jam();
 bool Start(class ALZCharacter* P);
 FString Prompt() const;
 virtual void Tick(float Delta) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 ELZFinaleStage Stage=ELZFinaleStage::Dormant;
private:
 UPROPERTY() class ALZChapter* Chapter;
 UPROPERTY() class ALZGameMode* GM;
 UPROPERTY() class ALZCharacter* Player;
 UPROPERTY() ALZElevatorSeam* Seam;
 UPROPERTY() class UStaticMeshComponent* LeftDoor;
 UPROPERTY() class UStaticMeshComponent* RightDoor;
 UPROPERTY() class USceneComponent* Bar;
 UPROPERTY() class UStaticMeshComponent* Arm;
 UPROPERTY() class UStaticMeshComponent* Hand;
 UPROPERTY() class UStaticMeshComponent* CutArm;
 UPROPERTY() class UTextRenderComponent* CabinDisplay;
 UPROPERTY() TArray<class UStaticMeshComponent*> Fingers;
 UPROPERTY() class APostProcessVolume* Look;
 UPROPERTY() class UPackage* GaragePackage;
 UPROPERTY() TArray<AActor*> OldActors;
 UPROPERTY() TArray<class ALZEnemy*> HeldEnemies;
 TArray<FString> Checks;
 TArray<FString> Frames;
 float Time=0,Force=0,Opening=0,NextAudio=0,RecordNext=0,RecordStart=0,QAStarted=0,PauseValue=0;
 int32 Starts=0,Frame=0,SoundBeat=0;
 bool bLoaded=false,bLoadFailed=false,bTransferred=false,bTest=false,bQA=false,bPrepared=false,bPauseChecked=false,bRepeatChecked=false,bQAForce=false,bAudioStopped=false,bPauseTest=false;
 FVector AlignStart,EntryStart;
 FRotator AlignRotation;
 class UStaticMeshComponent* Part(FName Name,FVector Position,FVector Size,FLinearColor Color);
 void SetOpening(float Value);
 void Change(ELZFinaleStage Value);
 bool Held() const;
 bool MovePlayer(FVector Destination);
 void ProtectEnemies(bool Active);
 void BeginGarageLoad();
 void TransferInCabin();
 void Abort(const TCHAR* Reason);
 void TogglePause();
 void QA(float Delta);
 void Check(bool Good,const TCHAR* What);
};
