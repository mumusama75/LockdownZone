#pragma once
#include "CoreMinimal.h"
class AActor;
class USceneComponent;
namespace LZCrowbarVisual
{
 // Shared 74 cm steel bar and hooked claw, with no gameplay collision.
 USceneComponent* Build(AActor* Owner,USceneComponent* Parent);
 FString InteractionKey();
}
