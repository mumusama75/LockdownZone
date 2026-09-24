#include "LZGarage.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Components/BoxComponent.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
ALZGarageBoss::ALZGarageBoss()
{
 bUseInfectedVisuals=false;
 HearingSensitivity=1.4f;SearchDuration=18;MaxHealth=Health=600;MoveSpeed=250;Damage=22;AttackCooldown=1.6f;
}
void ALZGarageBoss::BeginPlay()
{Super::BeginPlay();MaxHealth=Health=600;Damage=22;AttackCooldown=1.6f;}
void ALZGarageBoss::Tick(float Delta)
{
 Super::Tick(Delta);
 if(Garage)GetCharacterMovement()->MaxWalkSpeed=Garage->State==ELZGarageState::Driving?290:250;
}
float ALZGarageBoss::TakeDamage(float Amount,const FDamageEvent& Event,AController* Source,AActor* Causer)
{
 if(!Garage || Amount<=0)return 0;
 if(Causer==Garage && Garage->State==ELZGarageState::Driving && Garage->Speed>=650)
     return Super::TakeDamage(10000,Event,Source,Causer);
 // Firearms buy retreat time but cannot bypass the vehicle finale.
 StaggerUntil=GetWorld()->GetTimeSeconds()+.65f;
 return Super::TakeDamage(FMath::Min(Amount*.3f,FMath::Max(0.f,Health-1)),Event,Source,Causer);
}
ALZGarage::ALZGarage(){PrimaryActorTick.bCanEverTick=true;}
void ALZGarage::Start(ALZChapter* C,ALZCharacter* P)
{
 Chapter=C;Player=P;GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 for(TActorIterator<ALZEnemy> It(GetWorld());It;++It)It->Destroy();
 Build();
 auto* Volume=GetWorld()->SpawnActor<ANavMeshBoundsVolume>(World(FVector(2800,0,180)),FRotator::ZeroRotator);
 auto* Bounds=NewObject<UBoxComponent>(Volume);Bounds->SetupAttachment(Volume->GetRootComponent());Bounds->SetBoxExtent(FVector(3400,1700,450));Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bounds->SetCanEverAffectNavigation(false);Volume->AddInstanceComponent(Bounds);Bounds->RegisterComponent();
 if(auto* Nav=UNavigationSystemV1::GetCurrent(GetWorld()))Nav->OnNavigationBoundsUpdated(Volume);
 Player->GetCharacterMovement()->StopMovementImmediately();Player->StopCrouch();
 Player->SetActorLocation(World(FVector(0,0,90)));
 Cast<APlayerController>(Player->GetController())->SetControlRotation(FRotator(0,0,0));
 Chapter->Noise(World(FVector(-150,0,150)),0,TEXT("Relay"));
}
void ALZGarage::Build()
{
 const FLinearColor Concrete(.13f,.15f,.16f), Yellow(.65f,.38f,.04f), White(.7f,.72f,.65f);
 auto Block=[&](const TCHAR* Name,FVector Pos,FVector Size,FLinearColor Color=FLinearColor(.13f,.15f,.16f),FRotator Rot=FRotator::ZeroRotator)
 {return GM->SpawnBlock(Name,World(Pos),Size,Rot,Color);};
 auto Light=[&](FVector Pos,float Intensity,float Radius,FLinearColor Color)
 {auto* A=GetWorld()->SpawnActor<APointLight>(World(Pos),FRotator::ZeroRotator);auto* L=Cast<UPointLightComponent>(A->GetLightComponent());L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetLightColor(Color);};
 auto Label=[&](FVector Pos,const TCHAR* Text,float Size)
 {auto* A=GetWorld()->SpawnActor<ATextRenderActor>(World(Pos),FRotator(0,180,0));A->GetTextRender()->SetText(FText::FromString(Text));A->GetTextRender()->SetWorldSize(Size);A->GetTextRender()->SetHorizontalAlignment(EHTA_Center);};
 Block(TEXT("GarageFloor"),FVector(2700,0,-35),FVector(6600,3400,70));
 Block(TEXT("GarageCeiling"),FVector(2700,0,460),FVector(6600,3400,50));
 Block(TEXT("GarageBack"),FVector(-580,0,210),FVector(40,3400,420));
 for(int32 Side:{-1,1})
 {
   Block(TEXT("GarageBoundary"),FVector(2700,Side*1680,210),FVector(6600,40,420));
   for(int32 I=0;I<6;++I)
   {
     Block(TEXT("GarageColumn"),FVector(400+I*1000,Side*850,210),FVector(70,70,420));
     Block(TEXT("ColumnHazardBand"),FVector(400+I*1000,Side*850,100),FVector(74,74,35),Yellow);
     Block(TEXT("ParkingBayLine"),FVector(200+I*900,Side*1230,2),FVector(12,800,4),White);
     if(I%2==0){Block(TEXT("AbandonedCar"),FVector(500+I*900,Side*1250,65),FVector(450,210,100),FLinearColor(.07f,.1f,.12f));Block(TEXT("AbandonedCarCab"),FVector(470+I*900,Side*1250,135),FVector(210,185,70));}
   }
 }
 Block(TEXT("LiftRearPanel"),FVector(-400,0,150),FVector(25,260,300));
 Label(FVector(-360,0,330),TEXT("B1"),40);
 Label(FVector(800,-700,250),TEXT("P  /  EXIT  >"),28);
 // Vehicle is seen across the aisle before the boss encounter trigger.
 Vehicle=Block(TEXT("EscapeUtilityVehicle"),FVector(3000,0,85),FVector(440,220,100),FLinearColor(.12f,.25f,.20f));
 auto Part=[&](const TCHAR* Name,FVector Offset,FVector Size,FLinearColor Color)
 {auto* A=Block(Name,FVector(3000,0,85)+Offset,Size,Color);A->SetActorEnableCollision(false);A->AttachToActor(Vehicle,FAttachmentTransformRules::KeepWorldTransform);return A;};
 Part(TEXT("VehicleCab"),FVector(-20,0,92),FVector(200,195,90),FLinearColor(.04f,.075f,.08f));
 Part(TEXT("VehicleRoof"),FVector(-20,0,140),FVector(230,220,12),Yellow);
 Part(TEXT("VehicleRamBar"),FVector(240,0,0),FVector(35,240,60),Concrete);
 for(int32 Side:{-1,1})for(int32 End:{-1,1})
 {auto* Wheel=Part(TEXT("VehicleWheel"),FVector(End*140,Side*112,-45),FVector(75,35,75),FLinearColor(.015f,.015f,.015f));}
 for(int32 Side:{-1,1})Part(TEXT("Headlamp"),FVector(223,Side*70,12),FVector(8,42,24),White);
 Vehicle->GetStaticMeshComponent()->SetCollisionObjectType(ECC_Vehicle);
 Vehicle->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
 Vehicle->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Ignore);
 VehicleNode=Chapter->Add(ELZChapterNode::Vehicle,World(FVector(2920,-165,105)),FVector(20,25,35),FColor(210,165,60));
 Chapter->Add(ELZChapterNode::Pistol,World(FVector(200,-270,92)),FVector(24,10,16),FColor(90,135,145));
 Block(TEXT("SecurityDesk"),FVector(200,-270,38),FVector(140,100,76));
 for(int32 I=0;I<3;++I)Chapter->Add(ELZChapterNode::Ammo,World(FVector(500+I*500,-400,70)),FVector(30,20,20),FColor(190,150,55));
 Chapter->Add(ELZChapterNode::Medical,World(FVector(1900,-450,60)),FVector(25,18,12),FColor(170,35,30));
 for(int32 I=0;I<7;++I)
 {Light(FVector(I*850,0,340),640,1600,FLinearColor(.6f,.72f,.8f));Block(TEXT("DriveLaneArrow"),FVector(500+I*800,0,3),FVector(180,20,6),Yellow);}
 Light(FVector(3000,0,300),600,1200,FLinearColor(.8f,.65f,.25f));
 Boss=GetWorld()->SpawnActor<ALZGarageBoss>(World(FVector(4900,0,170)),FRotator(0,180,0));Boss->Garage=this;Boss->bActive=true;
 Boss->SetActorScale3D(FVector(1.7f));Boss->GetCharacterMovement()->MaxWalkSpeed=250;
 // A low-luminance sky backdrop gives the ruined skyline a readable silhouette.
 auto* Sky=Block(TEXT("CityHorizon"),FVector(14500,0,6000),FVector(100,30000,16000));Sky->SetActorEnableCollision(false);
 Sky->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Gameplay/M_CityHorizon.M_CityHorizon")));
 // A continuous shallow exit ramp joins a street at +800 cm.
 RampSurface=Block(TEXT("GarageExitRamp"),FVector(7250,0,365),FVector(2630,1600,70),Concrete,FRotator(17.74f,0,0));
 RampSurface->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Vehicle,ECR_Ignore);
 for(int32 Side:{-1,1})Block(TEXT("RampBarrier"),FVector(7250,Side*800,550),FVector(2800,35,400),Concrete,FRotator(17.74f,0,0));
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(World(FVector(10000,0,4000)),FRotator(-35,25,0));Sun->GetLightComponent()->SetIntensity(.35f);
 Block(TEXT("CityStreet"),FVector(11250,0,760),FVector(5500,4000,80),FLinearColor(.07f,.08f,.09f))->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Vehicle,ECR_Ignore);
 for(int32 I=0;I<12;++I)
 {
   const int32 Side=I%2?1:-1;const float X=9000+(I/2)*750;const float H=1100+(I%3)*650;
   Block(TEXT("CityBrokenTower"),FVector(X,Side*(2200+(I%3)*230),800+H/2),FVector(580,600,H),FLinearColor(.16f,.19f,.23f),FRotator(0,I*13,I%3==0?9:0));
   Block(TEXT("TowerBrokenCrown"),FVector(X+120,Side*(2200+(I%3)*230),800+H+80),FVector(500,350,200),Concrete,FRotator(17,I*13,22));
   for(int32 J=0;J<3;++J)Block(TEXT("CityDebris"),FVector(X+J*160,Side*(1150+J*180),850+J*35),FVector(220,160,130),Concrete,FRotator(0,J*35,15));
 }
 Block(TEXT("FallenSkybridgeLeft"),FVector(12000,-1250,1650),FVector(220,1800,220),Concrete,FRotator(0,0,18));
 Block(TEXT("FallenSkybridgeRight"),FVector(12000,1300,1850),FVector(220,1600,220),Concrete,FRotator(0,0,-24));
 for(int32 I=0;I<5;++I)Light(FVector(8800+I*1100,0,2600),800,4000,FLinearColor(.45f,.6f,.8f));
 Label(FVector(5750,0,360),TEXT("EXIT  /  CITY"),42);
 DriveCamera=GetWorld()->SpawnActor<ACameraActor>();DriveCamera->GetCameraComponent()->SetFieldOfView(85);
}
bool ALZGarage::EnterVehicle(ALZCharacter* P)
{
 if(P!=Player || P->IsInventoryOpen() || P->GetHealth()<=0 || (State!=ELZGarageState::Exploring && State!=ELZGarageState::Pursuit) || FVector::Dist2D(P->GetActorLocation(),Vehicle->GetActorLocation())>380)return false;
 State=ELZGarageState::Driving;Boss->bActive=true;
 P->GetCharacterMovement()->DisableMovement();P->SetActorEnableCollision(false);P->SetActorHiddenInGame(true);
 auto* PC=Cast<APlayerController>(P->GetController());
 DriveCamera->SetActorLocation(Vehicle->GetActorLocation()+FVector(-650,0,310));DriveCamera->SetActorRotation(FRotator(-10,0,0));
 P->DisableInput(PC);PC->SetViewTargetWithBlend(DriveCamera,.35f);
 VehicleNode->Destroy();Chapter->Noise(Vehicle->GetActorLocation(),6000,TEXT("Motor"));return true;
}
FString ALZGarage::Hint() const
{
 if(State==ELZGarageState::Driving)return TEXT("W 加速  S 制动/倒车  A/D 转向 · 撞开出口");
 if(State==ELZGarageState::CityReveal)return TEXT("零号大厦之外 · 破碎城区");
 return TEXT("");
}
void ALZGarage::SetQAInput(float Throttle,float Steering)
{if(FParse::Param(FCommandLine::Get(),TEXT("LZChapterQA"))){bQAInput=true;QAThrottle=Throttle;QASteer=Steering;}}
void ALZGarage::Drive(float Delta,float Throttle,float Steering)
{
 Speed=Throttle==0?FMath::FInterpConstantTo(Speed,0.f,Delta,1100.f):FMath::Clamp(Speed+850.f*Throttle*Delta,-600.f,1800.f);
 FVector Position=Vehicle->GetActorLocation();FVector Next=Position+FVector(Speed*Delta,Steering*FMath::Clamp(Speed,-500.f,500.f)*Delta,0);
 Next.X=FMath::Clamp(Next.X,Origin.X+700,Origin.X+13000);
 Next.Y=FMath::Clamp(Next.Y,Origin.Y-520,Origin.Y+520);
 Next.Z=Origin.Z+85+FMath::Clamp((Next.X-Origin.X-6000)/2500,0.f,1.f)*800;
 if(Next.X-Origin.X>6000 && Next.X-Origin.X<8500)
 {
   FHitResult GroundHit;FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(GarageGround),true);
   if(RampSurface->GetStaticMeshComponent()->LineTraceComponent(GroundHit,Next+FVector(0,0,2000),Next-FVector(0,0,2000),GroundQuery))Next.Z=GroundHit.ImpactPoint.Z+85;
 }
 if(IsValid(Boss) && Speed>=650 && FVector::Dist2D(FMath::ClosestPointOnSegment(Boss->GetActorLocation(),Position,Next),Boss->GetActorLocation())<300)
 {UGameplayStatics::ApplyDamage(Boss,10000,Player->GetController(),this,nullptr);bBossHit=true;Chapter->Noise(Next,5000,TEXT("Crash"));}
 const float RampPitch=Next.X-Origin.X>6000 && Next.X-Origin.X<8500?17.74f:0.f;
 Vehicle->SetActorRotation(FMath::RInterpTo(Vehicle->GetActorRotation(),FRotator(RampPitch,0,0),Delta,5));
 FHitResult Hit;Vehicle->SetActorLocation(Next,true,&Hit);
 if(Hit.bBlockingHit){Speed=0;UE_LOG(LogTemp,Verbose,TEXT("GARAGE_DRIVE_BLOCKED %s at %s"),*GetNameSafe(Hit.GetActor()),*Vehicle->GetActorLocation().ToString());}
 Player->SetActorLocation(Vehicle->GetActorLocation()+FVector(0,0,130));
 FVector CameraPosition=Vehicle->GetActorLocation()+FVector(-650,0,310);
 // While the camera is still inside the garage footprint, keep it below the roof.
 if(CameraPosition.X-Origin.X<6000)CameraPosition.Z=FMath::Min(CameraPosition.Z,Origin.Z+390);
 FCollisionQueryParams CameraQuery(SCENE_QUERY_STAT(GarageCamera),false,Player);CameraQuery.AddIgnoredActor(Vehicle);
 FHitResult CameraHit;
 if(GetWorld()->SweepSingleByChannel(CameraHit,Vehicle->GetActorLocation()+FVector(0,0,160),CameraPosition,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(15),CameraQuery))CameraPosition=CameraHit.Location+CameraHit.Normal*10;
 DriveCamera->SetActorLocation(CameraPosition);
 DriveCamera->SetActorRotation((Vehicle->GetActorLocation()+FVector(450,0,100)-CameraPosition).Rotation());
 if(Vehicle->GetActorLocation().X-Origin.X>10500 && bBossHit){State=ELZGarageState::CityReveal;RevealAt=GetWorld()->GetTimeSeconds();}
}
void ALZGarage::Tick(float Delta)
{
 Super::Tick(Delta);if(!Player || !GM || GM->IsRunOver())return;
 if(State==ELZGarageState::Exploring && Player->GetActorLocation().X-Origin.X>750)
 {State=ELZGarageState::Pursuit;Boss->bActive=true;Chapter->Noise(Player->GetActorLocation(),7000,TEXT("Crash"));}
 if(State==ELZGarageState::Driving && GetWorld()->GetTimeSeconds()>NextEngineNoise)
 {NextEngineNoise=GetWorld()->GetTimeSeconds()+.6f;Chapter->Noise(Vehicle->GetActorLocation(),3200,TEXT("Motor"));}
 if(State==ELZGarageState::Driving)
 {auto* PC=Cast<APlayerController>(Player->GetController());Drive(Delta,bQAInput?QAThrottle:(PC->IsInputKeyDown(EKeys::W)?1.f:(PC->IsInputKeyDown(EKeys::S)?-1.f:0.f)),bQAInput?QASteer:((PC->IsInputKeyDown(EKeys::D)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::A)?1.f:0.f)));}
 if(State==ELZGarageState::CityReveal)
 {
   const float T=GetWorld()->GetTimeSeconds()-RevealAt;
   DriveCamera->SetActorLocation(World(FVector(9000+T*90,0,2800+T*30)));
   DriveCamera->SetActorRotation((World(FVector(12500,0,1500))-DriveCamera->GetActorLocation()).Rotation());
   if(T>7){State=ELZGarageState::Complete;Player->EnableInput(Cast<APlayerController>(Player->GetController()));GM->bObjectiveComplete=true;GM->TryExtract(Player);}
 }
}
