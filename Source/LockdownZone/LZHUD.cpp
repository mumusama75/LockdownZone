#include "LZHUD.h"

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

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawRect(FLinearColor(0.008f, 0.016f, 0.02f, 0.65f), 22, 20, 250, 60);
    DrawRect(FLinearColor(0.24f, 0.70f, 0.73f, 0.9f), 22, 20, 3, 60);
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
    DrawShadowedText(TEXT("12F  /  隔离办公层"), 34, 58, FLinearColor(.62f,.70f,.70f), .70f);
    DrawShadowedText(FString::Printf(TEXT("生命    %03.0f"), Character->GetHealth()),
        34.0f, Canvas->ClipY - 115.0f, Character->GetHealth() < 30.0f ? FLinearColor::Red : FLinearColor::White, 1.15f);
    const FString WeaponLine = Character->GetSelectedWeapon() == EPlayerWeapon::Firearm
        ? FString::Printf(TEXT("%s   %02d / 17   备弹 %02d"), *Character->GetSelectedWeaponName(),
            Character->GetAmmoInMagazine(), Character->GetReserveAmmo())
        : FString::Printf(TEXT("武器    %s"), *Character->GetSelectedWeaponName());
    DrawShadowedText(WeaponLine, 34.0f, Canvas->ClipY - 82.0f,
        FLinearColor(1.0f, 0.8f, 0.25f), 1.1f);
    DrawShadowedText(FString::Printf(TEXT("已清除感染者  %d"), GameMode->GetEnemiesKilled()),
        34.0f, Canvas->ClipY - 49.0f, FLinearColor(0.75f, 0.9f, 0.75f), 1.0f);
    DrawShadowedText(TEXT("E交互  1/2切枪  R换弹  F手电  B背包"), Canvas->ClipX - 390, Canvas->ClipY - 37,
        FLinearColor(.64f,.72f,.74f), .68f);
    const FString FlashlightLine = !Character->HasFlashlight() ? TEXT("手电筒：未拾取")
        : Character->IsFlashlightOn() ? TEXT("手电筒：开启  [F] 关闭") : TEXT("手电筒：关闭  [F] 开启");
    DrawShadowedText(FlashlightLine, Canvas->ClipX - 360, Canvas->ClipY - 92,
        Character->IsFlashlightOn() ? FLinearColor(1.0f, .91f, .65f) : FLinearColor(.64f,.72f,.74f), .82f);
    DrawShadowedText(FString::Printf(TEXT("背包 %d / %d  |  物资价值 %d"), Character->GetUsedBagSlots(),
        Character->GetMaxBagSlots(), Character->GetLootValue()), Canvas->ClipX - 360, Canvas->ClipY - 65,
        FLinearColor(.88f,.78f,.48f), .78f);

    DrawShadowedText(GameMode->GetObjectiveText(), Canvas->ClipX - 550.0f, 32.0f,
        GameMode->IsObjectiveComplete() ? FLinearColor(0.3f, 1.0f, 0.45f) : FLinearColor(0.95f, 0.85f, 0.35f), 1.0f);
    DrawShadowedText(GameMode->GetStatusText(), Canvas->ClipX - 550.0f, 64.0f,
        FLinearColor(0.85f, 0.85f, 0.85f), 0.85f);

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
    const FLinearColor GridBorder(0.12f, 0.22f, 0.28f);
    const FLinearColor Red(0.95f, 0.25f, 0.22f);
    const FLinearColor Amber(0.96f, 0.65f, 0.22f);
    const FLinearColor White(0.92f, 0.95f, 0.96f);
    const FLinearColor Muted(0.55f, 0.64f, 0.68f);

    constexpr float CellWidth = 236.0f;
    constexpr float CellHeight = 128.0f;
    constexpr float Gap = 12.0f;

    // Mouse click selection support
    if (APlayerController* PC = GetOwningPlayerController())
    {
        float MouseX = 0.0f, MouseY = 0.0f;
        if (PC->GetMousePosition(MouseX, MouseY) && MouseX > 0.0f && MouseY > 0.0f)
        {
            const float CanvasMouseX = (MouseX - OriginX) / Scale;
            const float CanvasMouseY = (MouseY - OriginY) / Scale;
            for (int32 Slot = 0; Slot < 6; ++Slot)
            {
                const float X = 76 + (Slot % 3) * (CellWidth + Gap);
                const float Y = 268 + (Slot / 3) * (CellHeight + Gap);
                if (CanvasMouseX >= X && CanvasMouseX <= X + CellWidth &&
                    CanvasMouseY >= Y && CanvasMouseY <= Y + CellHeight)
                {
                    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
                    {
                        const_cast<ALZCharacter*>(Character)->SetSelectedInventorySlot(Slot);
                    }
                }
            }
        }
    }

    // Outer Fullscreen Dark Blur Overlay
    DrawRect(FLinearColor(0.002f, 0.005f, 0.008f, 0.88f), 0, 0, Canvas->ClipX, Canvas->ClipY);

    // Master Tactical Chassis Frame
    Frame(50, 36, 1180, 648, GridBorder, DarkBG);
    Rect(50, 36, 4, 648, Cyan);

    // Corner decorative tactical reticles
    Rect(54, 38, 14, 2, Cyan);
    Rect(54, 38, 2, 14, Cyan);
    Rect(1212, 38, 14, 2, Cyan);
    Rect(1224, 38, 2, 14, Cyan);

    // HEADER SECTION
    Text(TEXT("战区战术背包 / TACTICAL BACKPACK"), 76, 54, White, 1.35f);
    Text(TEXT("零号大厦 · 隔离办公层 | 特战单兵携行系统"), 76, 92, Muted, 0.74f, 620);

    // Player Vitals in Header
    Text(FString::Printf(TEXT("干员体征: %03.0f / 100"), Character->GetHealth()), 660, 58,
        Character->GetHealth() < 30.0f ? Red : Emerald, 0.82f, 210);
    for (int32 Bar = 0; Bar < 8; ++Bar)
    {
        const bool bActive = (Character->GetHealth() / 100.0f) * 8 > Bar;
        Rect(660 + Bar * 24, 82, 18, 8, bActive ? (Character->GetHealth() < 30.0f ? Red : Emerald) : FLinearColor(0.06f, 0.10f, 0.12f));
    }

    // Extraction Valuation Chip & Warning Badge
    Frame(896, 52, 310, 48, Gold, FLinearColor(0.08f, 0.06f, 0.02f, 0.95f), 1.5f);
    Rect(896, 52, 4, 48, Gold);
    Text(TEXT("◆ 撤离资产估值"), 912, 60, Muted, 0.68f);
    Text(FString::Printf(TEXT("$%d"), Character->GetLootValue()), 912, 75, Gold, 1.25f);
    Frame(1048, 62, 146, 28, FLinearColor(0.48f, 0.18f, 0.12f), FLinearColor(0.12f, 0.04f, 0.03f));
    Text(TEXT("⚠️ 战区状态 · 危险未暂停"), 1056, 68, Amber, 0.68f, 130);

    Rect(76, 122, 1128, 1, GridBorder);

    // SECTION 1: TACTICAL GEAR & SECURE CONTAINER (TOP BAR)
    Text(TEXT("◆ 特战战备装具 / TACTICAL GEAR"), 76, 132, TechBlue, 0.74f);
    Text(TEXT("装具槽位 (独立携带·不可丢弃)"), 610, 132, Muted, 0.67f, 198);

    // Slot 1: Primary Weapon (Glock)
    Frame(76, 154, 236, 68, GridBorder, PanelBG);
    Rect(76, 154, 3, 68, Character->GetSelectedWeapon() == EPlayerWeapon::Firearm ? Cyan : GridBorder);
    Text(TEXT("主武器"), 88, 162, TechBlue, 0.70f);
    Text(TEXT("格洛克 17 战术手枪"), 88, 182, White, 0.85f, 215);
    Text(FString::Printf(TEXT("弹匣: %02d / 17   备用: %02d"), Character->GetAmmoInMagazine(), Character->GetReserveAmmo()),
        88, 204, Muted, 0.70f, 215);

    // Slot 2: Melee Weapon (Fire Axe)
    Frame(324, 154, 236, 68, GridBorder, PanelBG);
    Rect(324, 154, 3, 68, Character->GetSelectedWeapon() == EPlayerWeapon::Melee ? Red : GridBorder);
    Text(TEXT("近战武器"), 336, 162, Red, 0.70f);
    Text(TEXT("应急重型消防斧"), 336, 182, White, 0.85f, 215);
    Text(TEXT("近身破拆 · 静默击杀 · 无限损耗"), 336, 204, Muted, 0.70f, 215);

    // Slot 3: Tactical Flashlight
    Frame(572, 154, 236, 68, GridBorder, PanelBG);
    Rect(572, 154, 3, 68, Character->IsFlashlightOn() ? Amber : GridBorder);
    Text(TEXT("战术照明"), 584, 162, Amber, 0.70f);
    Text(Character->HasFlashlight() ? TEXT("战术强光手电 (已装备)") : TEXT("未装备战术手电"), 584, 182, White, 0.85f, 215);
    Text(Character->HasFlashlight()
        ? (Character->IsFlashlightOn() ? TEXT("状态: [F] 照射开启") : TEXT("状态: [F] 已关闭"))
        : TEXT("前往配电室控制台拾取"), 584, 204, Character->IsFlashlightOn() ? Amber : Muted, 0.70f, 215);

    // SECTION 2: 3x2 STORAGE GRID (CENTER)
    Text(TEXT("◆ 战术收纳矩阵 / TACTICAL STORAGE MATRIX"), 76, 240, TechBlue, 0.74f);
    Text(TEXT("容量: 3 x 2  共 6 格"), 700, 240, Muted, 0.68f, 108);

    auto ItemColor = [&Gold, &Emerald, &TechBlue, &White](ELZInventoryItemType Type) -> FLinearColor
    {
        switch (Type)
        {
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
        case ELZInventoryItemType::Rare: return 500;
        case ELZInventoryItemType::Medical: return 60;
        case ELZInventoryItemType::Ammo: return Item.Quantity * 3;
        default: return 35;
        }
    };

    const int32 SelectedSlot = Character->GetSelectedInventorySlot();
    const FLZInventoryEntry* Selected = Character->GetInventoryItemAtSlot(SelectedSlot);

    // Draw grid matrix cells
    for (int32 Slot = 0; Slot < 6; ++Slot)
    {
        const int32 Row = Slot / 3;
        const int32 Col = Slot % 3;
        const float X = 76 + Col * (CellWidth + Gap);
        const float Y = 268 + Row * (CellHeight + Gap);
        const bool bIsSelected = (Slot == SelectedSlot);

        Frame(X, Y, CellWidth, CellHeight, bIsSelected ? Cyan : GridBorder,
            bIsSelected ? FLinearColor(0.02f, 0.05f, 0.07f, 0.95f) : PanelBG);

        // Delta Force style grid coordinate label (A1, A2, A3, B1, B2, B3)
        const FString Coord = FString::Printf(TEXT("%c%d"), 'A' + Row, Col + 1);
        Text(Coord, X + 10, Y + 8, bIsSelected ? Cyan : FLinearColor(0.25f, 0.35f, 0.40f), 0.68f);

        // Highlight selection corner brackets
        if (bIsSelected)
        {
            Rect(X + 1, Y + 1, 12, 2, Cyan);
            Rect(X + 1, Y + 1, 2, 12, Cyan);
            Rect(X + CellWidth - 13, Y + 1, 12, 2, Cyan);
            Rect(X + CellWidth - 3, Y + 1, 2, 12, Cyan);
            Rect(X + 1, Y + CellHeight - 3, 12, 2, Cyan);
            Rect(X + 1, Y + CellHeight - 13, 2, 12, Cyan);
            Rect(X + CellWidth - 13, Y + CellHeight - 3, 12, 2, Cyan);
            Rect(X + CellWidth - 3, Y + CellHeight - 13, 2, 12, Cyan);
        }

        const FLZInventoryEntry* Item = Character->GetInventoryItemAtSlot(Slot);
        if (!Item)
        {
            Rect(X + CellWidth * 0.5f - 8, Y + CellHeight * 0.5f - 1, 16, 2, FLinearColor(0.12f, 0.18f, 0.22f));
            Rect(X + CellWidth * 0.5f - 1, Y + CellHeight * 0.5f - 8, 2, 16, FLinearColor(0.12f, 0.18f, 0.22f));
            continue;
        }

        // If this slot is secondary part of multi-slot item, avoid re-drawing main labels
        if (Item->StartSlot != Slot)
        {
            Text(TEXT("◄ 占用延伸"), X + 24, Y + 56, Muted * 0.8f, 0.70f);
            continue;
        }

        const int32 Span = Item->SlotsPerItem;
        const float Width = Span > 1 ? (CellWidth * Span + Gap * (Span - 1)) : CellWidth;
        const FLinearColor Tint = ItemColor(Item->Type);

        // Quality border stripe on top
        Rect(X, Y, Width, 3, Tint);

        DrawInventoryIcon(Item->Type, OriginX + (X + 14) * Scale, OriginY + (Y + 36) * Scale, Scale, Tint);
        Text(ItemName(Item->Type), X + 82, Y + 34, White, 0.88f, Width - 94);

        if (Item->Type == ELZInventoryItemType::Ammo)
        {
            Frame(X + Width - 72, Y + 8, 64, 20, TechBlue * 0.6f, FLinearColor(0.04f, 0.12f, 0.16f));
            Text(FString::Printf(TEXT("x%02d"), Item->Quantity), X + Width - 60, Y + 10, TechBlue, 0.74f);
        }
        else if (Item->Quantity > 1)
        {
            Text(FString::Printf(TEXT("x%d"), Item->Quantity), X + Width - 48, Y + 10, White, 0.74f);
        }

        Text(Item->Type == ELZInventoryItemType::Rare ? TEXT("机密战利品 · 高价值资产")
            : Item->Type == ELZInventoryItemType::Medical ? TEXT("战地医疗包 · 恢复35HP")
            : Item->Type == ELZInventoryItemType::Ammo ? TEXT("9x19mm 手枪备用弹药") : TEXT("工业电子废料 · 回收价值"),
            X + 82, Y + 72, Muted, 0.65f, Width - 94);
        Text(FString::Printf(TEXT("占用 %d 格"), Span), X + 15, Y + 104, Muted, 0.67f);
        Text(FString::Printf(TEXT("估值 $%d"), ItemValue(*Item)), X + Width - 118, Y + 104, Tint, 0.72f, 104);
        if (Span > 1)
        {
            Rect(X + CellWidth + Gap * 0.5f, Y + 34, 1, 60, Tint * 0.35f);
        }
    }

    // SECTION 3: DELTA FORCE SECURE CONTAINER (LEFT-BOTTOM)
    Frame(76, 544, 732, 46, Gold * 0.75f, FLinearColor(0.04f, 0.03f, 0.015f, 0.95f), 1.2f);
    Rect(76, 544, 4, 46, Gold);
    const FString FuseState = GameMode->IsOfficePowerRestored() ? TEXT("已通电 · 供电全恢复 · 安全门开启")
        : GameMode->HasFuse() ? TEXT("安全携带中 · 15A主线保险丝 · 前往配电室安装") : TEXT("未获取 · 前往办公区拾取15A保险丝");
    Text(TEXT("◆ 密保安全箱 (金品质·必带出)"), 90, 553, Gold, 0.75f);
    Text(FuseState, 280, 553, GameMode->HasFuse() ? Cyan : Muted, 0.76f, 440);
    Text(TEXT("密保槽位: 1/1"), 720, 553, Muted, 0.67f, 80);

    // SECTION 4: TACTICAL ITEM INSPECTOR (RIGHT PANEL)
    Frame(842, 122, 362, 468, GridBorder, PanelBG);
    Rect(842, 122, 4, 468, Cyan);
    Text(TEXT("战术物资检视 / INSPECTION"), 862, 136, Cyan, 0.88f);
    Rect(862, 160, 322, 1, GridBorder);

    if (Selected)
    {
        const FLinearColor Tint = ItemColor(Selected->Type);
        const FString CategoryTag = Selected->Type == ELZInventoryItemType::Rare ? TEXT("【高阶机密战利品】")
            : Selected->Type == ELZInventoryItemType::Medical ? TEXT("【战地急救耗材】")
            : Selected->Type == ELZInventoryItemType::Ammo ? TEXT("【战术通用备弹】") : TEXT("【工业回收物资】");
        Text(CategoryTag, 862, 172, Tint, 0.74f, 322);
        Text(ItemName(Selected->Type), 862, 196, White, 1.28f, 322);

        // Preview Box
        Frame(862, 236, 322, 88, Tint * 0.4f, FLinearColor(0.012f, 0.022f, 0.030f, 0.95f));
        DrawInventoryIcon(Selected->Type, OriginX + 876 * Scale, OriginY + 252 * Scale, 1.2f * Scale, Tint);
        Text(Selected->Type == ELZInventoryItemType::Ammo
            ? FString::Printf(TEXT("现有储量:  %d 发 / 30"), Selected->Quantity)
            : FString::Printf(TEXT("现有储量:  %d 件"), Selected->Quantity), 964, 250, White, 0.82f, 210);
        Text(FString::Printf(TEXT("网格负载:  %d 格"), Selected->SlotsPerItem), 964, 276, Muted, 0.78f, 210);
        Text(FString::Printf(TEXT("撤离估值:  ◆ $%d"), ItemValue(*Selected)), 964, 300, Gold, 0.84f, 210);

        // Attribute Specs
        Text(TEXT("战术属性与使用指南"), 862, 336, White, 0.80f);
        FString UseHint;
        FString Description;
        switch (Selected->Type)
        {
        case ELZInventoryItemType::Ammo:
            UseHint = TEXT("指令: 关闭背包后按 R 装填弹匣");
            Description = TEXT("9x19mm 手枪通用弹药，换弹时自动扣减背包存量。");
            break;
        case ELZInventoryItemType::Medical:
            UseHint = Character->GetHealth() < 100.0f ? TEXT("指令: [E] 战地包扎 · 恢复 35 HP") : TEXT("状态: 生命值已达上限，无需使用");
            Description = FString::Printf(TEXT("便携军用医疗包，当前体征: %.0f / 100。"), Character->GetHealth());
            break;
        case ELZInventoryItemType::Rare:
            UseHint = TEXT("指令: 成功撤离后计入高额带出估值");
            Description = TEXT("企业级服务器机架刀片备件，需同一行两个连续网格。");
            break;
        default:
            UseHint = TEXT("指令: 成功撤离后折算回收收益");
            Description = TEXT("轻型工业电子元器件，可单格随身携带。");
            break;
        }
        Text(UseHint, 862, 362, Selected->Type == ELZInventoryItemType::Medical ? Emerald : White, 0.78f, 322);
        Text(Description, 862, 392, Muted, 0.68f, 322);

        // Action Buttons Box
        Frame(862, 428, 322, 44, Red * 0.6f, FLinearColor(0.08f, 0.03f, 0.02f, 0.90f));
        Text(TEXT("[ Delete ] 战区丢弃整组 (无法找回)"), 874, 442, Red, 0.75f, 300);
    }
    else
    {
        Text(TEXT("【可用储物槽位】"), 862, 175, Muted, 0.76f, 322);
        Text(TEXT("空闲网格"), 862, 202, White, 1.25f, 322);
        Text(TEXT("该槽位处于就绪状态。"), 862, 260, Muted, 0.82f, 322);
        Text(TEXT("靠近场景物资按 [E] 搜刮装入。"), 862, 295, White, 0.82f, 322);
        Text(TEXT("大件物资需同排相邻两格连续空间。"), 862, 330, Muted, 0.76f, 322);
        Text(TEXT("装备器材与主线保险丝独立携带。"), 862, 365, Muted, 0.76f, 322);
    }

    // Capacity & Load Progress Meter at Bottom of Inspector
    Rect(862, 488, 322, 1, GridBorder);
    const int32 UsedSlots = Character->GetUsedBagSlots();
    const int32 MaxSlots = Character->GetMaxBagSlots();
    Text(FString::Printf(TEXT("网格占用率:  %d / %d 格 (%d%%)"), UsedSlots, MaxSlots, (UsedSlots * 100) / MaxSlots),
        864, 498, UsedSlots >= MaxSlots ? Amber : White, 0.82f, 318);
    for (int32 SlotBar = 0; SlotBar < 6; ++SlotBar)
    {
        const bool bSlotFilled = SlotBar < UsedSlots;
        Rect(864 + SlotBar * 53, 524, 48, 10, bSlotFilled ? (UsedSlots >= MaxSlots ? Amber : Cyan) : FLinearColor(0.06f, 0.10f, 0.12f));
    }
    Text(FString::Printf(TEXT("战利品总值:  $%d"), Character->GetLootValue()), 864, 548, Gold, 1.05f, 318);

    // FOOTER COMMAND BAR
    Rect(76, 604, 1128, 1, GridBorder);
    const FString Feedback = Character->GetInventoryStatusText();
    Text(Feedback.IsEmpty() ? TEXT("战区提示: 只搜取高价值与必要补给。整理背包时现实世界不会暂停。") : Feedback,
        78, 614, Feedback.IsEmpty() ? Muted : Amber, 0.76f, 1124);
    Text(TEXT("[B / ESC] 收拢背包   [↑↓←→ / 鼠标点击] 选择网格   [E] 战地使用医疗包   [Delete] 丢弃整组   [R] 装填备弹"),
        78, 642, White, 0.78f, 1124);
}
