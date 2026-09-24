#include "LZGarageSlice.h"
#include "LZGarageEscapeQA.h"
#include "LZPickupTruck.h"
#include "LZRunState.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "LZHearingAI.h"
#include "LZCrowbarVisual.h"
#include "LZStealthSettings.h"
#include "LZAcoustics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LZOpeningSettings.h"
#include "Engine/TextRenderActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
bool ALZGarageSlice::RetryPending=false;
bool ALZGarageSlice::IsMap(const UObject* C){return C && C->GetWorld() && C->GetWorld()->GetMapName().Contains(TEXT("L_GarageEscape"));}
ALZGarageControl::ALZGarageControl(){Glow->SetIntensity(0);Label->SetVisibility(false);}
void ALZGarageControl::Setup(ALZGarageSlice* InSlice,int K,FVector Size){Slice=InSlice;Kind=K;Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Mesh->SetWorldScale3D(Size/100);Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));}
FString ALZGarageControl::GetInteractionPrompt(const ALZCharacter* P)const{return LZCrowbarVisual::InteractionKey()+(Kind==0?TEXT("撬开被椅子顶住的门"):Kind==1?TEXT("恢复卷帘门升起"):TEXT("重试车库遭遇"));}
void ALZGarageControl::Interact(ALZCharacter* P){if(Slice && FVector::Dist(P->GetActorLocation(),GetActorLocation())<350)Slice->UseControl(Kind,P);}
ALZGarageSlice::ALZGarageSlice(){PrimaryActorTick.bCanEverTick=true;}
void ALZGarageSlice::BeginPlay(){
 Super::BeginPlay();GM=GetWorld()->GetAuthGameMode<ALZGameMode>();GM->GarageSlice=this;Build();
 Player=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));if(Player && !bSeamlessArrival){
 Player->SetActorLocation(FVector(310,1170,100));
 if(auto* PC=Cast<APlayerController>(Player->GetController())){PC->SetControlRotation(FRotator(0,-18,0));PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=false;}
 auto* Run=GetGameInstance<ULZRunState>();if(!Run || !Run->Restore(Player)){Player->AcquireCrowbar();Player->TryStoreItem(ELZInventoryItemType::Scrap,4);}LastFoot=Player->GetActorLocation();}
 if(FParse::Param(FCommandLine::Get(),TEXT("GarageGuidanceQA")) || FParse::Param(FCommandLine::Get(),TEXT("GarageEscapeQA")) || FParse::Param(FCommandLine::Get(),TEXT("GarageMechanicsQA")))GetWorld()->SpawnActor<ALZGarageEscapeQA>();
 if(FParse::Param(FCommandLine::Get(),TEXT("GarageBeforeGate"))){SetBoothOpen();Player->SetActorLocation(FVector(5220,240,100));Cast<APlayerController>(Player->GetController())->SetControlRotation(FRotator(-8,155,0));}
 RetryPending=false;
}
void ALZGarageSlice::Build(){
 const FLinearColor Concrete(.22f,.26f,.28f),Paint(.72f,.58f,.2f),White(.64f,.68f,.68f);
 auto Block=[&](const TCHAR* N,FVector P,FVector Size,FLinearColor Color=FLinearColor(.22f,.26f,.28f),FRotator R=FRotator::ZeroRotator){return GM->SpawnBlock(N,P,Size,R,Color);};
 auto Light=[&](FVector P,float Power,float Radius,FLinearColor Color){auto* A=GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator);auto* L=Cast<UPointLightComponent>(A->GetLightComponent());L->SetIntensity(Power);L->SetAttenuationRadius(Radius);L->SetLightColor(Color);L->SetCastShadows(false);};
 auto Label=[&](FVector P,const TCHAR* T,float Size,FRotator R=FRotator(0,180,0)){auto* A=GetWorld()->SpawnActor<ATextRenderActor>(P,R);A->GetTextRender()->SetText(FText::FromString(T));A->GetTextRender()->SetWorldSize(Size);A->GetTextRender()->SetTextRenderColor(FColor(230,208,135));A->GetTextRender()->SetHorizontalAlignment(EHTA_Center);};
 Block(TEXT("G2_Floor"),FVector(2400,0,-25),FVector(4800,3200,50));Block(TEXT("G2_Ceiling"),FVector(2400,0,460),FVector(4800,3200,35));
 Block(TEXT("G2_West"),FVector(0,0,220),FVector(30,3200,440));Block(TEXT("G2_North"),FVector(2400,-1600,220),FVector(4800,30,440));Block(TEXT("G2_South"),FVector(2400,1600,220),FVector(4800,30,440));
 Block(TEXT("G2_EastUpper"),FVector(4800,-880,220),FVector(30,1440,440));Block(TEXT("G2_EastLower"),FVector(4800,1425,220),FVector(30,350,440));
 Block(TEXT("G2_ArrivalSide"),FVector(620,1400,150),FVector(20,400,300));Block(TEXT("G2_ArrivalBack"),FVector(240,1550,150),FVector(380,20,300));
 Block(TEXT("G2_LiftPanel"),FVector(180,1510,145),FVector(220,12,290),FLinearColor(.07f,.1f,.11f));Label(FVector(200,1470,315),TEXT("B1 / ARRIVAL"),25,FRotator(0,-90,0));
 auto StaticCar=[&](FVector P,FRotator R){Block(TEXT("G2_StaticSedan"),P+FVector(0,0,65),FVector(430,185,110),FLinearColor(.11f,.15f,.17f),R);Block(TEXT("G2_StaticCab"),P+FVector(0,0,139),FVector(200,165,65),FLinearColor(.08f,.1f,.12f),R);};
 for(int I=0;I<3;++I)StaticCar(FVector(320+I*540,-1290,0),FRotator(0,90,0));
 for(int I=0;I<4;++I)StaticCar(FVector(1300+I*710,1430,0),FRotator::ZeroRotator);
 // Two islands: 7 m between their marked boundaries; no raised kerbs.
 for(int I=0;I<2;++I){const float X=1850+I*1200;
  StaticCar(FVector(X-110,110,0),FRotator(0,90,0));
  auto* Col=Block(TEXT("G2_ChargeColumn"),FVector(X+155,80,220),FVector(90,90,440));Col->Tags.Add(TEXT("ChargeColumn"));
  Block(TEXT("G2_ColumnBand"),FVector(X+155,80,100),FVector(94,94,45),Paint)->SetActorEnableCollision(false);
  for(int Side:{-1,1}){Block(TEXT("G2_IslandLine"),FVector(X+Side*250,100,1),FVector(5,600,2),Paint)->SetActorEnableCollision(false);Block(TEXT("G2_IslandLine"),FVector(X,100+Side*300,1),FVector(500,5,2),Paint)->SetActorEnableCollision(false);}
 }
 // Parking marks retain the garage axes while only the pickup is rotated.
 for(int Side:{-1,1})Block(TEXT("G2_MaintBayLine"),FVector(950+Side*160,500,1),FVector(5,570,2),Paint)->SetActorEnableCollision(false);
 Block(TEXT("G2_MaintBayEnd"),FVector(950,210,1),FVector(325,5,2),Paint)->SetActorEnableCollision(false);
 Truck=GetWorld()->SpawnActor<ALZPickupTruck>(FVector(960,510,108),FRotator(0,-55,0));Truck->Slice=this;

 // Northern rooms; openings face the driving loop.
 for(float X:{2000.f,3000.f})Block(TEXT("G2_BossRoomSide"),FVector(X,-1280,210),FVector(22,640,420));
 Block(TEXT("G2_BossFrontL"),FVector(2160,-960,210),FVector(320,22,420));Block(TEXT("G2_BossFrontR"),FVector(2840,-960,210),FVector(320,22,420));
 BossGate=Block(TEXT("G2_BossGate"),FVector(2500,-960,180),FVector(360,25,360),FLinearColor(.28f,.13f,.09f));
 Label(FVector(2490,-925,390),TEXT("REPAIR / KEEP CLEAR"),24,FRotator(0,90,0));
 // Exit booth sits OUTSIDE the original 48 x 32 m footprint, north of the ramp.
 Block(TEXT("G2_BoothFloor"),FVector(5050,120,-25),FVector(500,560,50));
 Block(TEXT("G2_BoothUpperClosure"),FVector(4800,120,397),FVector(20,560,126));
 Block(TEXT("G2_BoothRoof"),FVector(5050,120,345),FVector(530,590,20));
 Block(TEXT("G2_BoothNorth"),FVector(5050,-160,165),FVector(500,20,330));
 Block(TEXT("G2_BoothEast"),FVector(5300,120,165),FVector(20,560,330));
 auto Glass=[&](const TCHAR* N,FVector P,FVector Size){auto* A=Block(N,P,Size);
 auto* M=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/M_ObservationGlass.M_ObservationGlass")),A);
 A->GetStaticMeshComponent()->SetMaterial(0,M);A->GetStaticMeshComponent()->SetCastShadow(false);return A;};
 Block(TEXT("G2_BoothWestNorth"),FVector(4800,-80,165),FVector(20,160,330));
 Block(TEXT("G2_BoothWestSill"),FVector(4800,290,40),FVector(20,220,80));
 Glass(TEXT("G2_BoothWestGlass"),FVector(4800,290,202),FVector(4,220,244));
 Block(TEXT("G2_BoothSouthSill"),FVector(5050,400,40),FVector(500,20,80));
 Glass(TEXT("G2_BoothSouthGlass"),FVector(5050,400,202),FVector(500,4,244));
 for(float X:{4800.f,5050.f,5300.f})Block(TEXT("G2_BoothMullion"),FVector(X,400,205),FVector(8,12,250));
 Block(TEXT("G2_BoothGateJamb"),FVector(4800,425,220),FVector(30,50,440));
 Block(TEXT("G2_BoothDoorHeader"),FVector(4800,90,312),FVector(20,180,36));
 DutyDoor=GetWorld()->SpawnActor<ALZGarageControl>(FVector(4800,90,145),FRotator::ZeroRotator);DutyDoor->Setup(this,0,FVector(18,180,290));
 DutyDoor->FindComponentByClass<UStaticMeshComponent>()->SetMobility(EComponentMobility::Movable);
 // Glass in upper half makes the overturned seat behind the lower leaf readable from adjacent windows.
 BlockingChair=GM->SpawnArtMesh(TEXT("G2_OverturnedChair"),TEXT("/Game/Art/KenneyFurniture/SM_chairDesk.SM_chairDesk"),FVector(4875,90,40),FRotator(0,0,70),FVector(.19f));BlockingChair->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);ChairClosed=BlockingChair->GetActorLocation();
 Block(TEXT("G2_ControlDesk"),FVector(5070,340,43),FVector(160,75,86),FLinearColor(.14f,.17f,.18f));
 Button=GetWorld()->SpawnActor<ALZGarageControl>(FVector(5120,312,112),FRotator::ZeroRotator);Button->Setup(this,1,FVector(24,22,22));
 auto* ButtonPaint=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),Button);ButtonPaint->SetVectorParameterValue(TEXT("Color"),FLinearColor(.06f,.38f,.16f));Button->FindComponentByClass<UStaticMeshComponent>()->SetMaterial(0,ButtonPaint);
 Block(TEXT("G2_ButtonMount"),FVector(5120,312,98),FVector(44,40,6),FLinearColor(.35f,.36f,.32f));
 Label(FVector(5120,298,143),TEXT("^"),22,FRotator(0,-90,0));
 Block(TEXT("G2_InterruptedOperatorTorso"),FVector(5160,-65,26),FVector(85,42,40),FLinearColor(.09f,.12f,.16f),FRotator(0,20,0));
 Block(TEXT("G2_InterruptedOperatorHead"),FVector(5210,-50,24),FVector(23,22,24),FLinearColor(.26f,.20f,.16f));
 const float Gap=FMath::Clamp(GetDefault<ULZGarageSliceSettings>()->ShutterInitialGap,5.f,35.f);
 Shutter=Block(TEXT("G2_ExitShutter"),FVector(4790,850,200+Gap),FVector(30,800,400),FLinearColor(.28f,.27f,.22f));Shutter->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 BossGate->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
 Label(FVector(4745,850,430),TEXT("EXIT"),28);
 // Neutral/warm pools, fixed shadowed sources; low-output fills retain collision readability.
 auto Spot=[&](FVector P,FRotator R,float Power,float Radius,float Cone,FLinearColor Color){auto* A=GetWorld()->SpawnActor<ASpotLight>(P,R);auto* L=Cast<USpotLightComponent>(A->GetLightComponent());L->SetIntensityUnits(ELightUnits::Unitless);L->SetMobility(EComponentMobility::Movable);L->SetWorldRotation(R);L->SetIntensity(Power*3);L->SetAttenuationRadius(Radius);L->SetInnerConeAngle(Cone*.55f);L->SetOuterConeAngle(Cone);L->SetLightColor(Color);L->SetCastShadows(true);return L;};
 Spot(FVector(260,1310,340),FRotator(-75,-40,0),95,750,55,FLinearColor(.88f,.84f,.70f));
 Spot(FVector(600,540,370),FRotator(-60,-10,0),180,1050,50,FLinearColor(1,.82f,.55f));
 Spot(FVector(2400,-650,415),FRotator(-65,90,0),170,1600,70,FLinearColor(.78f,.83f,.86f));
 Spot(FVector(3200,1050,415),FRotator(-75,-80,0),150,1500,70,FLinearColor(.86f,.82f,.72f));
 Spot(FVector(2510,-800,370),FRotator(-50,-90,0),200,950,58,FLinearColor(.90f,.75f,.55f));
 Spot(FVector(5050,160,315),FRotator(-75,80,0),160,520,65,FLinearColor(.88f,.84f,.70f));
 // Real shutter geometry blocks this exterior source: widening gap physically admits more light.
 Spot(FVector(5130,850,270),FRotator(-12,180,0),650,2400,60,FLinearColor(.75f,.83f,.92f));
 // Broad neutral bounce keeps every driving lane readable; accent spots retain the hierarchy.
 for(FVector P:{FVector(700,800,310),FVector(850,-950,310),FVector(2350,700,330),FVector(2350,-1100,330),FVector(3900,800,330),FVector(4050,-900,330)})Light(P,1100,2200,FLinearColor(.82f,.83f,.80f));
 Light(FVector(5050,100,210),180,700,FLinearColor(.86f,.84f,.78f));
 for(FVector P:{FVector(260,1310,350),FVector(600,540,390),FVector(2400,-650,433),FVector(3200,1050,433),FVector(2510,-800,390),FVector(5050,160,330)})Block(TEXT("G2_LampHousing"),P,FVector(65,28,8),FLinearColor(.5f,.49f,.42f))->SetActorEnableCollision(false);
 GarageLook=GetWorld()->SpawnActor<APostProcessVolume>();GarageLook->bUnbound=true;GarageLook->Priority=5;
 auto& PP=GarageLook->Settings;PP.bOverride_AutoExposureMinBrightness=PP.bOverride_AutoExposureMaxBrightness=true;PP.AutoExposureMinBrightness=PP.AutoExposureMaxBrightness=-5.3f;
 PP.bOverride_VignetteIntensity=true;PP.VignetteIntensity=.10f;PP.bOverride_BloomIntensity=true;PP.BloomIntensity=.12f;
 PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
 Boss=GetWorld()->SpawnActor<ALZGarageCharger>(FVector(2500,-1320,150),FRotator(0,90,0));Boss->Slice=this;Boss->SetActorScale3D(FVector(1.55f));Boss->GetCharacterMovement()->bEnablePhysicsInteraction=false;
 // 24 m / +5 m ramp. Road starts at its actual upper surface, no height scripting.
 const float Angle=FMath::RadiansToDegrees(FMath::Atan2(500.f,2400.f));
 Block(TEXT("G2_Ramp"),FVector(6000,850,220),FVector(2452,800,60),Concrete,FRotator(Angle,0,0));
 Block(TEXT("G2_RampGuardSouth"),FVector(6000,1265,335),FVector(2470,25,250),Concrete,FRotator(Angle,0,0));
 // Start the north guard past the booth, so it cannot mask the shutter from the console.
 Block(TEXT("G2_RampGuardNorth"),FVector(6250,435,387.1f),FVector(1942,25,250),Concrete,FRotator(Angle,0,0));
 // Road is wide enough to choose either side of the soft chicane. No track or steering assist.
 Block(TEXT("G2_SurfaceRoad"),FVector(28200,850,475),FVector(42000,2400,50),FLinearColor(.11f,.13f,.14f));
 for(int Side:{-1,1})Block(TEXT("G2_RoadEdge"),FVector(28200,850+Side*1210,650),FVector(42000,25,300),Concrete);
 for(int I=0;I<35;++I)Block(TEXT("G2_RoadDash"),FVector(7600+I*1200,850,502),FVector(260,12,2),White)->SetActorEnableCollision(false);
 Block(TEXT("G2_WideDetour"),FVector(26800,200,670),FVector(1800,900,340),Concrete,FRotator(0,-8,0));
 for(int I=0;I<15;++I){const float X=8500+I*2300;const float Y=850+(I%3-1)*720;
  auto* E=GetWorld()->SpawnActor<ALZEnemy>(FVector(X,Y,590),FRotator(0,180,0));E->Tags.Add(TEXT("GarageRoadInfected"));}
 for(int I=0;I<18;++I){const int Side=I%2?1:-1;const float X=8000+(I/2)*4900;
  Block(TEXT("G2_BrokenCity"),FVector(X,850+Side*2300,1550+(I%3)*400),FVector(1500,1300,2100+(I%3)*800),Concrete,FRotator(0,I*8,I%4==0?8:0));}
 // Distant water tanks are recognizable from the street, with a physical end boundary.
 Block(TEXT("G2_POI_Ground"),FVector(52500,850,475),FVector(7000,12000,50),Concrete);
 Block(TEXT("G2_POI_ServiceHall"),FVector(54600,2700,1100),FVector(2400,2200,1200),FLinearColor(.2f,.36f,.36f));
 for(int I=0;I<2;++I){const FVector Tank(53000+I*2000,-1600,2700-I*400);
  auto* A=Block(TEXT("G2_POI_WaterTank"),Tank,FVector(1100,1100,1300),FLinearColor(.2f,.36f,.36f));A->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
  for(int X:{-1,1})for(int Y:{-1,1})Block(TEXT("G2_POI_TankSupport"),FVector(Tank.X+X*330,Tank.Y+Y*330,1350),FVector(65,65,1700),Concrete);
 }
 Block(TEXT("G2_POI_Checkpoint"),FVector(49100,850,630),FVector(60,2400,260),Paint);
 Label(FVector(49050,850,1120),TEXT("NEXT POI / WATERWORKS"),70);
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(8000,0,5000),FRotator(-55,-35,0));Sun->GetLightComponent()->SetIntensity(.15f);Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true);
 GetWorld()->SpawnActor<ASkyAtmosphere>();auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetIntensity(.45f);Sky->GetLightComponent()->SetCastShadows(false);Sky->GetLightComponent()->SetRealTimeCapture(true);
 if(FParse::Param(FCommandLine::Get(),TEXT("GarageLightingBaseline"))){
 for(TActorIterator<ASpotLight> It(GetWorld());It;++It)It->GetLightComponent()->SetIntensity(0);
 for(TActorIterator<APointLight> It(GetWorld());It;++It)It->GetLightComponent()->SetIntensity(0);
 Light(FVector(650,600,330),850,1200,FLinearColor(1,.78f,.42f));
 for(int X=600;X<4800;X+=1100)for(int Y:{-700,850})Light(FVector(X,Y,410),1350,1450,FLinearColor(.58f,.73f,.83f));
 Light(FVector(5050,160,315),1000,1300,FLinearColor(.62f,.83f,1));Sun->GetLightComponent()->SetIntensity(2);Sky->GetLightComponent()->SetIntensity(.8f);
 }
 if(!UNavigationSystemV1::GetCurrent(GetWorld()))FNavigationSystem::AddNavigationSystemToWorld(*GetWorld(),FNavigationSystemRunMode::GameMode);
 auto* Volume=GetWorld()->SpawnActor<ANavMeshBoundsVolume>(FVector(2640,0,150),FRotator::ZeroRotator);auto* Box=NewObject<UBoxComponent>(Volume);Volume->AddInstanceComponent(Box);Box->SetupAttachment(Volume->GetRootComponent());Box->SetBoxExtent(FVector(2720,1640,300));Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();if(auto* Nav=UNavigationSystemV1::GetCurrent(GetWorld()))Nav->OnNavigationBoundsUpdated(Volume);
}
void ALZGarageSlice::Say(FString Text){Feedback=Text;FeedbackUntil=GetWorld()->GetTimeSeconds()+6;}
FString ALZGarageSlice::Hint()const{
 if(GetWorld()->GetTimeSeconds()<FeedbackUntil)return Feedback;
 if(Phase==ELZGaragePhase::POI)return TEXT("已脱离封锁 · 下一站：净水厂。F5 重试车库遭遇");
 if(Phase==ELZGaragePhase::Escape)return TEXT("自由驾驶驶向净水厂 · 可绕过尸群 · F5 重试");
 if(Truck && Truck->Driving)return FString::Printf(TEXT("%.0f km/h | W/S 加速刹车倒车 · A/D 转向 · 空格驻车 · E 下车 · F5 重试"),FMath::Abs(Truck->Speed())*.036f);
 if(Phase==ELZGaragePhase::Encounter && Truck && Player)return FString::Printf(TEXT("卷帘门已开启 · 维修皮卡距当前位置 %.0f 米 · 可绕过巨型感染者撤离"),FVector::Dist2D(Truck->GetActorLocation(),Player->GetActorLocation())/100);
 return FString();
}
void ALZGarageSlice::SetBoothOpen(){DutyOpen=true;const FRotator R(0,-92,0);DutyDoor->SetActorLocation(FVector(4800,0,145)+R.RotateVector(FVector(0,90,0)));DutyDoor->SetActorRotation(R);BlockingChair->SetActorLocation(ChairClosed+FVector(275,-50,0));}
void ALZGarageSlice::UseControl(int Kind,ALZCharacter* P){
 if(Kind==0){if(DutyOpen || bBoothPrying || !P || P->IsTraversing() || P->IsInventoryOpen())return;
 if(!P->HasCrowbarTool()){Say(TEXT("门被里面的椅子顶住，需要撬棍。"));return;}
 bBoothPrying=true;PryAt=GetWorld()->GetTimeSeconds();PryOperator=P;P->SetPrying(true);
 LZAcoustics::Emit(this,DutyDoor->GetActorLocation(),120,TEXT("Rattle"),TEXT("Rattle"),.15f);}
 else if(Kind==1)OpenExit();else Retry();
}
void ALZGarageSlice::TickBooth(float Delta){
 if(!bBoothPrying)return;auto* P=PryOperator.Get();
 if(!P || P->GetHealth()<=0 || GM->IsRunOver()){if(P)P->SetPrying(false);bBoothPrying=false;DutyDoor->SetActorLocation(FVector(4800,90,145));DutyDoor->SetActorRotation(FRotator::ZeroRotator);BlockingChair->SetActorLocation(ChairClosed);return;}
 const float T=FMath::Clamp((GetWorld()->GetTimeSeconds()-PryAt)/FMath::Max(.6f,GetDefault<ULZGarageSliceSettings>()->BoothPrySeconds),0.f,1.f);P->UpdatePryPose(T);
 // Shift the blocking seat first; only then rotate the leaf around its north hinge.
 BlockingChair->SetActorLocation(FMath::Lerp(ChairClosed,ChairClosed+FVector(275,-50,0),FMath::Clamp(T/.55f,0.f,1.f)));
 const FRotator R(0,-92*FMath::Clamp((T-.55f)/.45f,0.f,1.f),0);DutyDoor->SetActorLocation(FVector(4800,0,145)+R.RotateVector(FVector(0,90,0)));DutyDoor->SetActorRotation(R);
 if(T>=1){SetBoothOpen();P->SetPrying(false);PryOperator=nullptr;bBoothPrying=false;LZAcoustics::Emit(this,DutyDoor->GetActorLocation(),GetDefault<ULZOpeningSettings>()->PopRadius,TEXT("Clang"),TEXT("MetalImpact"),.6f);}
}
void ALZGarageSlice::OpenExit(){if(Phase!=ELZGaragePhase::Preparation || !DutyOpen)return;
 Phase=ELZGaragePhase::Encounter;++GateActivations;OpenAt=GetWorld()->GetTimeSeconds();BossRevealAt=OpenAt+GetDefault<ULZGarageSliceSettings>()->BossRevealDelay;
 LZAcoustics::Emit(this,FVector(4650,850,110),16000,TEXT("Motor"),TEXT("Crash"),.8f);
 LZAcoustics::Emit(this,FVector(4650,850,110),16000,TEXT("Clang"),TEXT("Crash"),.7f);
}
void ALZGarageSlice::Retry(){ALZGarageSlice::RetryPending=true;GM->RestartRun();}
void ALZGarageSlice::Tick(float Delta){
 Super::Tick(Delta);TickBooth(Delta);if(!Player || GM->IsRunOver())return;const float Now=GetWorld()->GetTimeSeconds();
 const auto* Settings=GetDefault<ULZGarageSliceSettings>();
 if(GarageLook){const float X=Truck->Driving?Truck->GetActorLocation().X:Player->GetActorLocation().X;GarageLook->BlendWeight=FParse::Param(FCommandLine::Get(),TEXT("GarageLightingBaseline"))?0:1-FMath::SmoothStep(5600.f,7800.f,X);}
 if(Phase==ELZGaragePhase::Preparation){
 if(!bRadioPlayed && FVector::Dist2D(Player->GetActorLocation(),Truck->GetActorLocation())<260){bRadioPlayed=true;
 if(LZAcoustics::Emit(this,Truck->GetActorLocation(),0,TEXT("RadioCall"),TEXT("Narrative"),.25f))Say(TEXT("[临时对讲机音效] 维修车……还没出来？坡道这边撑不了多久……"));}
 if(Now>NextIdleNoise){NextIdleNoise=Now+Settings->ShutterIdleInterval;LZAcoustics::Emit(this,Shutter->GetActorLocation(),0,TEXT("Rattle"),TEXT("Narrative"),.20f);}}
 if(OpenAt>=0){const float T=FMath::Clamp((Now-OpenAt)/FMath::Max(1.f,Settings->ShutterOpenSeconds),0.f,1.f);const float Gap=FMath::Clamp(Settings->ShutterInitialGap,5.f,35.f);Shutter->SetActorLocation(FVector(4790,850,200+FMath::Lerp(Gap,440.f,T)));if(T>=1)Shutter->SetActorEnableCollision(false);
 if(!bBossResponse && Now>OpenAt+1){bBossResponse=true;LZAcoustics::Emit(this,BossGate->GetActorLocation(),2400,TEXT("Crash"),TEXT("BossResponse"),.8f);}
 if(BossRevealAt>0 && Now>=BossRevealAt){BossRevealAt=-1;BossGate->SetActorEnableCollision(false);BossGate->SetActorHiddenInGame(true);if(IsValid(Boss)){Boss->Active=true;
 // Audible scrape at the accessible inside threshold, not the raised door mesh in the ceiling.
 // Crash priority uses the shared finite investigation commitment, so quiet footsteps cannot steal it.
 LZAcoustics::Emit(this,FVector(4650,850,110),16000,TEXT("Clang"),TEXT("Crash"),.8f);}}}
 if(Truck && Truck->Driving && Truck->GetActorLocation().X>7250 && Phase==ELZGaragePhase::Encounter)Phase=ELZGaragePhase::Escape;
 if(Truck && Truck->Driving && Truck->GetActorLocation().X>47500 && Phase==ELZGaragePhase::Escape)Phase=ELZGaragePhase::POI;
 const FVector P=Player->GetActorLocation();const float Moved=FVector::Dist2D(P,LastFoot);LastFoot=P;
 if(!Truck->Driving && Moved<150){FootDistance+=Moved;const auto* S=GetDefault<ULZStealthSettings>();if(Moved>.1f && FootDistance>95 && Now>NextFoot){FootDistance=0;NextFoot=Now+.4f;LZAcoustics::Emit(Player,P,Player->bIsCrouched?S->CrouchRadius:Player->IsSprinting()?S->SprintRadius:S->WalkRadius,TEXT("Step"),Player->IsSprinting()?TEXT("Sprint"):TEXT("Walk"),.4f);}}
 else FootDistance=0;
}
ALZGarageCharger::ALZGarageCharger(){MaxHealth=Health=400;HearingSensitivity=1.6f;SearchDuration=10;MoveSpeed=260;Damage=25;AttackCooldown=1.6f;}
void ALZGarageCharger::BeginPlay(){Super::BeginPlay();MaxHealth=Health=400;MoveSpeed=260;Damage=25;AttackCooldown=1.6f;GetCharacterMovement()->MaxWalkSpeed=260;}
void ALZGarageCharger::BeginCharge(FVector Target){if(!Active || Action || IsIncapacitated())return;Action=1;ChargeTarget=Target;Until=GetWorld()->GetTimeSeconds()+GetDefault<ULZGarageSliceSettings>()->WindupSeconds;bHitThisCharge=false;
 GetCharacterMovement()->StopMovementImmediately();if(auto* C=Cast<ALZHearingController>(GetController()))C->StopMovement();LZAcoustics::Emit(this,GetActorLocation(),1800,TEXT("Howl"),TEXT("BossWindup"),.8f);}
