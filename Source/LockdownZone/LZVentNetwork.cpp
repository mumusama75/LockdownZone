#include "LZVentNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "LZChapter.h"
#include "LZCrowbarVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/PointLight.h"
#include "Engine/TextRenderActor.h"
#include "Components/TextRenderComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool ALZVentNetwork::IsNewLayout()
{return !FParse::Param(FCommandLine::Get(),TEXT("LZSliceQA")) && !FParse::Param(FCommandLine::Get(),TEXT("LZLegacy"));}
TArray<FVector> ALZVentNetwork::Outlets()
{
 return {FVector(200,1850,0)};
}
bool ALZVentNetwork::CeilingHole(FVector C,FVector S)
{
 if(!IsNewLayout())return false;
 // Meeting-room cabinet and ramp share a continuous ceiling cutout.
 if(FMath::Abs(C.Y+1350)<S.Y*.5f+170 && C.X+S.X*.5f>-2050 && C.X-S.X*.5f<-1100)return true;
 for(FVector P:Outlets())if(FMath::Abs(C.X-P.X)<S.X*.5f+95 && FMath::Abs(C.Y-P.Y)<S.Y*.5f+95)return true;
 return false;
}
TArray<FBox> ALZVentNetwork::CeilingPanels(FVector C,FVector S)
{
 TArray<FBox> Panels;Panels.Add(FBox(C-S*.5f,C+S*.5f));if(!IsNewLayout())return Panels;
 TArray<FBox> Holes;Holes.Add(FBox(FVector(-2050,-1520,-1000),FVector(-1100,-1180,1000)));
 for(FVector P:Outlets())Holes.Add(FBox(P-FVector(95,95,1000),P+FVector(95,95,1000)));
 for(FBox H:Holes){TArray<FBox> Next;for(FBox B:Panels){if(!B.Intersect(H)){Next.Add(B);continue;}
 const float X0=FMath::Max(B.Min.X,H.Min.X),X1=FMath::Min(B.Max.X,H.Max.X),Y0=FMath::Max(B.Min.Y,H.Min.Y),Y1=FMath::Min(B.Max.Y,H.Max.Y);
 auto Add=[&](float A,float D,float E,float F){if(D-A>1 && F-E>1)Next.Add(FBox(FVector(A,E,B.Min.Z),FVector(D,F,B.Max.Z)));};
 Add(B.Min.X,X0,B.Min.Y,B.Max.Y);Add(X1,B.Max.X,B.Min.Y,B.Max.Y);Add(X0,X1,B.Min.Y,Y0);Add(X0,X1,Y1,B.Max.Y);}
 Panels=MoveTemp(Next);}return Panels;
}
ALZRollingCabinet::ALZRollingCabinet(){PrimaryActorTick.bCanEverTick=true;}
void ALZRollingCabinet::Setup(ALZVentNetwork* InNetwork)
{
 Network=InNetwork;Mesh->SetWorldScale3D(FVector(1.6,1.5,1.8));SetLabel(TEXT(""),FColor(65,155,150));Glow->SetIntensity(0);
 Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetCanEverAffectNavigation(false);
 // Wheel meshes are attached at absolute scale so they follow the constrained cabinet motion.
 for(int X:{-1,1})for(int Y:{-1,1})
 {auto* Wheel=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Wheel);Wheel->SetupAttachment(Mesh);Wheel->SetAbsolute(false,false,true);Wheel->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Wheel->SetRelativeLocation(FVector(X*57,Y*38,-49));Wheel->SetRelativeRotation(FRotator(90,0,0));Wheel->SetWorldScale3D(FVector(.28,.28,.16));Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);Wheel->RegisterComponent();}
 auto* Seam=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Seam);Seam->SetupAttachment(Mesh);Seam->SetAbsolute(false,false,true);Seam->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Seam->SetRelativeLocation(FVector(0,-50.5f,0));Seam->SetWorldScale3D(FVector(.02f,.015f,1.6f));Seam->SetCollisionEnabled(ECollisionEnabled::NoCollision);Seam->RegisterComponent();
 auto* Handle=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Handle);Handle->SetupAttachment(Mesh);Handle->SetAbsolute(false,false,true);Handle->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Handle->SetRelativeLocation(FVector(0,-52,20));Handle->SetWorldScale3D(FVector(.7,.06,.07));Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);Handle->RegisterComponent();
}
bool ALZRollingCabinet::IsAligned() const{return !bMoving && FMath::Abs(GetActorLocation().X+1750)<3;}
FString ALZRollingCabinet::GetInteractionPrompt(const ALZCharacter* P) const
{return bMoving?TEXT("[E] 松手停止推拉"):IsAligned()?TEXT("[E] 爬上柜顶 · 从侧面可拉回"):TEXT("[E] 沿轮痕推动柜子");}
void ALZRollingCabinet::Interact(ALZCharacter* P)
{
 if(!P || P->IsTraversing() || P->IsInventoryOpen() || FVector::Dist2D(P->GetActorLocation(),GetActorLocation())>300)return;
 if(bMoving){bMoving=false;Operator.Reset();return;}
 if(IsAligned() && P->GetActorLocation().X<GetActorLocation().X-90)
 {FVector End(GetActorLocation().X,GetActorLocation().Y,242);FVector Lift=P->GetActorLocation();Lift.Z=242;P->TryClimbRoute(Lift,End,End);return;}
 Target=ALZVentNetwork::EntryPoint(FVector(-2000,IsAligned()?1150:1350,100));Operator=P;bMoving=true;
}
void ALZRollingCabinet::Tick(float Delta)
{
 Super::Tick(Delta);if(!bMoving)return;
 ALZCharacter* P=Operator.Get();auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(!P || P->IsInventoryOpen() || P->IsTraversing() || (GM && GM->IsRunOver()) || FVector::Dist2D(P->GetActorLocation(),GetActorLocation())>450){bMoving=false;return;}
 const FVector Move=FMath::VInterpConstantTo(GetActorLocation(),Target,Delta,95)-GetActorLocation();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(CabinetPush),false,this);FHitResult H;
 if(GetWorld()->SweepSingleByChannel(H,GetActorLocation(),GetActorLocation()+Move,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(73,78,89)),Q)){bMoving=false;return;}
 AddActorWorldOffset(Move,true,&H);if(H.bBlockingHit || FVector::Dist(GetActorLocation(),Target)<1)bMoving=false;
}
void ALZVentAccess::Setup(ALZVentNetwork* InNetwork,FVector Bottom,FVector Top,bool Entry)
{
 Network=InNetwork;Ground=Bottom;Upper=Top;bEntryMouth=Entry;
 Mesh->SetWorldScale3D(FVector(1.25,.10,.25));SetLabel(TEXT(""),FColor(100,185,180));Glow->SetIntensity(0);Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
 if(Entry)for(int I=0;I<7;++I)
 {
  auto* Rail=GetWorld()->GetAuthGameMode<ALZGameMode>()->SpawnBlock(TEXT("MeetingPryGrate"),FVector(-1670,-1350,373+I*16),FVector(5,172,4),FRotator::ZeroRotator,FLinearColor(.32f,.37f,.39f));
  Rail->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);Grate.Add(Rail);
 }
}
FString ALZVentAccess::GetInteractionPrompt(const ALZCharacter* P) const
{
 if(bEntryMouth && !bGrateOpen && Network->Cabinet->IsAligned())return LZCrowbarVisual::InteractionKey()+TEXT("用撬棍撬开格栅并进入");
 if(bEntryMouth)return Network->Cabinet->IsAligned()?TEXT("[E] 抓住管口爬入 · Ctrl 蹲行"):TEXT("管口太高 · 下方有柜轮痕迹");
 return P && P->GetActorLocation().Z>400?TEXT("[E] 沿检修梯下到档案室"):TEXT("管道出口 · 从会议室进入");
}
void ALZVentAccess::Interact(ALZCharacter* P)
{
 if(!P || P->IsInventoryOpen() || P->IsTraversing())return;
 if(bEntryMouth)
 {
   if(!Network->Cabinet->IsAligned() || P->GetActorLocation().Z<220 || FVector::Dist2D(P->GetActorLocation(),Ground)>180)return;
   if(!bGrateOpen)
   {
    auto* C=GetWorld()->GetAuthGameMode<ALZGameMode>()->GetChapter();
    if(!P->HasCrowbarTool()){C->ShowFeedback(TEXT("格栅卡住了，需要撬棍。"));return;}
    for(auto* Rail:Grate)if(IsValid(Rail))Rail->Destroy();Grate.Empty();bGrateOpen=true;
    C->Noise(GetActorLocation(),250,TEXT("Rattle"));
   }
   FVector A=P->GetActorLocation();A.Z=410;FVector B=Upper;B.Z=410;
   if(P->TryClimbRoute(A,B,B))Network->bEntered=true;
   return;
 }
 if(!Network->bEntered)return;
 const bool Above=P->GetActorLocation().Z>400;
 if(!Above)return;
 const FVector End=Above?Ground:Upper;
 if(FVector::Dist2D(P->GetActorLocation(),Upper)>220 || FMath::Abs(P->GetActorLocation().Z-(Above?Upper.Z:Ground.Z))>160)return;
 FVector A=Ground;FVector B=Ground;
 if(Above){A.Z=Upper.Z;}else{B.Z=Upper.Z;}
 P->TryClimbRoute(A,B,End);
}
void ALZVentNetwork::Build(ALZGameMode* GM)
{
 const FLinearColor Steel(.15f,.24f,.26f);
 auto Block=[&](const TCHAR* N,FVector P,FVector S){auto* A=GM->SpawnBlock(N,P,S,FRotator::ZeroRotator,Steel);A->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);return A;};
 // Main corridors are tessellated on a 50 cm grid; shared boundaries never block junctions.
 TSet<FIntPoint> Cells;
 auto Area=[&](float X0,float X1,float Y0,float Y1){for(int X=FMath::FloorToInt(X0/50);X<FMath::CeilToInt(X1/50);++X)for(int Y=FMath::FloorToInt(Y0/50);Y<FMath::CeilToInt(Y1/50);++Y)Cells.Add(FIntPoint(X,Y));};
 Area(100,900,1450,1650);Area(-1650,-1050,-1450,-1250);Area(-1250,-1050,-1650,-1250);Area(700,900,-1750,1650);Area(-1250,900,-1750,-1550);
 // Each room has a short branch ending in a real open shaft.
 for(FVector P:Outlets())
 {
   const float Y=P.Y>1000?1550:P.Y<-1000?-1650:100;
   if(P.X==-3500){Area(-3600,-3400,-100,1650);Area(-3500,-3300,1450,1650);}
   else if(P.Y==100){Area(700,1200,0,200);}
   else Area(P.X-100,P.X+100,FMath::Min(Y,P.Y)-100,FMath::Max(Y,P.Y)+100);
 }
 auto IsHole=[&](FVector C){for(FVector P:Outlets())if(FMath::Abs(C.X-P.X)<76 && FMath::Abs(C.Y-P.Y)<76)return true;return false;};
 for(FIntPoint C:Cells)
 {
   FVector P(C.X*50+25,C.Y*50+25,550);
   // Entry ramp lies beside the north trunk; connect via its eastern landing.
   if(!IsHole(P) && !(P.Y<-1250 && P.Y>-1450 && P.X<-1150 && P.X>-1500))Block(TEXT("SafeVentFloor"),P,FVector(50,50,10));
   Block(TEXT("SafeVentRoof"),P+FVector(0,0,135),FVector(50,50,10));
   for(FIntPoint D:{FIntPoint(1,0),FIntPoint(-1,0),FIntPoint(0,1),FIntPoint(0,-1)})
    if(!Cells.Contains(C+D) && !(D.X==-1 && P.Y>-1450 && P.Y<-1250 && P.X<-1600))Block(TEXT("SafeVentSide"),P+FVector(D.X*25,D.Y*25,67),FVector(D.X?8:50,D.Y?8:50,130));
 }
 // The same proven entry kit is turned to run east inside meeting room 10.
 auto EntryBlock=[&](const TCHAR* Name,FVector P,FVector Size){return Block(Name,EntryPoint(P),EntrySize(Size));};
 EntryBlock(TEXT("MeetingVentLedge"),FVector(-2000,1500,355),FVector(180,180,10));
 EntryBlock(TEXT("MeetingVentMouthRoof"),FVector(-2000,1495,490),FVector(200,190,10));
 for(int Side:{-1,1})EntryBlock(TEXT("MeetingVentMouthSide"),FVector(-2000+Side*95,1500,422),FVector(10,180,130));
 const float Angle=FMath::RadiansToDegrees(FMath::Atan2(195.f,300.f));
 for(float Z:{452.5f,587.5f}){auto* R=GM->SpawnBlock(TEXT("MeetingVentRamp"),EntryPoint(FVector(-2000,1740,Z)),FVector(FMath::Sqrt(300.f*300+195.f*195),180,10),FRotator(Angle,0,0),Steel);R->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);}
 for(int Side:{-1,1})EntryBlock(TEXT("MeetingRampSide"),FVector(-2000+Side*95,1745,500),FVector(10,290,330));
 EntryBlock(TEXT("MeetingRampUpperLanding"),FVector(-2000,1945,550),FVector(180,110,10));
 Cabinet=GetWorld()->SpawnActor<ALZRollingCabinet>(EntryPoint(FVector(-2000,1150,100)),FRotator(0,-90,0));Cabinet->Setup(this);
 for(int Side:{-1,1})for(int I=0;I<8;++I)
 {auto* Scratch=GM->SpawnBlock(TEXT("CabinetWheelScuff"),EntryPoint(FVector(-2000+Side*60,1140+I*30,1)),EntrySize(FVector(8,22,1)),FRotator::ZeroRotator,FLinearColor(.35f,.32f,.22f));Scratch->SetActorEnableCollision(false);}
 auto* Mouth=GetWorld()->SpawnActor<ALZVentAccess>(EntryPoint(FVector(-2000,1410,380)),FRotator(0,-90,0));Mouth->Setup(this,EntryPoint(FVector(-2000,1350,242)),EntryPoint(FVector(-2000,1500,410)),true);Accesses.Add(Mouth);
 for(FVector P:Outlets())
 {
   FVector Bottom=P+FVector(0,0,58),Top=P+FVector(0,(P.Y>1000 || P.Y<-1000)?-180:180,605);
   if(P.Y==100)Top=P+FVector(-180,0,605);
   auto* High=GetWorld()->SpawnActor<ALZVentAccess>(P+FVector(0,-82,600),FRotator::ZeroRotator);High->Setup(this,Bottom,Top);Accesses.Add(High);
   for(int Side:{-1,1})Block(TEXT("VentLadderRail"),P+FVector(Side*65,85,300),FVector(5,5,570));
   for(int I=0;I<17;++I)Block(TEXT("VentLadderRung"),P+FVector(0,85,35+I*32),FVector(130,5,5));
 }
 // Soft spill from illuminated seams makes the route readable without a flashlight.
 TArray<FVector> Lights={EntryPoint(FVector(-2000,1000,280)),EntryPoint(FVector(-2000,1380,310)),FVector(-1200,-1350,635)};
 for(float X=-1100;X<=700;X+=450)Lights.Add(FVector(X,-1650,642));
 for(float Y=-1500;Y<=1500;Y+=450)Lights.Add(FVector(800,Y,642));
 Lights.Add(FVector(350,1550,642));Lights.Add(FVector(200,1750,642));
 for(FVector P:Lights)
 {
  auto* Light=GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator);
  auto* L=Cast<UPointLightComponent>(Light->GetLightComponent());
  L->SetIntensity(P.Z>500?35:90);L->SetAttenuationRadius(P.Z>500?720:650);
  L->SetLightColor(FLinearColor(.62f,.76f,.82f));L->SetCastShadows(true);
  Light->Tags.Add(TEXT("LZVentFill"));
  if(P.Z>500)
  {
   auto* Seam=Block(TEXT("VentLightSeam"),P+FVector(0,0,32),FVector(65,8,2));
   Seam->SetActorEnableCollision(false);
   auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDGreen.M_ZT_LEDGreen")),Seam);
   Mat->SetScalarParameterValue(TEXT("EmissiveStrength"),.06f);Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.62f,.76f,.82f));
   Seam->GetStaticMeshComponent()->SetMaterial(0,Mat);
  }
 }
}
