#include "LZPuzzleTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "LZCharacter.h"
#include "LZGameMode.h"

ALZPuzzleTerminal::ALZPuzzleTerminal()
{
    Mesh->SetRelativeScale3D(FVector(0.55f, 0.42f, 0.9f));
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
    case EPuzzleNode::Generator: return TEXT("动力节点");
    case EPuzzleNode::Cooling: return TEXT("冷却节点");
    case EPuzzleNode::Purifier: return TEXT("净化节点");
    default: return TEXT("损坏的维修记录");
    }
}

void ALZPuzzleTerminal::RefreshState()
{
    if (Node == EPuzzleNode::Clue)
    {
        SetLabel(TEXT("SOP-17 / DAMAGED LOG"), FColor(150, 200, 255));
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
        return TEXT("[E] 查看损坏的维修记录");
    }
    if (const ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>())
    {
        if (GameMode->IsPuzzleNodeActivated(Node))
        {
            return FString::Printf(TEXT("%s：已启动"), *GetNodeName());
        }
    }
    return FString::Printf(TEXT("[E] 启动%s"), *GetNodeName());
}
