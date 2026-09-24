#pragma once
#include "CoreMinimal.h"
class UAudioComponent;
namespace LZAcoustics
{
 // Emits perception only if the matching audible resource exists and playback starts.
 bool Emit(UObject* Context,FVector Position,float Radius,FName Clip,FName Category,float Volume=1,UAudioComponent* Speaker=nullptr);
}
