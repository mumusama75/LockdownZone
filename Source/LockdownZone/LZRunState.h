#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LZCharacter.h"
#include "LZRunState.generated.h"
// Session-only chapter-entry snapshot, also restored on explicit garage retry.
UCLASS()
class ULZRunState : public UGameInstance {
 GENERATED_BODY()
public:
 bool HasTransfer=false;
 void Capture(ALZCharacter* P);
 bool Restore(ALZCharacter* P);
private:
 UPROPERTY() TArray<FLZInventoryEntry> Items;
 float Health=100;
 int32 Ammo=0,NextId=1;
 bool Crowbar=false,Melee=false,Gun=false,Torch=false;
 EPlayerWeapon Selected=EPlayerWeapon::None;
};
