#include "LZObjective.h"

#include "Components/StaticMeshComponent.h"
#include "LZCharacter.h"
#include "LZGameMode.h"

ALZObjective::ALZObjective()
{
    Mesh->SetWorldScale3D(FVector(0.6f, 0.6f, 0.8f));
    SetLabel(TEXT("WATER CONTROL MODULE"), FColor(80, 220, 255));
}

void ALZObjective::Interact(ALZCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (!GameMode->IsPuzzleComplete())
        {
            return;
        }
        GameMode->CompleteObjective();
        Destroy();
    }
}

FString ALZObjective::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (!GameMode->IsPuzzleComplete())
        {
            return TEXT("控制模块被隔离：先恢复三个系统节点");
        }
    }
    return TEXT("[E] 取得净水控制模块（解锁撤离）");
}
