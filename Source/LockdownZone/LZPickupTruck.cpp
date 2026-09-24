#include "LZPickupTruck.h"
#include "LZGarageSlice.h"
#include "LZCharacter.h"
#include "LZGameMode.h"
#include "LZCrowbarVisual.h"
#include "LZAcoustics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
ALZPickupTruck::ALZPickupTruck(){
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PrePhysics;
 Chassis=CreateDefaultSubobject<UBoxComponent>(TEXT("TruckRigidBody"));SetRootComponent(Chassis);
 Chassis->SetBoxExtent(FVector(235,102,35));Chassis->SetCollisionProfileName(TEXT("PhysicsActor"));
 Chassis->SetCollisionObjectType(ECC_Vehicle);Chassis->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);
 Chassis->SetSimulatePhysics(true);Chassis->SetLinearDamping(.08f);Chassis->SetAngularDamping(.35f);Chassis->SetUseCCD(true);Chassis->SetCanEverAffectNavigation(false);
 auto* CabCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("CabCollision"));CabCollision->SetupAttachment(Chassis);CabCollision->SetRelativeLocation(FVector(8,0,80));CabCollision->SetBoxExtent(FVector(92,98,55));CabCollision->SetCollisionProfileName(TEXT("PhysicsActor"));CabCollision->SetCollisionObjectType(ECC_Vehicle);CabCollision->BodyInstance.bAutoWeld=true;CabCollision->SetCanEverAffectNavigation(false);
 Mesh->SetupAttachment(Chassis);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Mesh->SetRelativeScale3D(FVector(4.7f,2.04f,.55f));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);
 DoorHinge=CreateDefaultSubobject<USceneComponent>(TEXT("DriverDoorHinge"));DoorHinge->SetupAttachment(Chassis);DoorHinge->SetRelativeLocation(FVector(86,-105,55));
 auto* DoorQuery=CreateDefaultSubobject<UBoxComponent>(TEXT("DriverDoorQuery"));DoorQuery->SetupAttachment(DoorHinge);DoorQuery->SetRelativeLocation(FVector(-80,0,25));DoorQuery->SetBoxExtent(FVector(80,14,55));DoorQuery->SetCollisionEnabled(ECollisionEnabled::QueryOnly);DoorQuery->SetCollisionResponseToAllChannels(ECR_Ignore);DoorQuery->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);DoorQuery->SetCanEverAffectNavigation(false);
 CabinLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("OpenDoorLamp"));CabinLight->SetupAttachment(Chassis);CabinLight->SetRelativeLocation(FVector(15,-40,112));CabinLight->SetIntensity(160);CabinLight->SetAttenuationRadius(650);
 Glow->SetIntensity(0);Label->SetVisibility(false);
}
void ALZPickupTruck::BeginPlay(){
 Super::BeginPlay();Chassis->SetMassOverrideInKg(NAME_None,1400);Chassis->SetCenterOfMass(FVector(0,0,-35));
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto Part=[&](const TCHAR* N,FVector Loc,FVector Size,FLinearColor Color,USceneComponent* Parent=nullptr){
 auto* M=NewObject<UStaticMeshComponent>(this,FName(N));AddInstanceComponent(M);M->SetupAttachment(Parent?Parent:Chassis);M->SetStaticMesh(Cube);M->SetRelativeLocation(Loc);M->SetRelativeScale3D(Size/100);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);
 auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),this);Mat->SetVectorParameterValue(TEXT("Color"),Color);M->SetMaterial(0,Mat);M->RegisterComponent();return M;};
 const FLinearColor Teal(.04f,.36f,.30f),Metal(.20f,.24f,.24f),Dark(.018f,.024f,.027f),Yellow(.8f,.48f,.08f);
 auto* Paint=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),this);Paint->SetVectorParameterValue(TEXT("Color"),Teal);Mesh->SetMaterial(0,Paint);
 Part(TEXT("Hood"),FVector(157,0,47),FVector(145,198,42),Teal);
 Part(TEXT("CabRoof"),FVector(8,0,135),FVector(190,204,10),Teal);
 Part(TEXT("Dash"),FVector(81,0,64),FVector(22,188,35),Dark);
 Part(TEXT("RearCab"),FVector(-85,0,66),FVector(10,198,87),Teal);
 for(int Side:{-1,1}){
  Part(*FString::Printf(TEXT("A_Pillar%d"),Side),FVector(92,Side*99,101),FVector(9,9,70),Metal);
  Part(*FString::Printf(TEXT("BedSide%d"),Side),FVector(-163,Side*101,38),FVector(140,9,50),Teal);
  Part(*FString::Printf(TEXT("Rack%d"),Side),FVector(-103,Side*90,100),FVector(9,9,115),Yellow);
  Part(*FString::Printf(TEXT("Seat%d"),Side),FVector(-10,Side*50,52),FVector(58,55,15),Dark);
 }
 Part(TEXT("ToolRack"),FVector(-103,0,158),FVector(12,185,12),Yellow);
 Part(TEXT("ToolBox"),FVector(-170,0,50),FVector(80,160,35),Metal);
 Part(TEXT("Tailgate"),FVector(-235,0,35),FVector(9,200,48),Teal);
 Part(TEXT("PassengerDoor"),FVector(5,104,56),FVector(160,8,52),Teal);
 Part(TEXT("DriverDoor"),FVector(-80,0,0),FVector(160,8,52),Teal,DoorHinge);
 Part(TEXT("DriverHandle"),FVector(-128,-7,15),FVector(25,5,6),Yellow,DoorHinge);
 Part(TEXT("DriverFrame"),FVector(-152,0,45),FVector(7,7,65),Metal,DoorHinge);
 Part(TEXT("DriverTopFrame"),FVector(-80,0,77),FVector(155,7,7),Metal,DoorHinge);
 Part(TEXT("RamBumper"),FVector(245,0,-7),FVector(16,205,30),Metal);
 for(int I=0;I<4;++I){auto* W=Part(*FString::Printf(TEXT("Wheel%d"),I),FVector(I<2?150:-150,I%2?103:-103,-66),FVector(64,28,64),Dark);W->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));W->SetRelativeRotation(FRotator(0,0,90));W->SetRelativeScale3D(FVector(.64f,.64f,.28f));Wheels.Add(W);}
 auto* Text=NewObject<UTextRenderComponent>(this);AddInstanceComponent(Text);Text->SetupAttachment(Chassis);Text->SetRelativeLocation(FVector(-159,-109,48));Text->SetRelativeRotation(FRotator(0,-90,0));Text->SetText(FText::FromString(TEXT("MAINT / 01")));Text->SetWorldSize(18);Text->SetTextRenderColor(FColor::White);Text->RegisterComponent();
 DoorHinge->SetRelativeRotation(FRotator(0,45,0));Previous=GetActorLocation();
}
FVector ALZPickupTruck::DoorPosition()const{return GetActorTransform().TransformPosition(FVector(0,-140,40));}
float ALZPickupTruck::Speed()const{return FVector::DotProduct(Chassis->GetPhysicsLinearVelocity(),GetActorForwardVector());}
FString ALZPickupTruck::GetInteractionPrompt(const ALZCharacter* P)const{return LZCrowbarVisual::InteractionKey()+TEXT("驾驶维修皮卡 · 钥匙在车内");}
void ALZPickupTruck::Interact(ALZCharacter* P){if(FVector::Dist(P->GetActorLocation(),DoorPosition())<350)Enter(P);else if(Slice)Slice->Say(TEXT("绕到驾驶侧车门上车。"));}
bool ALZPickupTruck::Enter(ALZCharacter* P){
 if(Driving || !P || Chassis->GetPhysicsLinearVelocity().Size()>35 || P->IsTraversing() || P->GetHealth()<=0 || GetActorUpVector().Z<.8f || P->IsInventoryOpen())return false;
 if(FVector::Dist(P->GetActorLocation(),DoorPosition())>350)return false;
 P->SuspendOnFootActions();Driver=P;Driving=true;CloseAt=GetWorld()->GetTimeSeconds()+.65f;Throttle=Steer=0;Handbrake=false;
 P->GetCharacterMovement()->StopMovementImmediately();P->GetCharacterMovement()->DisableMovement();P->SetActorEnableCollision(false);P->SetActorHiddenInGame(true);
 auto* PC=Cast<APlayerController>(P->GetController());P->DisableInput(PC);EnableInput(PC);
 if(InputComponent){InputComponent->ClearActionBindings();InputComponent->AxisBindings.Empty();InputComponent->BindAxis(TEXT("Turn"),this,&ALZPickupTruck::LookYaw);InputComponent->BindAxis(TEXT("LookUp"),this,&ALZPickupTruck::LookPitch);InputComponent->BindAxis(TEXT("MoveForward"),this,&ALZPickupTruck::Forward);InputComponent->BindAxis(TEXT("MoveRight"),this,&ALZPickupTruck::Turn);InputComponent->BindAction(TEXT("Interact"),IE_Pressed,this,&ALZPickupTruck::ExitInput);InputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&ALZPickupTruck::BrakeOn);InputComponent->BindAction(TEXT("Jump"),IE_Released,this,&ALZPickupTruck::BrakeOff);InputComponent->BindAction(TEXT("RestartRun"),IE_Pressed,this,&ALZPickupTruck::RetryInput);}
 if(!DriveCamera){DriveCamera=GetWorld()->SpawnActor<ACameraActor>();DriveCamera->AttachToActor(this,FAttachmentTransformRules::KeepRelativeTransform);DriveCamera->SetActorRelativeLocation(FVector(50,-48,110));DriveCamera->SetActorRelativeRotation(FRotator(-3,0,0));DriveCamera->GetCameraComponent()->SetFieldOfView(95);}
 LookYawOffset=LookPitchOffset=0;UpdateDriverLook();
 PC->SetViewTargetWithBlend(DriveCamera,.25f);if(Slice)Slice->Say(TEXT("W 加速 / S 刹车倒车 / A D 转向 / 空格驻车 / 鼠标环视 / E 下车 / F5 重试"));return true;
}
bool ALZPickupTruck::FindExit(FVector& Place)const{
 if(!Driver)return false;
 const FVector Candidates[]={FVector(0,-210,0),FVector(-70,210,0),FVector(-340,0,0),FVector(340,0,0)};
 FCollisionQueryParams Q(SCENE_QUERY_STAT(TruckSafeExit),false,Driver);Q.AddIgnoredActor(this);
 for(FVector Local:Candidates){FVector P=GetActorTransform().TransformPosition(Local);FHitResult Floor;
  if(!GetWorld()->LineTraceSingleByChannel(Floor,P+FVector(0,0,150),P-FVector(0,0,250),ECC_WorldStatic,Q) || Floor.ImpactNormal.Z<.75f)continue;
  P=Floor.ImpactPoint+FVector(0,0,Driver->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3);
  if(GetWorld()->OverlapBlockingTestByChannel(P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(44,90),Q))continue;
  FHitResult Wall;if(GetWorld()->SweepSingleByChannel(Wall,GetActorLocation()+FVector(0,0,30),P,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(35),Q))continue;
  Place=P;return true;
 }return false;
}
bool ALZPickupTruck::Exit(){
 if(!Driving || GetWorld()->GetTimeSeconds()<CloseAt)return false;
 FVector P;if(Chassis->GetPhysicsLinearVelocity().Size()>GetDefault<ULZGarageSliceSettings>()->ExitSpeed || Chassis->GetPhysicsAngularVelocityInDegrees().Size()>15 || GroundWheels<3 || !FindExit(P)){if(Slice)Slice->Say(TEXT("请停车并留出下车空间；无法脱困可按 F5 重试。"));return false;}
 auto* PC=Cast<APlayerController>(Driver->GetController());Driving=false;Throttle=Steer=0;Handbrake=true;DisableInput(PC);
 Driver->SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);Driver->SetActorEnableCollision(true);Driver->SetActorHiddenInGame(false);Driver->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Driver->EnableInput(PC);PC->SetControlRotation(FRotator(0,GetActorRotation().Yaw,0));PC->SetViewTargetWithBlend(Driver,.2f);DoorAngle=45;Driver=nullptr;return true;
}
void ALZPickupTruck::LookYaw(float Value){
 if(!Driving)return;
 const auto* S=GetDefault<ULZGarageSliceSettings>();
 LookYawOffset=FMath::Clamp(LookYawOffset+Value*S->DriverLookSensitivity,-S->DriverLookYawLimit,S->DriverLookYawLimit);UpdateDriverLook();
}
void ALZPickupTruck::LookPitch(float Value){
 if(!Driving)return;
 const auto* S=GetDefault<ULZGarageSliceSettings>();
 // LookUp includes inverted MouseY mapping; local camera pitch uses the opposite sign.
 LookPitchOffset=FMath::Clamp(LookPitchOffset-Value*S->DriverLookSensitivity,-S->DriverLookDownLimit,S->DriverLookUpLimit);UpdateDriverLook();
}
void ALZPickupTruck::UpdateDriverLook(){if(DriveCamera)DriveCamera->SetActorRelativeRotation(FRotator(-3+LookPitchOffset,LookYawOffset,0));}
void ALZPickupTruck::RetryInput(){if(Slice)Slice->Retry();}
void ALZPickupTruck::SetTestInput(float F,float T,bool B){TestInput=true;Throttle=F;Steer=T;Handbrake=B;}
void ALZPickupTruck::DrivePhysics(float Delta){
 const auto* S=GetDefault<ULZGarageSliceSettings>();const float Mass=Chassis->GetMass(),V=Speed();
 SteerActual=FMath::FInterpTo(SteerActual,Steer*S->SteeringDegrees,Delta,4);GroundWheels=0;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(TruckSuspension),false,this);if(Driver)Q.AddIgnoredActor(Driver);
 for(int I=0;I<4;++I){const FVector Mount=GetActorTransform().TransformPosition(FVector(I<2?150:-150,I%2?94:-94,-10));FHitResult H;
  if(!GetWorld()->LineTraceSingleByChannel(H,Mount,Mount-GetActorUpVector()*105,ECC_WorldStatic,Q) || H.ImpactNormal.Z<.4f)continue;
  ++GroundWheels;const FVector Vel=Chassis->GetPhysicsLinearVelocityAtPoint(Mount);
  const float Spring=FMath::Clamp((105-H.Distance)*21000-FVector::DotProduct(Vel,GetActorUpVector())*3300,0.f,Mass*980);
  Chassis->AddForceAtLocation(GetActorUpVector()*Spring,Mount);
  const FVector Forward=FVector::VectorPlaneProject(FRotator(0,I<2?SteerActual:0,0).RotateVector(FVector::ForwardVector).RotateAngleAxis(GetActorRotation().Yaw,FVector::UpVector),H.ImpactNormal).GetSafeNormal();
  const FVector Right=FVector::CrossProduct(H.ImpactNormal,Forward).GetSafeNormal();
  const float Lateral=FMath::Clamp(-FVector::DotProduct(Vel,Right)*Mass*12.f,-Mass*900.f,Mass*900.f)/4;
  float Drive=0;
  if(Driving && GetWorld()->GetTimeSeconds()>CloseAt && !Handbrake){
   if(Throttle*V< -30)Drive=Throttle*Mass*1050/4;
   else if((Throttle>0 && V<S->MaxSpeed) || (Throttle<0 && V> -600))Drive=Throttle*Mass*(Throttle>0?S->EngineAcceleration:300)/4;
  }
  const float Drag=(Handbrake || !Driving)?FMath::Clamp(-FVector::DotProduct(Vel,Forward)*Mass*3,-Mass*1100.f,Mass*1100.f)/4:-FVector::DotProduct(Vel,Forward)*Mass*.18f/4;
  Chassis->AddForceAtLocation(Forward*(Drive+Drag)+Right*Lateral,Mount);
 }
 for(int I=0;I<Wheels.Num();++I)Wheels[I]->SetRelativeRotation(FRotator(0,I<2?SteerActual:0,90));
}
void ALZPickupTruck::SweepVictims(float Delta){
 const FVector Velocity=Chassis->GetPhysicsLinearVelocity();const FVector Fwd=GetActorForwardVector();
 for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){auto* E=*It;if(E->IsDead())continue;
 const FVector Local=GetActorTransform().InverseTransformPosition(E->GetActorLocation());
 const float Relative=FVector::DotProduct(Velocity-E->GetVelocity(),Fwd);
 // Leading bumper only, approaching relative velocity, victim within physical vertical footprint.
 const float Reach=245+FMath::Min(100.f,Velocity.Size()*Delta)+E->GetSimpleCollisionRadius();
 if(FMath::Abs(Local.Y)>102+E->GetSimpleCollisionRadius() || FMath::Abs(Local.Z)>170 || FMath::Abs(Local.X)>Reach || FMath::Abs(Local.X)<140 || Relative*Local.X<=0)continue;
 const float Now=GetWorld()->GetTimeSeconds();if(LastImpact.Contains(E) && Now-LastImpact[E]<1.5f)continue;
 const bool Boss=Cast<ALZGarageCharger>(E)!=nullptr;const float Threshold=Boss?GetDefault<ULZGarageSliceSettings>()->RamSpeed:400;
 if(FMath::Abs(Relative)<Threshold)continue;
 FHitResult Wall;FCollisionQueryParams Q(SCENE_QUERY_STAT(RamContact),false,this);Q.AddIgnoredActor(E);if(Driver)Q.AddIgnoredActor(Driver);
 if(GetWorld()->LineTraceSingleByChannel(Wall,GetActorLocation(),E->GetActorLocation(),ECC_WorldStatic,Q))continue;
 LastImpact.Add(E,Now);++Impacts;E->SetActorEnableCollision(false);
 UGameplayStatics::ApplyDamage(E,10000,nullptr,this,nullptr);LZAcoustics::Emit(this,E->GetActorLocation(),1800,TEXT("Crash"),TEXT("Crash"),.7f);
 Chassis->AddImpulse(-Velocity.GetSafeNormal()*Chassis->GetMass()*100);}
}
void ALZPickupTruck::Tick(float Delta){
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();
 if(auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();GM && GM->IsRunOver()){Throttle=Steer=0;Handbrake=true;}
 DoorAngle=FMath::FInterpConstantTo(DoorAngle,Driving?0:45,Delta,80);DoorHinge->SetRelativeRotation(FRotator(0,DoorAngle,0));CabinLight->SetIntensity(Driving?2:45);
 DrivePhysics(Delta);SweepVictims(Delta);
 if(Driving && Driver){Driver->SetActorLocation(GetActorLocation()+FVector(0,0,60),false,nullptr,ETeleportType::TeleportPhysics);
 if(Now>NextNoise){NextNoise=Now+.75f;LZAcoustics::Emit(this,GetActorLocation(),2800,TEXT("Motor"),TEXT("Engine"),.22f);}}
 Previous=GetActorLocation();
}
