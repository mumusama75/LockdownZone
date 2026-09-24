#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LZMotionAnimInstance.generated.h"
UCLASS(Transient)
class LOCKDOWNZONE_API ULZMotionAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
