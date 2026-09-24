#include "LZHUD.h"
#include "LZGarageSlice.h"
#include "LZPickupTruck.h"
#include "LZChapter.h"
#include "LZGarage.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "LZCharacter.h"
#include "LZInventoryTypes.h"
#include "LZGameMode.h"
#include "LZInteractable.h"
#include "LZEnemy.h"
#include "GameFramework/PlayerController.h"

void ALZHUD::DrawShadowedText(const FString& Text, float X, float Y, const FLinearColor& Color,
    float Scale, bool bCentered)
{
    if (bCentered)
    {
        float Width = 0.0f;
        float Height = 0.0f;
        GetTextSize(Text, Width, Height, GEngine->GetMediumFont(), Scale);
        X -= Width * 0.5f;
    }
    DrawText(Text, FLinearColor(0.0f, 0.0f, 0.0f, 0.85f), X + 2.0f, Y + 2.0f,
        GEngine->GetMediumFont(), Scale, false);
    DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale, false);
}

void ALZHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas)
    {
        return;
    }

    ALZCharacter* Character = Cast<ALZCharacter>(GetOwningPawn());
    ALZGameMode* GameMode = GetWorld()->GetAuthGameMode<ALZGameMode>();
    if (!Character || !GameMode)
    {
        return;
    }

    // Settlement and inventory own the screen; no target raycasts, prompt or crosshair underneath.
    if (GameMode->IsRunOver())
    {
        DrawRunResult(Character, GameMode);
        return;
    }
    if (Character->IsInventoryOpen())
    {
        DrawInventory(Character, GameMode);
        return;
    }

    if(auto* G=GameMode->GarageSlice; G && G->Truck && G->Truck->Driving)
    {
        DrawRect(FLinearColor(0,0,0,.7f),20,Canvas->ClipY-100,Canvas->ClipX-40,80);
        DrawShadowedText(G->Hint(),40,Canvas->ClipY-65,FLinearColor::White,.85f);
        return;
    }
    if (auto* C = GameMode->GetChapter(); C && C->Garage && C->Garage->State >= ELZGarageState::Driving)
    {
        const auto* G = C->Garage;
        if (G->State == ELZGarageState::CityReveal)
        {
            DrawRect(FLinearColor::Black,0,0,Canvas->ClipX,44);
            DrawRect(FLinearColor::Black,0,Canvas->ClipY-60,Canvas->ClipX,60);
            DrawShadowedText(TEXT("破碎城区 · 零号大厦之外"),Canvas->ClipX*.5f,Canvas->ClipY-43,FLinearColor(.8f,.85f,.9f),1.0f,true);
        }
        else
        {
            DrawShadowedText(*G->Hint(),Canvas->ClipX*.5f,Canvas->ClipY-52,FLinearColor::White,.85f,true);
            DrawShadowedText(FString::Printf(TEXT("%02.0f km/h"),FMath::Abs(G->Speed)*.036f),40,Canvas->ClipY-62,FLinearColor(.8f,.9f,.8f),1.3f);
        }
        return;
    }
    LastInventoryMouse = FVector2D(-1,-1);
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawRect(FLinearColor(0.008f, 0.016f, 0.02f, 0.65f), 22, 20, 250, 60);
    DrawRect(FLinearColor(0.24f, 0.70f, 0.73f, 0.9f), 22, 20, 3, 60);
    if((!GameMode->GetChapter() && !GameMode->GarageSlice) || !GameMode->GetStatusText().IsEmpty())
    DrawRect(FLinearColor(0.008f, 0.016f, 0.02f, 0.64f), Canvas->ClipX - 570, 20, 548, 82);
    DrawRect(FLinearColor(0.008f, 0.016f, 0.02f, 0.64f), 22, Canvas->ClipY - 130, 540, 114);
    DrawLine(CenterX - 8.0f, CenterY, CenterX + 8.0f, CenterY, FLinearColor::White, 1.5f);
    DrawLine(CenterX, CenterY - 8.0f, CenterX, CenterY + 8.0f, FLinearColor::White, 1.5f);

    // Show a reliable Chinese target readout even when world-space font atlases are limited.
    if (APlayerController* PC = GetOwningPlayerController())
    {
        FVector ViewLocation;
        FRotator ViewRotation;
        PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
        FHitResult TargetHit;
        FCollisionQueryParams TargetParams(SCENE_QUERY_STAT(HUDTargetTrace), false, Character);
        if (GetWorld()->LineTraceSingleByChannel(TargetHit, ViewLocation,
            ViewLocation + ViewRotation.Vector() * 8000.0f, ECC_Visibility, TargetParams))
        {
            if (ALZEnemy* Enemy = Cast<ALZEnemy>(TargetHit.GetActor()))
            {
                const FString EnemyInfo = FString::Printf(TEXT("%s  %.0f / %.0f"),
                    *Enemy->GetEnemyDisplayName(), Enemy->GetEnemyHealth(), Enemy->GetEnemyMaxHealth());
                float EnemyWidth = 0.0f;
                float EnemyHeight = 0.0f;
                GetTextSize(EnemyInfo, EnemyWidth, EnemyHeight, GEngine->GetMediumFont(), 0.9f);
                DrawShadowedText(EnemyInfo, CenterX - EnemyWidth * 0.5f, CenterY - 58.0f,
                    FLinearColor(1.0f, 0.28f, 0.18f), 0.9f);
            }
        }
    }

    DrawShadowedText(TEXT("封锁区 / 零号大厦"), 34.0f, 28.0f,
        FLinearColor(0.35f, 0.85f, 1.0f), 1.15f);
    DrawShadowedText((GameMode->GarageSlice || (GameMode->GetChapter() && GameMode->GetChapter()->Garage))?TEXT("B1  /  地下车库与城市出口"):TEXT("12F  /  隔离办公层"), 34, 58, FLinearColor(.62f,.70f,.70f), .70f);
    DrawShadowedText(FString::Printf(TEXT("生命    %03.0f"), Character->GetHealth()),
        34.0f, Canvas->ClipY - 115.0f, Character->GetHealth() < 30.0f ? FLinearColor::Red : FLinearColor::White, 1.15f);
    const FString WeaponLine = Character->GetSelectedWeapon() == EPlayerWeapon::Firearm
        ? FString::Printf(TEXT("%s   %02d / 17   备弹 %02d"), *Character->GetSelectedWeaponName(),
            Character->GetAmmoInMagazine(), Character->GetReserveAmmo())
        : FString::Printf(TEXT("武器    %s"), *Character->GetSelectedWeaponName());
    DrawShadowedText(WeaponLine, 34.0f, Canvas->ClipY - 82.0f,
        FLinearColor(1.0f, 0.8f, 0.25f), 1.1f);
    DrawShadowedText(GameMode->GetChapter()?(GameMode->GetChapter()->Garage?TEXT("寻找驶离车库的机会"):TEXT("保持安静")):FString::Printf(TEXT("已清除感染者  %d"), GameMode->GetEnemiesKilled()),
        34.0f, Canvas->ClipY - 49.0f, FLinearColor(0.75f, 0.9f, 0.75f), 1.0f);
    DrawShadowedText(GameMode->GetChapter()?TEXT("Ctrl 静步  右键格挡/推开  G投掷  F手电  B背包"):TEXT("Ctrl蹲下潜行  E交互  1/2切枪  R换弹  F手电  B背包"), Canvas->ClipX - 440, Canvas->ClipY - 37,
        FLinearColor(.64f,.72f,.74f), .68f);
    const FString FlashlightLine = !Character->HasFlashlight() ? TEXT("手电筒：未拾取")
        : Character->IsFlashlightOn() ? TEXT("手电筒：开启  [F] 关闭") : TEXT("手电筒：关闭  [F] 开启");
    DrawShadowedText(FlashlightLine, Canvas->ClipX - 360, Canvas->ClipY - 92,
        Character->IsFlashlightOn() ? FLinearColor(1.0f, .91f, .65f) : FLinearColor(.64f,.72f,.74f), .82f);
    DrawShadowedText(FString::Printf(TEXT("背包 %d / %d  |  物资价值 %d"), Character->GetUsedBagSlots(),
        Character->GetMaxBagSlots(), Character->GetLootValue()), Canvas->ClipX - 360, Canvas->ClipY - 65,
        FLinearColor(.88f,.78f,.48f), .78f);

    if (Character->bIsCrouched)
    {
        const float StealthBoxW = 280.0f;
        const float StealthBoxH = 30.0f;
        const float StealthBoxX = CenterX - StealthBoxW * 0.5f;
        const float StealthBoxY = Canvas->ClipY - 170.0f;
        DrawRect(FLinearColor(0.005f, 0.02f, 0.015f, 0.85f), StealthBoxX, StealthBoxY, StealthBoxW, StealthBoxH);
        DrawRect(FLinearColor(0.22f, 0.94f, 0.58f, 0.95f), StealthBoxX, StealthBoxY, 3.0f, StealthBoxH);
        DrawShadowedText(GameMode->GetChapter()?TEXT("静步"):TEXT("▼ 潜行静默中 · 敌方侦测-55% · 掩体遮蔽"), CenterX, StealthBoxY + 6.0f,
            FLinearColor(0.22f, 0.94f, 0.58f), 0.82f, true);
    }

    if(GameMode->GetChapter())
    {
        DrawShadowedText(FString::Printf(TEXT("体力 %.0f  |  投掷物 %d%s"),Character->GetStamina(),Character->GetThrowableCount(),Character->IsBlocking()?TEXT("  格挡中"):TEXT("")),34,Canvas->ClipY-150,FLinearColor(.55f,.85f,.72f),.85f);
    }
    DrawShadowedText(GameMode->GetObjectiveText(), Canvas->ClipX - 550.0f, 32.0f,
        GameMode->IsObjectiveComplete() ? FLinearColor(0.3f, 1.0f, 0.45f) : FLinearColor(0.95f, 0.85f, 0.35f), 1.0f);
    DrawShadowedText(GameMode->GetStatusText(), Canvas->ClipX - 550.0f, 64.0f,
        FLinearColor(0.85f, 0.85f, 0.85f), 0.85f);

    if (Character->IsTraversing() || Character->CanVault())
    {
        DrawShadowedText(Character->IsFinaleLocked()?TEXT(""):Character->IsPrying()? TEXT("撬动门框中") : Character->IsTraversing()? TEXT("翻越中 · 无法射击") : TEXT("[空格] 翻越低隔断 · 会议室捷径"), CenterX, CenterY+82, FLinearColor(.35f,1,.8f), .9f, true);
    }
    if (!Character->IsTraversing())
    if (ALZInteractable* Target = Character->FindInteractable())
    {
        FString Prompt = Target->GetInteractionPrompt(Character);
        float Width = 0.0f;
        float Height = 0.0f;
        GetTextSize(Prompt, Width, Height, GEngine->GetMediumFont(), 1.0f);
        DrawShadowedText(Prompt, CenterX - Width * 0.5f, CenterY + 48.0f, FLinearColor::White, 1.0f);
    }
}

