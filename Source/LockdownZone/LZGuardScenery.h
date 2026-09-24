#pragma once
#include "CoreMinimal.h"
class ALZGameMode;
namespace LZGuardScenery { struct FGuard {AActor* Body; FVector BadgePosition;}; FGuard Spawn(ALZGameMode* GM,FVector Center,FName Tag); }
