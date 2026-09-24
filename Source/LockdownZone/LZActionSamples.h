#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LZActionSamples.generated.h"
UCLASS(Config=Game,DefaultConfig)
class LOCKDOWNZONE_API ULZActionSampleSettings:public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere) bool EnableInChapter=false;
 UPROPERTY(Config,EditAnywhere) float PickupSeconds=1.15f;
};
namespace LZActionSamples
{
 bool IsLab(const UObject* Context);
 bool Enabled(const UObject* Context);
}