void ALZHUD::DrawRunResult(const ALZCharacter* Character, const ALZGameMode* GameMode)
{
    const float Scale = FMath::Min(Canvas->ClipX / 1280.0f, Canvas->ClipY / 720.0f);
    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.83f), 0, 0, Canvas->ClipX, Canvas->ClipY);
    const FString Result = GameMode->WasExtractionSuccessful() ? TEXT("撤离成功") : TEXT("行动失败");
    const FLinearColor ResultColor = GameMode->WasExtractionSuccessful()
        ? FLinearColor(0.3f, 1.0f, 0.45f) : FLinearColor(1.0f, 0.2f, 0.15f);
    DrawShadowedText(Result, CenterX, CenterY - 90 * Scale, ResultColor, 1.8f * Scale, true);
    const FString Summary = FString::Printf(TEXT("带出物资：$%d    用时：%s"),
        GameMode->WasExtractionSuccessful() ? Character->GetLootValue() : 0,
        *GameMode->GetElapsedTimeText());
    DrawShadowedText(Summary, CenterX, CenterY, FLinearColor::White, Scale, true);
    DrawShadowedText(TEXT("按 F5 重新开始行动"), CenterX, CenterY + 55 * Scale,
        FLinearColor(0.7f, 0.7f, 0.7f), 0.9f * Scale, true);
}