void ALZGarageCharger::Tick(float Delta){
 Super::Tick(Delta);auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();if(!Active || !GM || GM->IsRunOver() || IsDead())return;const float Now=GetWorld()->GetTimeSeconds();
 if(IsIncapacitated()){if(Action){Action=0;NextCharge=Now+4;}return;}
 const auto* S=GetDefault<ULZGarageSliceSettings>();
 if(Action==0){if(HasHeardNoise() && Now>NextCharge){const FVector Target=GetHeardLocation();const float D=FVector::Dist2D(Target,GetActorLocation());FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(BossChargeLOS),false,this);
 if(D>400 && D<1900 && !GetWorld()->LineTraceSingleByChannel(H,GetActorLocation(),FVector(Target.X,Target.Y,GetActorLocation().Z),ECC_WorldStatic,Q))BeginCharge(Target);}}
 else if(Action==1){GetMesh()->SetRelativeRotation(FRotator(-18*FMath::Sin(FMath::Clamp((S->WindupSeconds-(Until-Now))/S->WindupSeconds,0.f,1.f)*PI),-90,0));GetCharacterMovement()->StopMovementImmediately();SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),FRotator(0,(ChargeTarget-GetActorLocation()).Rotation().Yaw,0),Delta,35));
 if(Now>=Until){GetMesh()->SetRelativeRotation(FRotator(0,-90,0));Action=2;ChargeDirection=GetActorForwardVector();Until=Now+1.7f;GetCharacterMovement()->DisableMovement();}}
 else if(Action==2){GetCharacterMovement()->Velocity=ChargeDirection*S->ChargeSpeed;FHitResult H;SetActorLocation(GetActorLocation()+ChargeDirection*S->ChargeSpeed*Delta,true,&H);
 if(H.bBlockingHit){if(H.GetActor() && H.GetActor()->ActorHasTag(TEXT("ChargeColumn")))++ColumnStuns;
 if(!bHitThisCharge)if(auto* P=Cast<ALZCharacter>(H.GetActor())){UGameplayStatics::ApplyDamage(P,30,GetController(),this,nullptr);++ChargeHits;bHitThisCharge=true;}
 Action=3;Until=Now+S->StunSeconds;LZAcoustics::Emit(this,H.ImpactPoint,1400,TEXT("Clang"),TEXT("Crash"),.8f);}
 else if(Now>=Until){Action=3;Until=Now+2;}}
 else if(Now>=Until){Action=0;NextCharge=Now+5;GetCharacterMovement()->SetMovementMode(MOVE_Walking);}
}
