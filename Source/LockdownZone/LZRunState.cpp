#include "LZRunState.h"
void ULZRunState::Capture(ALZCharacter* P){
 if(!P)return;
 P->CancelHeldItem();Items=P->InventoryEntries;Health=P->Health;Ammo=P->AmmoInMagazine;NextId=P->NextItemId;
 Crowbar=P->bOwnsCrowbar;Melee=P->bHasMeleeWeapon;Gun=P->bHasFirearm;Torch=P->bHasFlashlight;Selected=P->SelectedWeapon;HasTransfer=true;
}
bool ULZRunState::Restore(ALZCharacter* P){
 if(!HasTransfer || !P)return false;
 P->InventoryEntries=Items;P->Health=FMath::Max(1.f,Health);P->AmmoInMagazine=Ammo;P->NextItemId=NextId;
 P->bOwnsCrowbar=Crowbar;P->bHasMeleeWeapon=Melee;P->bHasFirearm=Gun;P->bHasFlashlight=Torch;P->bFlashlightOn=false;P->SelectedWeapon=Selected;P->UpdateWeaponVisibility();return true;
}