void ALZHUD::DrawInventoryIcon(ELZInventoryItemType Type, float X, float Y, float Scale, const FLinearColor& Color)
{
    auto Rect = [this, X, Y, Scale](float PX, float PY, float Width, float Height, const FLinearColor& Tint)
    {
        DrawRect(Tint, X + PX * Scale, Y + PY * Scale, Width * Scale, Height * Scale);
    };
    auto Line = [this, X, Y, Scale, &Color](float X1, float Y1, float X2, float Y2)
    {
        DrawLine(X + X1 * Scale, Y + Y1 * Scale, X + X2 * Scale, Y + Y2 * Scale, Color, FMath::Max(1.0f, Scale * 1.5f));
    };
    const FLinearColor Dark(0.025f, 0.04f, 0.05f);
    switch (Type)
    {
    case ELZInventoryItemType::Ammo:
        for (int32 Bullet = 0; Bullet < 3; ++Bullet)
        {
            Rect(9 + Bullet * 17, 16, 10, 26, Color);
            Rect(11 + Bullet * 17, 9, 6, 7, Color * 0.74f);
            Rect(8 + Bullet * 17, 42, 12, 3, Color);
        }
        break;
    case ELZInventoryItemType::Medical:
        Rect(7, 14, 50, 33, Color * 0.3f);
        Line(7, 14, 57, 14); Line(7, 47, 57, 47); Line(7, 14, 7, 47); Line(57, 14, 57, 47);
        Line(23, 14, 23, 8); Line(23, 8, 41, 8); Line(41, 8, 41, 14);
        Rect(27, 20, 10, 22, Color); Rect(21, 26, 22, 10, Color);
        break;
    case ELZInventoryItemType::Scrap:
        Rect(5, 8, 54, 40, Color * 0.24f);
        Rect(22, 19, 20, 19, Color); Rect(26, 23, 12, 11, Dark);
        for (int32 Pin = 0; Pin < 4; ++Pin)
        {
            Line(10, 19 + Pin * 6, 21, 19 + Pin * 6);
            Line(42, 19 + Pin * 6, 54, 19 + Pin * 6);
        }
        break;
    case ELZInventoryItemType::Rare:
        Rect(3, 11, 58, 34, Color * 0.26f);
        Line(3, 11, 61, 11); Line(3, 45, 61, 45); Line(3, 11, 3, 45); Line(61, 11, 61, 45);
        for (int32 Rack = 0; Rack < 3; ++Rack)
        {
            Rect(9, 16 + Rack * 9, 34, 4, Color * 0.75f);
            Rect(50, 16 + Rack * 9, 4, 4, Color);
        }
        break;
    case ELZInventoryItemType::Axe:
        Rect(28, 6, 6, 48, Color * 0.7f);
        Rect(14, 8, 20, 16, Color);
        Rect(10, 10, 6, 12, Color * 1.25f);
        Line(34, 12, 42, 16);
        Line(42, 16, 34, 20);
        break;
    case ELZInventoryItemType::Pistol:
        Rect(10, 14, 40, 12, Color);
        Rect(14, 18, 6, 4, Dark);
        Rect(14, 26, 14, 22, Color * 0.82f);
        Line(28, 26, 28, 33);
        Line(28, 33, 24, 33);
        break;
    case ELZInventoryItemType::Flashlight:
        Rect(8, 22, 34, 14, Color * 0.82f);
        Rect(42, 18, 12, 22, Color);
        Rect(54, 20, 4, 18, FLinearColor(1.0f, 0.95f, 0.5f));
        Line(16, 24, 16, 34);
        Line(24, 24, 24, 34);
        Line(32, 24, 32, 34);
        break;
    }
}

