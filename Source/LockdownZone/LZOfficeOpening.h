#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LZOfficeOpening.generated.h"
UCLASS()
class LOCKDOWNZONE_API ALZOfficeOpening:public AActor
{
 GENERATED_BODY()
public:
 ALZOfficeOpening();
 void Setup(class ALZChapter* Chapter,class ALZChapterNode* Door,class ALZChapterNode* Tool,AActor* DoorLeaf);
 void Interact(class ALZCharacter* Player);
 virtual void Tick(float Delta) override;
 bool IsPrying() const {return bPrying;}
 int32 SuccessfulPrys=0;
private:
 UPROPERTY() class ALZChapter* C;
 UPROPERTY() class ALZChapterNode* Door;
 UPROPERTY() class AStaticMeshActor* Leaf;
 UPROPERTY() class ALZCharacter* Operator;
 UPROPERTY() class USceneComponent* PryBar;
 FVector Closed,Hinge;
 FRotator ClosedRotation;
 float Started=0,NextCheck=0,RattleUntil=0;
 bool bPrying=false;
 void Pose(float Degrees);
};
