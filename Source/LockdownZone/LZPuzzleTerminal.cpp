#include "LZPuzzleTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "LZCharacter.h"
#include "LZGameMode.h"

ALZPuzzleTerminal::ALZPuzzleTerminal()
{
    Mesh->SetRelativeScale3D(FVector(0.35f, 0.45f, 0.65f));
}

void ALZPuzzleTerminal::BeginPlay()
{
    Super::BeginPlay();
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        GameMode->RegisterPuzzleTerminal(this);
    }
    RefreshState();
}

void ALZPuzzleTerminal::Configure(EPuzzleNode NewNode)
{
    Node = NewNode;
    RefreshState();
}

FString ALZPuzzleTerminal::GetNodeName() const
{
    switch (Node)
    {
    case EPuzzleNode::Generator: return TEXT("04配电间·动力节点");
    case EPuzzleNode::Cooling: return TEXT("03机房·冷却循环");
    case EPuzzleNode::Purifier: return TEXT("中央走廊·环境净化");
    default: return TEXT("SOP-17 运维备忘便签");
    }
}

void ALZPuzzleTerminal::RefreshState()
{
    if (Node == EPuzzleNode::Clue)
    {
        SetLabel(TEXT("SOP-17 / FACILITY LOG"), FColor(100, 200, 255));
        if (Glow)
        {
            Glow->SetIntensity(25.0f);
            Glow->SetLightColor(FLinearColor(0.35f, 0.75f, 1.0f));
        }
        return;
    }

    bool bActive = false;
    if (const ALZGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALZGameMode>() : nullptr)
    {
        bActive = GameMode->IsPuzzleNodeActivated(Node);
    }
    const FString LabelText = FString::Printf(TEXT("%s / %s"),
        Node == EPuzzleNode::Generator ? TEXT("POWER") : Node == EPuzzleNode::Cooling ? TEXT("COOLANT") : TEXT("PURIFIER"),
        bActive ? TEXT("ONLINE") : TEXT("OFFLINE"));
    SetLabel(LabelText, bActive ? FColor(70, 255, 120) : FColor(255, 130, 50));
    if (Glow)
    {
        Glow->SetIntensity(bActive ? 35.0f : 20.0f);
        Glow->SetLightColor(bActive ? FLinearColor(0.2f, 1.0f, 0.4f) : FLinearColor(1.0f, 0.55f, 0.15f));
    }
}

void ALZPuzzleTerminal::Interact(ALZCharacter* Character)
{
    if (!Character)
    {
        return;
    }
    if (ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (Node == EPuzzleNode::Clue)
        {
            GameMode->ShowPuzzleClue();
        }
        else
        {
            GameMode->TryActivatePuzzleNode(Node);
        }
        RefreshState();
    }
}

FString ALZPuzzleTerminal::GetInteractionPrompt(const ALZCharacter* Character) const
{
    if (Node == EPuzzleNode::Clue)
    {
        return TEXT("[E] 查看 SOP-17 运维备忘便签");
    }
    if (const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (GameMode->IsPuzzleNodeActivated(Node))
        {
            return FString::Printf(TEXT("%s：已并网运行"), *GetNodeName());
        }
    }
    return FString::Printf(TEXT("[E] 启动%s"), *GetNodeName());
}
