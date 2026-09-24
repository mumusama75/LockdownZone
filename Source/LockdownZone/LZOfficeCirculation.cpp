#include "LZGameMode.h"
#include "LZVentNetwork.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Centimetres. Room frontages and door widths are authored together, so the
// exported plan describes the collision walls built by this exact function.
void ALZGameMode::BuildOfficeCirculation()
{
    const FLinearColor Wall(.17f,.18f,.19f), Service(.06f,.18f,.23f);
    TArray<FString> Plan;
    Plan.Add(TEXT("kind,name,x,y,width,depth"));
    auto WallBox = [&](const TCHAR* Name, float X, float Y, float W, float D)
    {
        SpawnModularWall(Name,FVector(X,Y,175),FVector(W,D,350),FRotator::ZeroRotator,Wall);
        Plan.Add(FString::Printf(TEXT("wall,%s,%.0f,%.0f,%.0f,%.0f"),Name,X,Y,W,D));
    };
    auto H = [&](const TCHAR* Name, float A, float B, float Y)
    { WallBox(Name,(A+B)/2,Y,B-A,60); };
    auto V = [&](const TCHAR* Name, float X, float A, float B)
    { WallBox(Name,X,(A+B)/2,60,B-A); };
    auto Door = [&](const TCHAR* Name, float X, float Y, float Width, bool bVertical=false)
    {
        SpawnBlock(FString(Name)+TEXT("Lintel"),FVector(X,Y,325),
            bVertical ? FVector(60,Width,50) : FVector(Width,60,50),FRotator::ZeroRotator,Wall);
        Plan.Add(FString::Printf(TEXT("door,%s,%.0f,%.0f,%.0f,%.0f"),Name,X,Y,bVertical?60:Width,bVertical?Width:60));
    };
    auto Window = [&](const TCHAR* Name,float X,float Y,float Width,float SillHeight=70)
    {
        SpawnBlock(FString(Name)+TEXT("Sill"),FVector(X,Y,SillHeight/2),FVector(Width,60,SillHeight));
        SpawnBlock(FString(Name)+TEXT("Header"),FVector(X,Y,340),FVector(Width,60,20));
        auto* Pane=SpawnBlock(Name,FVector(X,Y,(330+SillHeight)/2),FVector(Width,4,330-SillHeight));
        Pane->Tags.Add(TEXT("LZMeetingObservation"));
        Pane->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/M_ObservationGlass.M_ObservationGlass")));
        Pane->GetStaticMeshComponent()->SetCastShadow(false);
        Plan.Add(FString::Printf(TEXT("window,%s,%.0f,%.0f,%.0f,4"),Name,X,Y,Width));
    };
    // Public/admin gallery: 390 cm clear between the 900/1350 wall centrelines.
    if(ALZVentNetwork::IsNewLayout())
    {
        H(TEXT("ReceptionClosedFront"),-2520,60,900);
        H(TEXT("AdminFrontEastAB"),540,3260,900);
        Door(TEXT("PowerShortcutC"),3380,900,240);
        H(TEXT("PowerFrontEastC"),3500,3750,900);
        Door(TEXT("OfficeExitB"),300,900,480);
        // Diagonal closure connects wake-room corner to the relocated frontage.
        auto* Closure=SpawnBlock(TEXT("ReceptionCornerClosure"),FVector(-2510,825,175),FVector(60,210,350));
    }
    else
    {
        H(TEXT("NorthFrontA"),-2050,280,900);
        H(TEXT("NorthFrontB"),520,1780,900);
        H(TEXT("NorthFrontC"),2020,2880,900);
        H(TEXT("NorthFrontD"),3120,3750,900);
        Door(TEXT("AdminEntry"),400,900,240);
        Door(TEXT("AdminEastEntry"),1900,900,240);
        Door(TEXT("PowerEntry"),3000,900,240);
    }
    if(ALZVentNetwork::IsNewLayout())
    {
        V(TEXT("PowerWestBeforeD"),2200,900,1030);
        Door(TEXT("PowerEntryD"),2200,1150,240,true);
        V(TEXT("PowerWestAfterD"),2200,1270,2050);
    }
    else V(TEXT("PowerSecureWest"),2200,900,2050);
    V(TEXT("ManagerWest"),-1600,1350,2050);
    V(TEXT("ManagerEast"),-650,1350,2050);
    if(!ALZVentNetwork::IsNewLayout())
    {
    // Fallen reinforced ceiling cassette rests on a desk frame: a walkable 30-degree ramp.
    SpawnBlock(TEXT("CollapsedCeilingRamp"),FVector(-1150,1550,170),FVector(700,180,16),FRotator(30,0,0),Wall);
    SpawnBlock(TEXT("RampSupportFrame"),FVector(-990,1550,105),FVector(130,130,180),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("VentFloorAboveCeiling"),FVector(-685,1550,350),FVector(330,180,10),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("VentUpperRoof"),FVector(-605,1550,490),FVector(490,200,20),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("VentUpperSideA"),FVector(-605,1455,420),FVector(490,10,130),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("VentUpperSideB"),FVector(-605,1645,420),FVector(490,10,130),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("VentEndCap"),FVector(-345,1550,420),FVector(10,200,140),FRotator::ZeroRotator,Service);
    Plan.Add(TEXT("vent,UpperServiceDuct,-605,1550,490,200"));
    Plan.Add(TEXT("ramp,FallenCeilingAccess,-1150,1550,700,180"));
    }
    V(TEXT("RecordsEast"),-250,1350,2050);
    V(TEXT("ArchiveEast"),1600,1350,2050);
    H(TEXT("ManagerFrontA"),-1600,-1200,1350);
    H(TEXT("ManagerFrontB"),-1000,-550,1350);
    Door(TEXT("ManagerEntry"),-1100,1350,200);
    Door(TEXT("RecordsEntry"),-450,1350,200);
    H(TEXT("ArchiveFrontA"),-350,380,1350);
    H(TEXT("ArchiveFrontB"),620,720,1350);
    Window(TEXT("ArchiveFlashlightObservation"),1020,1350,600,16);
    H(TEXT("ArchiveFrontWindowEast"),1320,1600,1350);
    Door(TEXT("ArchiveEntry"),500,1350,240);
    // Southern back-of-house gallery, 240 cm clear; IT never acts as the
    // only route to the pantry, meeting room or toilets.
    if(ALZVentNetwork::IsNewLayout())
    {
        // Player-defined north is -Y. Only A connects the office to this gallery.
        H(TEXT("MeetingViewWest"),-2520,-2400,-750);
        Window(TEXT("OfficeMeetingWindow"),-1775,-750,1250);
        H(TEXT("MeetingViewEast"),-1150,60,-750);
        H(TEXT("OfficeNorthBoundaryEast"),540,3750,-750);
        Door(TEXT("OfficeExitA"),300,-750,480);
        // The gallery now continues past IT to its eastern entrance (D16).
        H(TEXT("ITGalleryFront"),1150,3150,-1200);
        V(TEXT("ServerWestAB"),1150,-2050,-1200);
    }
    else
    {
        H(TEXT("SouthFrontA"),-2500,-2370,-900);
        H(TEXT("SouthFrontB"),-2130,1730,-900);
        H(TEXT("SouthFrontC"),1970,3150,-900);
        Door(TEXT("SupportEntry"),-2250,-900,240);
        Door(TEXT("ServerFrontEntry"),1850,-900,240);
        V(TEXT("ServerWestLower"),1150,-2050,-1220);
        V(TEXT("ServerWestUpper"),1150,-980,-900);
        Door(TEXT("ServerServiceEntry"),1150,-1100,240,true);
        ServiceShortcutDoor = SpawnBlock(TEXT("ServerInsideReleaseDoor"),FVector(1150,-1100,150),FVector(24,240,300),FRotator::ZeroRotator,Service);
    }
    V(TEXT("ServerEastLower"),3150,-2050,-1660);
    V(TEXT("ServerEastUpper"),3150,-1420,ALZVentNetwork::IsNewLayout()?-1200:-900);
    Door(TEXT("ServerEastEntry"),3150,-1540,240,true);
    const float Dividers[] = {-2700,-2200,-650};
    for (int32 I=0; I<3; ++I) V(*FString::Printf(TEXT("SupportDivider%d"),I),Dividers[I],-2050,-1200);
    H(TEXT("ToiletsFrontA"),-3750,-3300,-1200);
    H(TEXT("ToiletsFrontB"),-3100,-2550,-1200);
    if(ALZVentNetwork::IsNewLayout())
    {
        H(TEXT("CleanerFrontB"),-2350,-2200,-1200);
        H(TEXT("MeetingWindowWest"),-2200,-2050,-1200);
        Window(TEXT("MeetingRoomWindow"),-1650,-1200,800);
        H(TEXT("MeetingWindowEast"),-1250,-1070,-1200);
        Door(TEXT("MeetingEntry"),-950,-1200,240);
        H(TEXT("MeetingFrontEast"),-830,180,-1200);
    }
    else
    {
        H(TEXT("CleanerFrontB"),-2350,-1950,-1200);
        H(TEXT("MeetingVaultSide"),-1750,-1570,-1200);
        auto* MeetingBarrier=SpawnBlock(TEXT("MeetingVaultBarrier"),FVector(-1850,-1200,47.5f),FVector(200,50,95),FRotator::ZeroRotator,Service);
        MeetingBarrier->Tags.Add(TEXT("LZVaultable"));
        Door(TEXT("MeetingVault"),-1850,-1200,200);
        Plan.Add(TEXT("vault,MeetingShortcut,-1850,-1200,200,50"));
        H(TEXT("MeetingFrontB"),-1330,180,-1200);
        Door(TEXT("MeetingEntry"),-1450,-1200,240);
    }
    H(TEXT("PantryFrontB"),420,1150,-1200);
    Door(TEXT("ToiletsEntry"),-3200,-1200,200);
    Door(TEXT("CleanerEntry"),-2450,-1200,200);
    Door(TEXT("PantryEntry"),300,-1200,240);
    // Simple functional fixtures; the design pass deliberately uses greybox.
    for (int32 I=0;I<3;++I)
    {
        SpawnBlock(TEXT("WCBasin"),FVector(-3590+I*240,-1930,45),FVector(85,100,90),FRotator::ZeroRotator,Wall);
        if(I<2) SpawnBlock(TEXT("WCPartition"),FVector(-3470+I*240,-1830,105),FVector(12,400,210),FRotator::ZeroRotator,Wall);
    }
    SpawnBlock(TEXT("JanitorSink"),FVector(-2450,-1930,45),FVector(160,100,90),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("PantryCounter"),FVector(350,-1970,45),FVector(1000,120,90),FRotator::ZeroRotator,Wall);
    if(!ALZVentNetwork::IsNewLayout()) SpawnBlock(TEXT("FuseMaintenanceConsole"),FVector(2050,-1250,38),FVector(100,60,76),FRotator::ZeroRotator,Service);
    const FVector RoomLights[] = { FVector(-1450,-1630,310),FVector(300,-1650,310),
        FVector(-3200,-1600,310),FVector(-2450,-1650,310),FVector(-1100,1680,310),
        FVector(600,1660,310),FVector(-1600,1100,310),FVector(-1300,-1080,310),
        FVector(-3000,1500,310),FVector(3350,-1250,310),
        FVector(1200,1100,310),FVector(0,1100,310),FVector(600,-1080,310),FVector(-2700,-1080,310) };
    for(const FVector& Position:RoomLights)
    {
        auto* Light=GetWorld()->SpawnActor<APointLight>(Position,FRotator::ZeroRotator);
        auto* Point=Cast<UPointLightComponent>(Light->GetLightComponent());
        Point->SetIntensity(180);
        Point->SetAttenuationRadius(800);
        Point->SetLightColor(FLinearColor(.86f,.92f,1));
        Point->SetCastShadows(true);
        FacilityLights.Add(Light);
    }
    // Arrival lift banks explain visitor reception. They are scenic shafts,
    // never presented as working escape routes in this disaster scenario.
    if(!ALZVentNetwork::IsNewLayout()) for(int32 I=0;I<2;++I)
        SpawnBlock(TEXT("OfflineArrivalLift"),FVector(-3300+I*420,2035,145),FVector(240,25,290),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("CollapsedWestStairDoor"),FVector(-3740,-1040,145),FVector(30,220,290),FRotator::ZeroRotator,Service);
    SpawnBlock(TEXT("WestStairCollapse"),FVector(-3630,-1040,75),FVector(160,240,150),FRotator(0,0,12),Wall);
    auto Sign = [&](const TCHAR* Text,FVector Pos,float Yaw)
    {
        auto* A=GetWorld()->SpawnActor<ATextRenderActor>(Pos,FRotator(0,Yaw,0));
        A->GetTextRender()->SetText(FText::FromString(Text));
        A->GetTextRender()->SetWorldSize(23);
        A->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
        A->GetTextRender()->SetTextRenderColor(FColor(185,225,240));
    };
    if(!ALZVentNetwork::IsNewLayout()) Sign(TEXT("01 RECEPTION / LIFTS OFFLINE"),FVector(-3070,2000,320),-90);
    Sign(TEXT("WC"),FVector(-3200,-1160,285),90);
    Sign(TEXT("CLEANER"),FVector(-2450,-1160,285),90);
    Sign(TEXT("MEETING"),FVector(ALZVentNetwork::IsNewLayout()?-950:-1450,-1160,285),90);
    Sign(TEXT("PANTRY"),FVector(300,-1160,285),90);
    Sign(TEXT("MANAGER / SOP-17"),FVector(-1100,1310,285),-90);
    Sign(TEXT("RECORDS / OPTIONAL"),FVector(-450,1310,285),-90);
    Sign(TEXT("ARCHIVE"),FVector(500,1310,285),-90);
    if(ALZVentNetwork::IsNewLayout())
    {
        Sign(TEXT("IT / SERVER"),FVector(3190,-1540,285),0);
        Sign(TEXT("C / ELECTRICAL"),FVector(3380,860,285),-90);
        Sign(TEXT("D / ELECTRICAL"),FVector(2160,1150,285),180);
        Sign(TEXT("A / SERVICES"),FVector(300,-710,285),90);
        Sign(TEXT("B / ADMIN"),FVector(300,860,285),-90);
    }
    else
    {
        Sign(TEXT("IT / FUSE SPARES"),FVector(1850,-855,285),90);
        Sign(TEXT("SERVICE / RELEASE INSIDE"),FVector(1190,-1100,285),0);
        Sign(TEXT("ELECTRICAL"),FVector(3000,855,285),-90);
    }
    Sign(TEXT("WEST STAIR / COLLAPSED"),FVector(-3610,-1040,280),0);
    Sign(TEXT("RESCUE LIFT / POWER REQUIRED"),FVector(3460,0,310),180);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    FFileHelper::SaveStringArrayToFile(Plan,*(FPaths::ProjectSavedDir()/TEXT("LZOfficeLayout.csv")));
#endif
}