void ALZHUD::DrawInventory(const ALZCharacter* Character, const ALZGameMode* GameMode)
{
    // All coordinates use a 1280 x 720 design space, centred and scaled to either dimension.
    const float Scale = FMath::Min(Canvas->ClipX / 1280.0f, Canvas->ClipY / 720.0f);
    const float OriginX = (Canvas->ClipX - 1280.0f * Scale) * 0.5f;
    const float OriginY = (Canvas->ClipY - 720.0f * Scale) * 0.5f;

    auto Rect = [this, OriginX, OriginY, Scale](float X, float Y, float Width, float Height, const FLinearColor& Tint)
    {
        DrawRect(Tint, OriginX + X * Scale, OriginY + Y * Scale, Width * Scale, Height * Scale);
    };

    auto Frame = [this, OriginX, OriginY, Scale](float X, float Y, float Width, float Height,
        const FLinearColor& BorderColor, const FLinearColor& FillColor, float Thickness = 1.0f)
    {
        DrawRect(FillColor, OriginX + X * Scale, OriginY + Y * Scale, Width * Scale, Height * Scale);
        const float T = FMath::Max(1.0f, Thickness * Scale);
        DrawRect(BorderColor, OriginX + X * Scale, OriginY + Y * Scale, Width * Scale, T);
        DrawRect(BorderColor, OriginX + X * Scale, OriginY + (Y + Height) * Scale - T, Width * Scale, T);
        DrawRect(BorderColor, OriginX + X * Scale, OriginY + Y * Scale, T, Height * Scale);
        DrawRect(BorderColor, OriginX + (X + Width) * Scale - T, OriginY + Y * Scale, T, Height * Scale);
    };

    auto Text = [this, OriginX, OriginY, Scale](const FString& InText, float X, float Y,
        const FLinearColor& Tint, float TextScale = 1.0f, float WrapWidth = -1.0f)
    {
        if (WrapWidth <= 0.0f)
        {
            DrawShadowedText(InText, OriginX + X * Scale, OriginY + Y * Scale, Tint, TextScale * Scale);
            return;
        }

        const float MaxWidth = WrapWidth * Scale;
        TArray<FString> Words;
        InText.ParseIntoArray(Words, TEXT(" "), false);
        FString CurrentLine;
        float CurrentY = OriginY + Y * Scale;

        for (const FString& Word : Words)
        {
            const FString TestLine = CurrentLine.IsEmpty() ? Word : CurrentLine + TEXT(" ") + Word;
            float W = 0.0f, H = 0.0f;
            GetTextSize(TestLine, W, H, GEngine->GetMediumFont(), TextScale * Scale);
            if (W > MaxWidth && !CurrentLine.IsEmpty())
            {
                DrawShadowedText(CurrentLine, OriginX + X * Scale, CurrentY, Tint, TextScale * Scale);
                CurrentLine = Word;
                CurrentY += 20.0f * Scale;
            }
            else
            {
                CurrentLine = TestLine;
            }
        }
        if (!CurrentLine.IsEmpty())
        {
            DrawShadowedText(CurrentLine, OriginX + X * Scale, CurrentY, Tint, TextScale * Scale);
        }
    };

    // Quality & Theme Colors: Delta Force Hawk Ops tactical palette
    const FLinearColor Gold(1.0f, 0.78f, 0.22f);
    const FLinearColor Emerald(0.22f, 0.94f, 0.58f);
    const FLinearColor Purple(0.78f, 0.58f, 1.0f);
    const FLinearColor TechBlue(0.20f, 0.85f, 0.98f);
    const FLinearColor Cyan(0.35f, 0.85f, 1.0f);
    const FLinearColor DarkBG(0.008f, 0.014f, 0.020f, 0.97f);
    const FLinearColor PanelBG(0.016f, 0.026f, 0.034f, 0.95f);
    const FLinearColor CellBG(0.020f, 0.032f, 0.040f, 0.90f);
    const FLinearColor GridBorder(0.12f, 0.22f, 0.28f);
    const FLinearColor Red(0.95f, 0.25f, 0.22f);
    const FLinearColor Amber(0.96f, 0.65f, 0.22f);
    const FLinearColor White(0.92f, 0.95f, 0.96f);
    const FLinearColor Muted(0.55f, 0.64f, 0.68f);

    constexpr float CellSize = 72.0f;
    constexpr float CellGap = 2.0f;
    constexpr float GridStartX = 72.0f;
    constexpr float GridStartY = 152.0f;

    APlayerController* PC = GetOwningPlayerController();
    ALZCharacter* MutableChar = const_cast<ALZCharacter*>(Character);

    // Mouse click selection & drag-and-drop support
    int32 HoverCol = INDEX_NONE;
    int32 HoverRow = INDEX_NONE;
    if (PC)
    {
        float MouseX = 0.0f, MouseY = 0.0f;
        const bool bHasMousePos = PC->GetMousePosition(MouseX, MouseY) && (MouseX > 0.0f || MouseY > 0.0f);
        if (bHasMousePos)
        {
            const float CanvasMouseX = (MouseX - OriginX) / Scale;
            const float CanvasMouseY = (MouseY - OriginY) / Scale;
            const float TotalGridW = 6 * CellSize + 5 * CellGap;
            const float TotalGridH = 6 * CellSize + 5 * CellGap;

            const bool bInGrid = (CanvasMouseX >= GridStartX && CanvasMouseX < (GridStartX + TotalGridW) &&
                                  CanvasMouseY >= GridStartY && CanvasMouseY < (GridStartY + TotalGridH));

            if (bInGrid)
            {
                HoverCol = FMath::Clamp(FMath::FloorToInt32((CanvasMouseX - GridStartX) / (CellSize + CellGap)), 0, 5);
                HoverRow = FMath::Clamp(FMath::FloorToInt32((CanvasMouseY - GridStartY) / (CellSize + CellGap)), 0, 5);
                if ((LastInventoryMouse.X >= 0 && !FVector2D(MouseX, MouseY).Equals(LastInventoryMouse, 0.5f)) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
                    MutableChar->SetCursorPos(HoverCol, HoverRow);
            }

            LastInventoryMouse = FVector2D(MouseX, MouseY);
            const bool bLMBJustPressed = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
            const bool bLMBJustReleased = PC->WasInputKeyJustReleased(EKeys::LeftMouseButton);
            const bool bRMBJustPressed = PC->WasInputKeyJustPressed(EKeys::RightMouseButton);
            const bool bLMBIsDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);

            // Right-click anywhere cancels held item
            if (bRMBJustPressed && Character->HasHeldItem())
            {
                MutableChar->CancelHeldItem();
                bMouseDragging = false;
            }

            // Left Mouse Button Pressed:
            if (bLMBJustPressed)
            {
                const bool bClickRotateBtn = (CanvasMouseX >= 558.0f && CanvasMouseX <= 738.0f &&
                                              CanvasMouseY >= 426.0f && CanvasMouseY <= 460.0f);
                const bool bClickDiscardBtn = (CanvasMouseX >= 753.0f && CanvasMouseX <= 933.0f &&
                                               CanvasMouseY >= 426.0f && CanvasMouseY <= 460.0f);

                if (bClickRotateBtn)
                {
                    MutableChar->RotateInventoryItem();
                    bMouseDragging = false;
                }
                else if (bClickDiscardBtn)
                {
                    MutableChar->DiscardSelectedInventoryItem();
                    bMouseDragging = false;
                }
                else if (Character->HasHeldItem())
                {
                    if (bInGrid)
                    {
                        const bool bPlaced = MutableChar->PlaceHeldItemAtCell(HoverCol, HoverRow);
                        if (bPlaced)
                        {
                            bMouseDragging = false;
                        }
                        else
                        {
                            // In click-to-place mode, if clicked an invalid spot, initialize drag in case they hold and drag
                            bMouseDragging = true;
                            DragStartMousePos = FVector2D(MouseX, MouseY);
                        }
                    }
                    else
                    {
                        // Clicked outside grid while holding: cancel back to origin
                        MutableChar->CancelHeldItem();
                        bMouseDragging = false;
                    }
                }
                else
                {
                    if (bInGrid)
                    {
                        MutableChar->PickUpItemAtCell(HoverCol, HoverRow);
                        if (Character->HasHeldItem())
                        {
                            bMouseDragging = true;
                            DragStartMousePos = FVector2D(MouseX, MouseY);
                        }
                    }
                }
            }
            // Left Mouse Button Released (Drag-and-Drop Drop support):
            else if (bLMBJustReleased)
            {
                if (bMouseDragging && Character->HasHeldItem())
                {
                    const float DragDist = FVector2D::Distance(DragStartMousePos, FVector2D(MouseX, MouseY));
                    if (DragDist >= 8.0f)
                    {
                        int32 DropCol = HoverCol;
                        int32 DropRow = HoverRow;
                        if (DropCol == INDEX_NONE || DropRow == INDEX_NONE)
                        {
                            const float ClampedX = FMath::Clamp(CanvasMouseX, GridStartX, GridStartX + TotalGridW - 1.0f);
                            const float ClampedY = FMath::Clamp(CanvasMouseY, GridStartY, GridStartY + TotalGridH - 1.0f);
                            DropCol = FMath::Clamp(FMath::FloorToInt32((ClampedX - GridStartX) / (CellSize + CellGap)), 0, 5);
                            DropRow = FMath::Clamp(FMath::FloorToInt32((ClampedY - GridStartY) / (CellSize + CellGap)), 0, 5);
                        }
                        const bool bPlaced = MutableChar->PlaceHeldItemAtCell(DropCol, DropRow);
                        if (!bPlaced)
                        {
                            // If placement / swap failed on drag release:
                            // SNAP BACK TO ORIGINAL POSITION! Never get stuck!
                            MutableChar->CancelHeldItem();
                            MutableChar->SetInventoryStatusText(TEXT("位置受阻，物品已放回原位"));
                        }
                    }
                }
                bMouseDragging = false;
            }

            if (!bLMBIsDown && bMouseDragging)
            {
                bMouseDragging = false;
            }
        }

        if (PC->WasInputKeyJustPressed(EKeys::MiddleMouseButton))
        {
            MutableChar->RotateInventoryItem();
        }
    }

    // Outer Fullscreen Dark Blur Overlay
    DrawRect(FLinearColor(0.002f, 0.005f, 0.008f, 0.88f), 0, 0, Canvas->ClipX, Canvas->ClipY);

    // Master Tactical Chassis Frame
    Frame(44, 30, 1192, 660, GridBorder, DarkBG);
    Rect(44, 30, 4, 660, Cyan);

    // Corner decorative tactical reticles
    Rect(48, 32, 14, 2, Cyan);
    Rect(48, 32, 2, 14, Cyan);
    Rect(1222, 32, 14, 2, Cyan);
    Rect(1234, 32, 2, 14, Cyan);

    // HEADER SECTION
    Text(TEXT("战区战术背包 / TACTICAL BACKPACK  [6×6 战术矩阵]"), 72, 48, White, 1.30f);
    Text(TEXT("零号大厦 · 隔离办公层 | 三角洲战术网格 · 空间自由收纳系统 (36 格空间)"), 72, 82, Muted, 0.74f, 620);

    // Player Vitals in Header
    Text(FString::Printf(TEXT("干员体征: %03.0f / 100"), Character->GetHealth()), 660, 50,
        Character->GetHealth() < 30.0f ? Red : Emerald, 0.82f, 210);
    for (int32 Bar = 0; Bar < 8; ++Bar)
    {
        const bool bActive = (Character->GetHealth() / 100.0f) * 8 > Bar;
        Rect(660 + Bar * 24, 74, 18, 8, bActive ? (Character->GetHealth() < 30.0f ? Red : Emerald) : FLinearColor(0.06f, 0.10f, 0.12f));
    }

    // Extraction Valuation Chip & Warning Badge
    Frame(896, 44, 316, 46, Gold, FLinearColor(0.08f, 0.06f, 0.02f, 0.95f), 1.5f);
    Rect(896, 44, 4, 46, Gold);
    Text(TEXT("◆ 撤离资产估值"), 910, 51, Muted, 0.66f);
    Text(FString::Printf(TEXT("$%d"), Character->GetLootValue()), 910, 66, Gold, 1.25f);
    Frame(1054, 52, 146, 28, FLinearColor(0.48f, 0.18f, 0.12f), FLinearColor(0.12f, 0.04f, 0.03f));
    Text(TEXT("⚠️ 战区状态 · 危险未暂停"), 1062, 58, Amber, 0.68f, 130);

    Rect(72, 108, 1140, 1, GridBorder);

    // SECTION 1: 6x6 TACTICAL STORAGE GRID (LEFT)
    Text(TEXT("◆ 战术收纳矩阵 / 6×6 SPATIAL GRID"), 72, 118, TechBlue, 0.80f);
    Text(TEXT("容量: 6 × 6 共 36 格 | [鼠标/方向键] 选格  [E/空格/点击] 拿起/放下  [R] 旋转  [Del] 丢弃"),
        360, 118, Muted, 0.68f, 400);

    auto ItemColor = [&Gold, &Emerald, &TechBlue, &White, &Red, &Amber](ELZInventoryItemType Type) -> FLinearColor
    {
        switch (Type)
        {
        case ELZInventoryItemType::Axe: return Red;
        case ELZInventoryItemType::Pistol: return Amber;
        case ELZInventoryItemType::Flashlight: return Gold;
        case ELZInventoryItemType::Rare: return Gold;
        case ELZInventoryItemType::Medical: return Emerald;
        case ELZInventoryItemType::Ammo: return TechBlue;
        default: return White;
        }
    };
    auto ItemName = [](ELZInventoryItemType Type) -> FString
    {
        switch (Type)
        {
        case ELZInventoryItemType::Axe: return TEXT("应急重型消防斧");
        case ELZInventoryItemType::Crowbar: return TEXT("应急撬棍");
        case ELZInventoryItemType::Pistol: return TEXT("格洛克17 战术手枪");
        case ELZInventoryItemType::Flashlight: return TEXT("战术强光手电筒");
        case ELZInventoryItemType::Ammo: return TEXT("9x19mm 手枪备弹");
        case ELZInventoryItemType::Medical: return TEXT("便携急救医疗包");
        case ELZInventoryItemType::Rare: return TEXT("机密服务器备件");
        default: return TEXT("轻型工业废料");
        }
    };
    auto ItemValue = [](const FLZInventoryEntry& Item) -> int32
    {
        switch (Item.Type)
        {
        case ELZInventoryItemType::Axe: return 280;
        case ELZInventoryItemType::Pistol: return 350;
        case ELZInventoryItemType::Flashlight: return 80;
        case ELZInventoryItemType::Rare: return 500;
        case ELZInventoryItemType::Medical: return 60;
        case ELZInventoryItemType::Ammo: return Item.Quantity * 3;
        default: return 120;
        }
    };

    const int32 CursorCol = Character->GetCursorX();
    const int32 CursorRow = Character->GetCursorY();

    // Column markers (1..6)
    for (int32 Col = 0; Col < 6; ++Col)
    {
        const float MarkerX = GridStartX + Col * (CellSize + CellGap) + CellSize * 0.5f - 4;
        Text(FString::Printf(TEXT("%d"), Col + 1), MarkerX, GridStartY - 18.0f, Col == CursorCol ? Cyan : FLinearColor(0.35f, 0.45f, 0.50f), 0.70f);
    }
    // Row markers (A..F)
    for (int32 Row = 0; Row < 6; ++Row)
    {
        const float MarkerY = GridStartY + Row * (CellSize + CellGap) + CellSize * 0.5f - 8;
        Text(FString::Printf(TEXT("%c"), 'A' + Row), GridStartX - 20.0f, MarkerY, Row == CursorRow ? Cyan : FLinearColor(0.35f, 0.45f, 0.50f), 0.70f);
    }

    // 1. Draw 36 empty background cells
    for (int32 Row = 0; Row < 6; ++Row)
    {
        for (int32 Col = 0; Col < 6; ++Col)
        {
            const float X = GridStartX + Col * (CellSize + CellGap);
            const float Y = GridStartY + Row * (CellSize + CellGap);
            const bool bIsCursor = (Col == CursorCol && Row == CursorRow);

            Frame(X, Y, CellSize, CellSize, bIsCursor ? Cyan : GridBorder,
                bIsCursor ? FLinearColor(0.025f, 0.055f, 0.075f, 0.95f) : CellBG);

            // Subtle coordinate watermark in cell
            const FString Coord = FString::Printf(TEXT("%c%d"), 'A' + Row, Col + 1);
            Text(Coord, X + 5, Y + 5, bIsCursor ? Cyan : FLinearColor(0.18f, 0.26f, 0.30f), 0.52f);

            // Center subtle crosshair
            Rect(X + CellSize * 0.5f - 4, Y + CellSize * 0.5f - 1, 8, 2, FLinearColor(0.08f, 0.14f, 0.18f));
            Rect(X + CellSize * 0.5f - 1, Y + CellSize * 0.5f - 4, 2, 8, FLinearColor(0.08f, 0.14f, 0.18f));
        }
    }

    // 2. Draw placed items
    const int32 HeldItemId = Character->GetHeldItemId();
    for (const FLZInventoryEntry& Item : Character->GetInventoryEntries())
    {
        if (Item.ItemId == HeldItemId && HeldItemId != 0)
        {
            continue; // Currently being held/moved
        }

        const float X = GridStartX + Item.PosX * (CellSize + CellGap);
        const float Y = GridStartY + Item.PosY * (CellSize + CellGap);
        const float W = Item.Width * CellSize + (Item.Width - 1) * CellGap;
        const float H = Item.Height * CellSize + (Item.Height - 1) * CellGap;
        const bool bIsSelected = Item.CoversCell(CursorCol, CursorRow);
        const FLinearColor Tint = ItemColor(Item.Type);

        // Filled item background
        Frame(X, Y, W, H, bIsSelected ? Cyan : Tint * 0.8f,
            FLinearColor(Tint.R * 0.10f, Tint.G * 0.10f, Tint.B * 0.10f, 0.95f), bIsSelected ? 2.0f : 1.0f);
        // Top quality stripe
        Rect(X, Y, W, 3, Tint);

        // Center item icon
        const float IconScale = FMath::Min(1.0f, FMath::Min(W, H) / 55.0f);
        DrawInventoryIcon(Item.Type, OriginX + (X + W * 0.5f - 32 * IconScale) * Scale,
            OriginY + (Y + H * 0.5f - 24 * IconScale) * Scale, IconScale * Scale, Tint);

        // Item name & size label
        if (W >= 120.0f || H >= 120.0f)
        {
            Text(ItemName(Item.Type), X + 8, Y + 8, White, 0.76f, W - 16);
            Text(FString::Printf(TEXT("%d×%d 格"), Item.Width, Item.Height), X + 8, Y + H - 20, Muted, 0.65f);
        }
        else
        {
            Text(FString::Printf(TEXT("%d×%d"), Item.Width, Item.Height), X + 6, Y + H - 18, Muted, 0.60f);
        }

        // Ammo count or stack count
        if (Item.Type == ELZInventoryItemType::Ammo)
        {
            Frame(X + W - 38, Y + 6, 32, 16, TechBlue * 0.7f, FLinearColor(0.04f, 0.12f, 0.16f));
            Text(FString::Printf(TEXT("x%02d"), Item.Quantity), X + W - 34, Y + 7, TechBlue, 0.65f);
        }
        else if (Item.Quantity > 1)
        {
            Text(FString::Printf(TEXT("x%d"), Item.Quantity), X + W - 28, Y + 6, White, 0.65f);
        }

        // Corner brackets on selected item
        if (bIsSelected)
        {
            Rect(X + 1, Y + 1, 8, 2, Cyan);
            Rect(X + 1, Y + 1, 2, 8, Cyan);
            Rect(X + W - 9, Y + 1, 8, 2, Cyan);
            Rect(X + W - 3, Y + 1, 2, 8, Cyan);
            Rect(X + 1, Y + H - 3, 8, 2, Cyan);
            Rect(X + 1, Y + H - 9, 2, 8, Cyan);
            Rect(X + W - 9, Y + H - 3, 8, 2, Cyan);
            Rect(X + W - 3, Y + H - 9, 2, 8, Cyan);
        }
    }

    // 3. Draw held item footprint preview (Delta Force / Backpack Battles drag-and-drop feedback)
    if (Character->HasHeldItem())
    {
        const int32 HeldW = Character->GetHeldWidth();
        const int32 HeldH = Character->GetHeldHeight();
        int32 TargetX = 0, TargetY = 0;
        Character->GetHeldTargetPos(CursorCol, CursorRow, TargetX, TargetY);

        const TArray<int32> Overlaps = Character->GetOverlappingItemIds(TargetX, TargetY, HeldW, HeldH, HeldItemId);
        const FLZInventoryEntry* HeldEntry = Character->GetHeldItem();

        FLinearColor PreviewBorder = Emerald;
        FLinearColor PreviewFill = FLinearColor(0.15f, 0.85f, 0.45f, 0.38f);
        FString PlacementHint = TEXT("✓ 可放置 [点击/松开]");

        if (Overlaps.IsEmpty())
        {
            PreviewBorder = Emerald;
            PreviewFill = FLinearColor(0.15f, 0.85f, 0.45f, 0.38f);
            PlacementHint = TEXT("✓ 可放置 [点击/松开]");
        }
        else if (Overlaps.Num() == 1)
        {
            const FLZInventoryEntry* OtherEntry = Character->GetInventoryItemById(Overlaps[0]);
            if (HeldEntry && OtherEntry && HeldEntry->Type == ELZInventoryItemType::Ammo && OtherEntry->Type == ELZInventoryItemType::Ammo)
            {
                PreviewBorder = TechBlue;
                PreviewFill = FLinearColor(0.15f, 0.55f, 0.95f, 0.38f);
                PlacementHint = TEXT("◆ 合并备弹 [点击/松开]");
            }
            else if (Character->CanCleanSwapWith(TargetX, TargetY, Overlaps[0]))
            {
                PreviewBorder = Amber;
                PreviewFill = FLinearColor(0.95f, 0.65f, 0.15f, 0.38f);
                PlacementHint = TEXT("⇄ 交换物品 [点击/松开]");
            }
            else
            {
                PreviewBorder = Red;
                PreviewFill = FLinearColor(0.95f, 0.20f, 0.18f, 0.42f);
                PlacementHint = TEXT("✗ 位置受阻 [空间无法交换]");
            }
        }
        else
        {
            PreviewBorder = Red;
            PreviewFill = FLinearColor(0.95f, 0.20f, 0.18f, 0.42f);
            PlacementHint = TEXT("✗ 位置受阻 [多项阻挡]");
        }

        const float FPX = GridStartX + TargetX * (CellSize + CellGap);
        const float FPY = GridStartY + TargetY * (CellSize + CellGap);
        const float FPW = HeldW * CellSize + (HeldW - 1) * CellGap;
        const float FPH = HeldH * CellSize + (HeldH - 1) * CellGap;

        Frame(FPX, FPY, FPW, FPH, PreviewBorder, PreviewFill, 2.5f);
        Text(PlacementHint, FPX + 6, FPY + 6, PreviewBorder, 0.75f, FPW - 12);
        Text(FString::Printf(TEXT("规格: %d×%d [按R旋转]"), HeldW, HeldH), FPX + 6, FPY + FPH - 22, White, 0.70f);

        // Preview icon in floating footprint
        if (HeldEntry)
        {
            DrawInventoryIcon(HeldEntry->Type, OriginX + (FPX + FPW * 0.5f - 30) * Scale,
                OriginY + (FPY + FPH * 0.5f - 22) * Scale, Scale, PreviewBorder);
        }
    }

    // SECTION 2: TACTICAL ITEM INSPECTOR (RIGHT PANEL)
    const float PanelX = 538.0f;
    const float PanelWidth = 674.0f;
    const float PanelY = 130.0f;
    const float PanelHeight = 472.0f;

    Frame(PanelX, PanelY, PanelWidth, PanelHeight, GridBorder, PanelBG);
    Rect(PanelX, PanelY, 4, PanelHeight, Cyan);

    Text(TEXT("战术物资检视 / INSPECTION"), PanelX + 20, PanelY + 16, Cyan, 0.88f);
    Rect(PanelX + 20, PanelY + 40, PanelWidth - 40, 1, GridBorder);

    // Current selected entry or held entry
    const FLZInventoryEntry* ActiveEntry = Character->HasHeldItem()
        ? Character->GetHeldItem()
        : Character->GetInventoryItemAtCell(CursorCol, CursorRow);

    if (ActiveEntry)
    {
        const FLinearColor Tint = ItemColor(ActiveEntry->Type);
        const FString CategoryTag = ActiveEntry->Type == ELZInventoryItemType::Axe ? TEXT("【近战破拆武器 · 无限损耗】")
            : ActiveEntry->Type == ELZInventoryItemType::Pistol ? TEXT("【主武器 · 9mm战术手枪】")
            : ActiveEntry->Type == ELZInventoryItemType::Flashlight ? TEXT("【战术照明器材 · 探照侦察】")
            : ActiveEntry->Type == ELZInventoryItemType::Rare ? TEXT("【高阶机密战利品 · 核心资产】")
            : ActiveEntry->Type == ELZInventoryItemType::Medical ? TEXT("【战地急救耗材 · 恢复35HP】")
            : ActiveEntry->Type == ELZInventoryItemType::Ammo ? TEXT("【战术通用备弹 · 9x19mm】") : TEXT("【工业回收物资 · 变现资源】");

        Text(CategoryTag, PanelX + 20, PanelY + 50, Tint, 0.75f, PanelWidth - 40);
        Text(ItemName(ActiveEntry->Type), PanelX + 20, PanelY + 74, White, 1.25f, PanelWidth - 40);

        // Preview Box
        Frame(PanelX + 20, PanelY + 110, PanelWidth - 40, 84, Tint * 0.4f, FLinearColor(0.012f, 0.022f, 0.030f, 0.95f));
        DrawInventoryIcon(ActiveEntry->Type, OriginX + (PanelX + 36) * Scale, OriginY + (PanelY + 126) * Scale, 1.25f * Scale, Tint);

        const FString CountStr = ActiveEntry->Type == ELZInventoryItemType::Ammo
            ? FString::Printf(TEXT("现有储量:  %d 发 / 30"), ActiveEntry->Quantity)
            : FString::Printf(TEXT("现有数量:  %d 件"), ActiveEntry->Quantity);
        Text(CountStr, PanelX + 130, PanelY + 120, White, 0.82f, 240);
        Text(FString::Printf(TEXT("网格占位:  %d × %d 格  (共 %d 格)"), ActiveEntry->Width, ActiveEntry->Height, ActiveEntry->Width * ActiveEntry->Height),
            PanelX + 130, PanelY + 144, Muted, 0.78f, 240);
        Text(FString::Printf(TEXT("估值收益:  ◆ $%d"), ItemValue(*ActiveEntry)), PanelX + 130, PanelY + 168, Gold, 0.84f, 240);

        // Tactical Specs & Description
        Text(TEXT("战术属性与使用指南"), PanelX + 20, PanelY + 206, White, 0.80f);
        FString UseHint;
        FString Description;
        switch (ActiveEntry->Type)
        {
        case ELZInventoryItemType::Axe:
            UseHint = TEXT("指令: 按 1 切出应急重型消防斧，近距离破拆与击杀");
            Description = TEXT("占用 2×6 (或 6×2) 格空间。近距离攻击无需子弹，对感染者造成重创。在背包中即可随身装备。");
            break;
        case ELZInventoryItemType::Pistol:
            UseHint = TEXT("指令: 按 2 切合格洛克17手枪，按 R 装填弹匣");
            Description = FString::Printf(TEXT("占用 2×2 格空间。当前弹匣: %02d / 17，后备备弹: %02d。可靠的中近距离防卫火器。"),
                Character->GetAmmoInMagazine(), Character->GetReserveAmmo());
            break;
        case ELZInventoryItemType::Flashlight:
            UseHint = TEXT("指令: 探索中按 F 开关战术手电筒");
            Description = TEXT("占用 2×1 (或 1×2) 格空间。高流明防暴照明，穿透办公层黑暗走廊，不消耗电量。");
            break;
        case ELZInventoryItemType::Ammo:
            UseHint = TEXT("指令: 关闭背包后按 R 从背包备弹装入格洛克弹匣");
            Description = TEXT("占用 1×1 单格空间，单格最大堆叠 30 发。换弹时自动优先扣除背包备弹。");
            break;
        case ELZInventoryItemType::Medical:
            UseHint = Character->GetHealth() < 100.0f ? TEXT("指令: [E] 战地包扎 · 恢复 35 HP") : TEXT("状态: 生命值已达上限，无需使用");
            Description = FString::Printf(TEXT("占用 1×2 格空间。便携军用急救医疗包，当前体征: %.0f / 100。"), Character->GetHealth());
            break;
        case ELZInventoryItemType::Rare:
            UseHint = TEXT("指令: 成功撤离后计入高额带出估值 ($500)");
            Description = TEXT("占用 2×2 格空间。企业级机房刀片服务器备件，核心机密数据资产。");
            break;
        default:
            UseHint = TEXT("指令: 成功撤离后折算工业回收收益");
            Description = TEXT("占用 1×1 格空间。轻型工业电子废料元器件，可单格灵活塞入背包空隙。");
            break;
        }

        Text(UseHint, PanelX + 20, PanelY + 228, ActiveEntry->Type == ELZInventoryItemType::Medical ? Emerald : White, 0.78f, PanelWidth - 40);
        Text(Description, PanelX + 20, PanelY + 252, Muted, 0.70f, PanelWidth - 40);

        // Interactive Action Buttons
        const float BtnW = 180.0f;
        const float BtnH = 34.0f;
        const float RotBtnX = PanelX + 20;
        const float RotBtnY = PanelY + 296;
        const float DelBtnX = PanelX + 215;
        const float DelBtnY = PanelY + 296;

        float MouseScreenX = 0.0f, MouseScreenY = 0.0f;
        float CanvasMX = -1.0f, CanvasMY = -1.0f;
        if (PC && PC->GetMousePosition(MouseScreenX, MouseScreenY))
        {
            CanvasMX = (MouseScreenX - OriginX) / Scale;
            CanvasMY = (MouseScreenY - OriginY) / Scale;
        }

        const bool bHoverRot = CanvasMX >= RotBtnX && CanvasMX <= RotBtnX + BtnW && CanvasMY >= RotBtnY && CanvasMY <= RotBtnY + BtnH;
        const bool bHoverDel = CanvasMX >= DelBtnX && CanvasMX <= DelBtnX + BtnW && CanvasMY >= DelBtnY && CanvasMY <= DelBtnY + BtnH;

        Frame(RotBtnX, RotBtnY, BtnW, BtnH, bHoverRot ? Cyan : Gold,
            bHoverRot ? FLinearColor(0.04f, 0.14f, 0.18f, 0.95f) : FLinearColor(0.02f, 0.04f, 0.05f, 0.95f), 1.5f);
        Text(TEXT("↻ [点击/R] 旋转方向"), RotBtnX + 16, RotBtnY + 8, bHoverRot ? Cyan : Gold, 0.80f);

        Frame(DelBtnX, DelBtnY, BtnW, BtnH, bHoverDel ? Red : GridBorder,
            bHoverDel ? FLinearColor(0.25f, 0.05f, 0.05f, 0.95f) : FLinearColor(0.02f, 0.03f, 0.04f, 0.95f), 1.5f);
        Text(TEXT("✕ [点击/Del] 丢弃装备"), DelBtnX + 16, DelBtnY + 8, bHoverDel ? Red : Muted, 0.80f);

        Text(TEXT("[左键拖拽 / E] 放置装备   [右键] 取消移动并放回原位   [中键/R] 旋转"), PanelX + 20, PanelY + 338, Muted, 0.70f);
    }
    else
    {
        Text(TEXT("【空闲就绪网格】"), PanelX + 20, PanelY + 54, Muted, 0.76f);
        Text(FString::Printf(TEXT("网格坐标: %c%d  (就绪)"), 'A' + CursorRow, CursorCol + 1), PanelX + 20, PanelY + 80, White, 1.22f);
        Text(TEXT("该网格处于完全空闲就绪状态。"), PanelX + 20, PanelY + 120, Muted, 0.82f);
        Text(TEXT("◆ 装备规格与自由收纳指南:"), PanelX + 20, PanelY + 150, White, 0.82f);
        Text(TEXT("• 应急消防破拆斧: 占用 2×6 (或 6×2) 格"), PanelX + 32, PanelY + 176, Red, 0.78f);
        Text(TEXT("• 格洛克17战术手枪: 占用 2×2 格"), PanelX + 32, PanelY + 200, Amber, 0.78f);
        Text(TEXT("• 战术强光手电筒: 占用 2×1 (或 1×2) 格"), PanelX + 32, PanelY + 224, Gold, 0.78f);
        Text(TEXT("• 战地医疗急救包: 占用 1×2 格 (可按E使用)"), PanelX + 32, PanelY + 248, Emerald, 0.78f);
        Text(TEXT("• 9mm备弹 / 电子零件: 占用 1×1 单格"), PanelX + 32, PanelY + 272, TechBlue, 0.78f);
        Text(TEXT("• 按 [R] 自由旋转物品方向，支持鼠标点击或拖拽自由排布。"), PanelX + 20, PanelY + 310, Cyan, 0.80f);
    }

    // Secure Container Status Chip (Right Bottom)
    Frame(PanelX + 20, PanelY + 364, PanelWidth - 40, 36, Gold * 0.75f, FLinearColor(0.04f, 0.03f, 0.015f, 0.95f), 1.2f);
    Rect(PanelX + 20, PanelY + 364, 4, 36, Gold);
    const FString FuseState = GameMode->IsOfficePowerRestored() ? TEXT("已通电 · 供电全恢复 · 安全门开启")
        : GameMode->HasFuse() ? TEXT("安全携带中 · 15A主线保险丝 · 前往配电室安装") : TEXT("未获取 · 前往办公区拾取15A保险丝");
    Text(TEXT("◆ 密保安全箱:"), PanelX + 32, PanelY + 372, Gold, 0.75f);
    Text(FuseState, PanelX + 140, PanelY + 372, GameMode->HasFuse() ? Cyan : Muted, 0.75f, PanelWidth - 160);

    // Capacity & Load Progress Meter at Bottom of Inspector
    Rect(PanelX + 20, PanelY + 410, PanelWidth - 40, 1, GridBorder);
    const int32 UsedSlots = Character->GetUsedBagSlots();
    const int32 MaxSlots = Character->GetMaxBagSlots();
    Text(FString::Printf(TEXT("网格负载:  %d / %d 格 (%d%%)"), UsedSlots, MaxSlots, (UsedSlots * 100) / MaxSlots),
        PanelX + 20, PanelY + 420, UsedSlots >= MaxSlots ? Amber : White, 0.82f, 318);

    // 36 small load bars
    for (int32 SlotBar = 0; SlotBar < 36; ++SlotBar)
    {
        const bool bSlotFilled = SlotBar < UsedSlots;
        const float BarX = PanelX + 20 + (SlotBar % 18) * 35.0f;
        const float BarY = PanelY + 444 + (SlotBar / 18) * 8.0f;
        Rect(BarX, BarY, 32.0f, 6.0f, bSlotFilled ? (UsedSlots >= MaxSlots ? Amber : Cyan) : FLinearColor(0.05f, 0.09f, 0.12f));
    }

    // FOOTER COMMAND BAR
    Rect(72, 622, 1140, 1, GridBorder);
    const FString Feedback = Character->GetInventoryStatusText();
    Text(Feedback.IsEmpty() ? TEXT("战区提示: 三角洲空间收纳背包。支持点击与拖拽放置，按 R 旋转装备，世界不会暂停。") : Feedback,
        74, 630, Feedback.IsEmpty() ? Muted : Amber, 0.76f, 1136);
    Text(TEXT("[B / ESC] 关闭背包   [鼠标点击/拖拽/方向键] 选择网格   [左键/松开/E] 拿起/放下   [R] 旋转方向   [右键] 取消   [Delete] 丢弃装备"),
        74, 654, White, 0.78f, 1136);
}
