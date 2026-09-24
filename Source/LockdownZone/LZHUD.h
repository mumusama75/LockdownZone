#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LZHUD.generated.h"

class ALZCharacter;
class ALZGameMode;
enum class ELZInventoryItemType : uint8;

UCLASS()
class LOCKDOWNZONE_API ALZHUD : public AHUD
{
    GENERATED_BODY()
    FVector2D LastInventoryMouse = FVector2D(-1,-1);

public:
    virtual void DrawHUD() override;

private:
    void DrawInventory(const ALZCharacter* Character, const ALZGameMode* GameMode);
    void DrawRunResult(const ALZCharacter* Character, const ALZGameMode* GameMode);
    void DrawInventoryIcon(ELZInventoryItemType Type, float X, float Y, float Scale, const FLinearColor& Color);
    void DrawShadowedText(const FString& Text, float X, float Y, const FLinearColor& Color,
        float Scale = 1.0f, bool bCentered = false);

    bool bMouseDragging = false;
    FVector2D DragStartMousePos = FVector2D::ZeroVector;
};
