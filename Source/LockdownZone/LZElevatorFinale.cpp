#include "LZElevatorFinale.h"
#include "AudioMixerBlueprintLibrary.h"
#include "LZChapter.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "LZHearingAI.h"
#include "LZGameMode.h"
#include "LZGarageSlice.h"
#include "LZRunState.h"
#include "LZCrowbarVisual.h"
#include "LZAcoustics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/HUD.h"
#include "GameFramework/WorldSettings.h"
#include "BrainComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformTime.h"
#include "UObject/UObjectGlobals.h"
#include "HighResScreenshot.h"
#include "UnrealClient.h"

ALZElevatorSeam::ALZElevatorSeam(){
 Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 Mesh->SetWorldScale3D(FVector(.14f,.20f,2.2f));Mesh->SetVisibility(false);Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Mesh->SetCanEverAffectNavigation(false);
 Glow->SetIntensity(0);Label->SetVisibility(false);
}
void ALZElevatorSeam::Interact(ALZCharacter* P){if(Finale)Finale->Start(P);}
FString ALZElevatorSeam::GetInteractionPrompt(const ALZCharacter* P)const{return Finale && Finale->Stage==ELZFinaleStage::Jammed?LZCrowbarVisual::InteractionKey()+TEXT("用撬棍撬开"):FString();}
ALZElevatorFinale::ALZElevatorFinale(){PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("CabinRoot")));}
UStaticMeshComponent* ALZElevatorFinale::Part(FName Name,FVector Position,FVector Size,FLinearColor Color){
 auto* M=NewObject<UStaticMeshComponent>(this,Name);AddInstanceComponent(M);M->SetupAttachment(GetRootComponent());M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));M->SetRelativeLocation(Position);M->SetRelativeScale3D(Size/100);M->SetMobility(EComponentMobility::Movable);M->SetCollisionProfileName(TEXT("BlockAllDynamic"));M->SetCanEverAffectNavigation(false);
 auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel")),this);
 if(!Mat)Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")),this);
 Mat->SetVectorParameterValue(TEXT("Color"),Color);Mat->SetVectorParameterValue(TEXT("Tint"),Color);M->SetMaterial(0,Mat);M->RegisterComponent();return M;
}
void ALZElevatorFinale::Initialize(ALZChapter* C,AActor* OldDoor){
 Chapter=C;GM=GetWorld()->GetAuthGameMode<ALZGameMode>();Player=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));SetActorLocation(FVector(3500,0,0));
 if(IsValid(OldDoor)){OldDoor->SetActorHiddenInGame(true);OldDoor->SetActorEnableCollision(false);}
 LeftDoor=Part(TEXT("LiftLeafLeft"),FVector(0,-60,150),FVector(20,120,300),FLinearColor(.24f,.27f,.28f));
 RightDoor=Part(TEXT("LiftLeafRight"),FVector(0,60,150),FVector(20,120,300),FLinearColor(.24f,.27f,.28f));
 Part(TEXT("CabinFloor"),FVector(170,0,-10),FVector(340,300,20),FLinearColor(.18f,.20f,.20f));
 Part(TEXT("CabinCeiling"),FVector(170,0,310),FVector(340,300,20),FLinearColor(.30f,.31f,.29f));
 Part(TEXT("CabinBack"),FVector(340,0,150),FVector(15,300,300),FLinearColor(.25f,.29f,.28f));
 Part(TEXT("CabinSideLeft"),FVector(170,-150,150),FVector(340,15,300),FLinearColor(.25f,.29f,.28f));
 Part(TEXT("CabinSideRight"),FVector(170,150,150),FVector(340,15,300),FLinearColor(.25f,.29f,.28f));
 for(int Side:{-1,1})Part(Side<0?TEXT("CabinJambLeft"):TEXT("CabinJambRight"),FVector(0,Side*135,150),FVector(22,30,300),FLinearColor(.12f,.15f,.14f));
 auto* Lamp=NewObject<UPointLightComponent>(this);AddInstanceComponent(Lamp);Lamp->SetupAttachment(GetRootComponent());Lamp->SetRelativeLocation(FVector(200,0,260));Lamp->SetIntensity(95);Lamp->SetAttenuationRadius(550);Lamp->SetLightColor(FLinearColor(.9f,.84f,.68f));Lamp->SetCastShadows(true);Lamp->RegisterComponent();
 Bar=LZCrowbarVisual::Build(this,GetRootComponent());Bar->SetVisibility(false,true);
 Arm=Part(TEXT("DamagedForearmPlaceholder"),FVector(-35,0,145),FVector(95,12,13),FLinearColor(.25f,.17f,.15f));Arm->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arm->SetVisibility(false);
 Hand=Part(TEXT("SeveredHandPlaceholder"),FVector(50,0,145),FVector(23,16,9),FLinearColor(.28f,.19f,.17f));Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);Hand->SetVisibility(false);
 for(int I=0;I<4;++I){auto* Finger=Part(*FString::Printf(TEXT("Finger%d"),I),FVector(72,I*4-6,145),FVector(20,3,4),FLinearColor(.28f,.19f,.17f));Finger->SetCollisionEnabled(ECollisionEnabled::NoCollision);Finger->SetVisibility(false);Fingers.Add(Finger);}
 CabinDisplay=NewObject<UTextRenderComponent>(this);AddInstanceComponent(CabinDisplay);CabinDisplay->SetupAttachment(GetRootComponent());CabinDisplay->SetRelativeLocation(FVector(20,0,262));CabinDisplay->SetWorldSize(23);CabinDisplay->SetHorizontalAlignment(EHTA_Center);CabinDisplay->SetTextRenderColor(FColor(210,189,139));CabinDisplay->SetText(FText::FromString(TEXT("12 F")));CabinDisplay->RegisterComponent();
 Seam=GetWorld()->SpawnActor<ALZElevatorSeam>(FVector(3480,0,150),FRotator::ZeroRotator);Seam->Finale=this;Seam->SetOwner(this);Seam->SetActorEnableCollision(false);
 CutArm=Part(TEXT("DetachedForearmPlaceholder"),FVector(28,0,145),FVector(48,12,12),FLinearColor(.25f,.17f,.15f));CutArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);CutArm->SetVisibility(false);
 EnableInput(Cast<APlayerController>(Player->GetController()));InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&ALZElevatorFinale::TogglePause).bExecuteWhenPaused=true;
 bQA=FParse::Param(FCommandLine::Get(),TEXT("LZElevatorQA"));bTest=bQA || FParse::Param(FCommandLine::Get(),TEXT("LZElevatorTest"));
}
void ALZElevatorFinale::SetOpening(float V){Opening=FMath::Clamp(V,0.f,120.f);LeftDoor->SetRelativeLocation(FVector(0,-60-Opening,150));RightDoor->SetRelativeLocation(FVector(0,60+Opening,150));}
void ALZElevatorFinale::Change(ELZFinaleStage V){Stage=V;Time=0;UE_LOG(LogTemp,Display,TEXT("ELEVATOR_STAGE %d at %.2f"),int(V),GetWorld()->GetTimeSeconds());}
void ALZElevatorFinale::Jam(){
 if(Stage!=ELZFinaleStage::Dormant)return;Change(ELZFinaleStage::Jammed);Chapter->ElevatorState=ELZElevatorState::Jammed;SetOpening(4);Seam->SetActorEnableCollision(true);
 GM->SpawnEnemy(FVector(2700,650,90),false);GM->SpawnEnemy(FVector(2750,-650,90),false);LZAcoustics::Emit(this,FVector(3480,0,110),9000,TEXT("Crash"),TEXT("Crash"),1);
}
FString ALZElevatorFinale::Prompt()const{
 if(Stage==ELZFinaleStage::Align || Stage==ELZFinaleStage::Force)return LZCrowbarVisual::InteractionKey()+TEXT("按住发力 · 松开暂停");
 if(Stage==ELZFinaleStage::Done && bLoadFailed)return TEXT("第一关完成 · 第二关地图未安装");
 if(Stage==ELZFinaleStage::Failed)return TEXT("电梯演出中断，可重试");return FString();
}
bool ALZElevatorFinale::Start(ALZCharacter* P){
 if(Stage!=ELZFinaleStage::Jammed || !P || !P->HasCrowbarTool() || P->IsInventoryOpen() || P->IsTraversing() || GM->IsRunOver())return false;
 auto* Cam=P->FindComponentByClass<UCameraComponent>();FVector Contact(3480,0,145);FVector Eye=Cam->GetComponentLocation();
 if(P->GetActorLocation().X>3450 || FVector::Dist(Eye,Contact)>230 || FVector::DotProduct(Cam->GetForwardVector(),(Contact-Eye).GetSafeNormal())<.93f)return false;
 FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(ElevatorSeamLOS),false,P);if(GetWorld()->LineTraceSingleByChannel(H,Eye,Contact,ECC_Visibility,Q) && H.GetActor()!=Seam && H.GetActor()!=this)return false;
 Q.AddIgnoredActor(this);Q.AddIgnoredActor(Seam);const FVector Align(3400,0,P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2);
 if(GetWorld()->SweepSingleByChannel(H,P->GetActorLocation(),Align,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(P->GetCapsuleComponent()->GetScaledCapsuleRadius(),P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Q))return false;
 ++Starts;Player=P;AlignStart=P->GetActorLocation();AlignRotation=P->GetControlRotation();ProtectEnemies(true);P->SetFinaleLocked(true);Seam->SetActorEnableCollision(false);Chapter->ElevatorState=ELZElevatorState::QTE;Change(ELZFinaleStage::Align);BeginGarageLoad();
 Look=GetWorld()->SpawnActor<APostProcessVolume>();Look->SetOwner(this);Look->bUnbound=true;Look->Priority=90;Look->Settings.bOverride_AutoExposureMinBrightness=Look->Settings.bOverride_AutoExposureMaxBrightness=true;Look->Settings.AutoExposureMinBrightness=Look->Settings.AutoExposureMaxBrightness=-5.1f;Look->Settings.bOverride_MotionBlurAmount=true;Look->Settings.MotionBlurAmount=0;
 return true;
}
void ALZElevatorFinale::ProtectEnemies(bool Active){
 if(Active){for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){auto* E=*It;if(E->IsDead())continue;HeldEnemies.Add(E);E->GetCharacterMovement()->StopMovementImmediately();E->GetCharacterMovement()->DisableMovement();E->SetActorTickEnabled(false);if(auto* C=Cast<AAIController>(E->GetController())){C->StopMovement();if(C->GetBrainComponent())C->GetBrainComponent()->PauseLogic(TEXT("Elevator performance"));}}}
 else{for(auto* E:HeldEnemies)if(IsValid(E)){E->SetActorTickEnabled(true);E->GetCharacterMovement()->SetMovementMode(MOVE_Walking);if(auto* C=Cast<AAIController>(E->GetController()))if(C->GetBrainComponent())C->GetBrainComponent()->ResumeLogic(TEXT("Elevator ended"));}HeldEnemies.Empty();}
}
bool ALZElevatorFinale::Held()const{
 if(bQA)return bQAForce;
 auto* PC=Cast<APlayerController>(Player->GetController());TArray<FInputActionKeyMapping> Keys;GetDefault<UInputSettings>()->GetActionMappingByName(TEXT("Interact"),Keys);for(const auto& K:Keys)if(PC->IsInputKeyDown(K.Key))return true;return false;
}
bool ALZElevatorFinale::MovePlayer(FVector Destination){FHitResult H;Player->SetActorLocation(Destination,true,&H,ETeleportType::None);return !H.bBlockingHit || FVector::Dist(Player->GetActorLocation(),Destination)<2;}
void ALZElevatorFinale::BeginGarageLoad(){
 const FString Path=TEXT("/Game/Chapter2Greybox/Maps/L_GarageEscape");if(!FPackageName::DoesPackageExist(Path)){bLoadFailed=true;return;}
 TWeakObjectPtr<ALZElevatorFinale> Self(this);LoadPackageAsync(Path,FLoadPackageAsyncDelegate::CreateLambda([Self](const FName&,UPackage* Package,EAsyncLoadingResult::Type Result){if(Self.IsValid()){Self->GaragePackage=Package;Self->bLoaded=Result==EAsyncLoadingResult::Succeeded;Self->bLoadFailed=!Self->bLoaded;}}));
}
void ALZElevatorFinale::TransferInCabin(){
 // The garage is runtime-generated. Keep the pawn/camera/cabin alive while replacing only the outside world.
 for(TActorIterator<AActor> It(GetWorld());It;++It){auto* A=*It;if(A==this || A==Player || A==GM || A==Look || A->GetOwner()==this || A->GetOwner()==Player || A->IsA<AController>() || A->IsA<APlayerCameraManager>() || A->IsA<AHUD>() || A->IsA<AWorldSettings>())continue;
 if(A->FindComponentByClass<UPrimitiveComponent>() || A->IsA<ALight>() || A->IsA<APostProcessVolume>() || A->GetClass()->GetName().StartsWith(TEXT("LZ")))OldActors.Add(A);}
 for(auto* A:OldActors){A->SetActorHiddenInGame(true);A->SetActorEnableCollision(false);A->SetActorTickEnabled(false);TInlineComponentArray<USceneComponent*> Components(A);for(auto* C:Components)C->SetVisibility(false,true);if(auto* PP=Cast<APostProcessVolume>(A))PP->bEnabled=false;TInlineComponentArray<UAudioComponent*> Audio(A);for(auto* Sound:Audio)Sound->FadeOut(.25f,0);}
 if(auto* Run=GetGameInstance<ULZRunState>())Run->Capture(Player);
 const FTransform Old=GetActorTransform();const FVector Local=Old.InverseTransformPosition(Player->GetActorLocation());const FRotator Rot=Player->GetControlRotation()+FRotator(0,90,0);
 SetActorLocationAndRotation(FVector(310,1200,0),FRotator(0,90,0));Player->SetActorLocation(GetActorTransform().TransformPosition(Local),false,nullptr,ETeleportType::TeleportPhysics);Player->GetController()->SetControlRotation(Rot);
 GM->Chapter=nullptr;GM->bCombatUnlocked=true;
 auto* G=GetWorld()->SpawnActorDeferred<ALZGarageSlice>(ALZGarageSlice::StaticClass(),FTransform::Identity);G->bSeamlessArrival=true;UGameplayStatics::FinishSpawningActor(G,FTransform::Identity);GM->GarageSlice=G;
 bTransferred=true;Chapter->ElevatorState=ELZElevatorState::Basement;
 // Ambient audio is not carried to the garage; ordinary AI remain inactive outside the retired floor.
 UE_LOG(LogTemp,Display,TEXT("ELEVATOR_SAME_WORLD_TRANSFER pawn=%s camera=%s"),*Player->GetName(),*Player->FindComponentByClass<UCameraComponent>()->GetName());
}
void ALZElevatorFinale::Abort(const TCHAR* Reason){UE_LOG(LogTemp,Error,TEXT("ELEVATOR_ABORT %s"),Reason);Bar->SetVisibility(false,true);if(IsValid(Player))Player->SetFinaleLocked(false);if(!bTransferred)ProtectEnemies(false);if(Look)Look->Destroy();Change(ELZFinaleStage::Failed);}
void ALZElevatorFinale::EndPlay(const EEndPlayReason::Type Reason){if(bQA && RecordStart>0 && !bAudioStopped)UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("InterruptedFinale"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));if(IsValid(Player))Player->SetFinaleLocked(false);if(!bTransferred)ProtectEnemies(false);Super::EndPlay(Reason);}
void ALZElevatorFinale::Tick(float Delta){
 Super::Tick(Delta);if(!Player || !Chapter)return;
 if(bTest && !bPrepared && GetWorld()->GetTimeSeconds()>1){bPrepared=true;Chapter->bDoorBreached=Chapter->bFuseInstalled=Chapter->bCrowbar=true;Player->AcquireCrowbar();Player->SetActorLocation(FVector(3325,0,98));Player->GetController()->SetControlRotation(FRotator(-7,0,0));Jam();}
 if(bQA)QA(Delta);
 if(UGameplayStatics::IsGamePaused(this))return;
 if(Stage==ELZFinaleStage::Dormant || Stage==ELZFinaleStage::Jammed || Stage==ELZFinaleStage::Done || Stage==ELZFinaleStage::Failed)return;
 if(GM->IsRunOver()){Abort(TEXT("Run ended"));return;}
 Time+=Delta;const auto* S=GetDefault<ULZElevatorFinaleSettings>();auto* PC=Cast<APlayerController>(Player->GetController());
 const float Half=Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 switch(Stage){
 case ELZFinaleStage::Align:{float T=FMath::SmoothStep(0.f,S->AlignSeconds,Time);if(!MovePlayer(FMath::Lerp(AlignStart,FVector(3400,0,Half+2),T))){Abort(TEXT("Alignment obstructed"));break;}PC->SetControlRotation(FMath::Lerp(AlignRotation,FRotator(-7,0,0),T));if(T>=1){Bar->SetVisibility(true,true);Change(ELZFinaleStage::Force);}break;}
 case ELZFinaleStage::Force:{
  if(Held())Force=FMath::Min(1.f,Force+Delta/FMath::Max(1.f,S->ForceSeconds));
  float Gap=4;
  if(Force<.2f)Gap=4+FMath::SmoothStep(0.f,.2f,Force)*3;
  else if(Force<.48f)Gap=7+FMath::SmoothStep(.2f,.48f,Force)*24;
  else if(Force<.65f)Gap=31+FMath::SmoothStep(.48f,.65f,Force)*6;
  else if(Force<.88f)Gap=37+FMath::SmoothStep(.65f,.88f,Force)*38;
  else Gap=75+FMath::SmoothStep(.88f,1.f,Force)*45;
  SetOpening(Gap);
  const FRotator Pose(0,-18+FMath::Sin(Force*PI*2)*20,-24);const FQuat Q=Pose.Quaternion();Bar->SetRelativeRotation(Pose);Bar->SetRelativeLocation(FVector(-12,-Gap+2,130)-Q.RotateVector(FVector(13,0,26)));
  if(Held() && Force>=(SoundBeat==0?.03f:SoundBeat==1?.24f:SoundBeat==2?.66f:.9f) && SoundBeat<4){LZAcoustics::Emit(this,FVector(3480,0,130),0,SoundBeat==3?TEXT("Clang"):TEXT("Rattle"),TEXT("Cinematic"),.5f);++SoundBeat;}
  if(Force>=1){EntryStart=Player->GetActorLocation();Chapter->ElevatorState=ELZElevatorState::AutoEnter;Change(ELZFinaleStage::Enter);}break;}
 case ELZFinaleStage::Enter:{float T=FMath::SmoothStep(0.f,S->EntrySeconds,Time);Bar->SetVisibility(Time<.3f,true);if(!MovePlayer(FMath::Lerp(EntryStart,FVector(3740,0,Half+2),T))){Abort(TEXT("Cabin entry obstructed"));break;}if(T>=1){Check(Player->GetActorLocation().X-Half>3515,TEXT("whole capsule is inside before door closure"));Change(ELZFinaleStage::Turn);}break;}
 case ELZFinaleStage::Turn:{float T=FMath::SmoothStep(0.f,S->TurnSeconds,Time);PC->SetControlRotation(FRotator(-4,180*T,0));if(T>=1){Chapter->ElevatorState=ELZElevatorState::Closing;Arm->SetVisibility(true);Hand->SetVisibility(true);for(auto* F:Fingers)F->SetVisibility(true);Change(ELZFinaleStage::Close);}break;}
 case ELZFinaleStage::Close:{float T=FMath::SmoothStep(0.f,1.1f,Time);SetOpening(FMath::Lerp(120.f,9.f,T));Arm->SetRelativeLocation(FVector(FMath::Lerp(-65.f,-5.f,FMath::Clamp(Time/.5f,0.f,1.f)),0,145));Hand->SetRelativeLocation(FVector(FMath::Lerp(-15.f,55.f,FMath::Clamp(Time/.5f,0.f,1.f)),0,145));for(int I=0;I<Fingers.Num();++I)Fingers[I]->SetRelativeLocation(Hand->GetRelativeLocation()+FVector(17,I*4-6,0));if(T>=1){LZAcoustics::Emit(this,GetActorLocation(),0,TEXT("Motor"),TEXT("Cinematic"),.55f);Change(ELZFinaleStage::Pinch);}break;}
 case ELZFinaleStage::Pinch:if(Time>.7f){LZAcoustics::Emit(this,GetActorLocation(),0,TEXT("Clang"),TEXT("Cinematic"),.7f);Arm->SetVisibility(false);CutArm->SetVisibility(true);Change(ELZFinaleStage::Sever);}break;
 case ELZFinaleStage::Sever:{SetOpening(FMath::Lerp(9.f,0.f,FMath::Clamp(Time/.22f,0.f,1.f)));const float T=FMath::Clamp(Time/.75f,0.f,1.f);Hand->SetRelativeLocation(FVector(55+20*T,0,FMath::Lerp(145.f,8.f,T*T)));Hand->SetRelativeRotation(FRotator(70*T,20*T,0));CutArm->SetRelativeLocation(FVector(28+20*T,0,FMath::Lerp(145.f,8.f,T*T)));CutArm->SetRelativeRotation(FRotator(70*T,20*T,0));for(int I=0;I<Fingers.Num();++I){Fingers[I]->SetRelativeLocation(Hand->GetRelativeLocation()+FVector(17,I*4-6,0));Fingers[I]->SetRelativeRotation(FRotator(70*T,20*T,0));}if(Time>1){Check(Opening==0,TEXT("both leaves fully shut before descent"));LZAcoustics::Emit(this,GetActorLocation(),0,TEXT("Crash"),TEXT("Cinematic"),.20f);Chapter->ElevatorState=ELZElevatorState::Descending;Change(ELZFinaleStage::Descending);LZAcoustics::Emit(this,GetActorLocation(),0,TEXT("Motor"),TEXT("Cinematic"),.35f);}break;}
 case ELZFinaleStage::Descending:
  CabinDisplay->SetText(FText::FromString(FString::Printf(TEXT("%02d  v"),FMath::Max(1,12-int(Time*3)))));
  if(bLoadFailed){Chapter->ElevatorState=ELZElevatorState::Escaped;Player->SetFinaleLocked(false);if(Look)Look->Destroy();Change(ELZFinaleStage::Done);break;}
  if(bLoaded && Time>1 && !bTransferred)TransferInCabin();
  if(bTransferred && Time>S->DescentSeconds){CabinDisplay->SetText(FText::FromString(TEXT("B1")));Change(ELZFinaleStage::Arrival);LZAcoustics::Emit(this,GetActorLocation(),0,TEXT("Relay"),TEXT("Cinematic"),.4f);}break;
 case ELZFinaleStage::Arrival:{float T=FMath::SmoothStep(0.f,1.5f,Time);SetOpening(120*T);if(Look)Look->BlendWeight=1-T;if(T>=1){Player->SetFinaleLocked(false);if(Look)Look->Destroy();Change(ELZFinaleStage::Done);Check(GM->GarageSlice && !GM->GetChapter() && !Player->IsFinaleLocked(),TEXT("same pawn reaches garage and regains input"));}break;}
 default:break;
 }
}
void ALZElevatorFinale::Check(bool Good,const TCHAR* What){Checks.Add(FString::Printf(TEXT("%s %s"),Good?TEXT("PASS"):TEXT("FAIL"),What));UE_LOG(LogTemp,Display,TEXT("ELEVATOR_QA %s"),*Checks.Last());}
void ALZElevatorFinale::QA(float Delta){
 const float Now=GetWorld()->GetTimeSeconds();static double RecordWallStart=0;if(!bPrepared)return;
 if(Stage==ELZFinaleStage::Jammed && Now>3){
  Check(!Player->IsFinaleLocked() && !Player->GetController()->IsMoveInputIgnored() && Force==0 && Starts==0,TEXT("fresh or retried sequence starts with clean control and progress"));
  Chapter->Use(Chapter->FindNode(ELZChapterNode::Elevator),Player);Check(Stage==ELZFinaleStage::Jammed,TEXT("call button does not start QTE"));
  RecordStart=Now;RecordWallStart=FPlatformTime::Seconds();QAStarted=Now;UAudioMixerBlueprintLibrary::StartRecordingOutput(this,30);const bool Started=Start(Player);Check(Started,TEXT("center seam starts protected QTE"));if(!Started){Abort(TEXT("QA seam start rejected"));return;}Check(!Start(Player) && Starts==1,TEXT("duplicate start rejected"));}
 const float Elapsed=Now-QAStarted;
 static bool Retried=false;
 if(FParse::Param(FCommandLine::Get(),TEXT("LZElevatorRetryQA")) && !Retried && Stage==ELZFinaleStage::Force && Elapsed>1){Retried=true;GM->RestartRun();return;}
bQAForce=Elapsed<2 || Elapsed>3.2f;
 if(Stage==ELZFinaleStage::Force && Elapsed>1.3f && !bPauseTest){bPauseTest=true;const float BeforeForce=Force;const float BeforeTime=Now;TWeakObjectPtr<ALZElevatorFinale> Weak(this);UGameplayStatics::SetGamePaused(this,true);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,BeforeForce,BeforeTime](float){if(Weak.IsValid()){UE_LOG(LogTemp,Display,TEXT("PAUSE_DIAGNOSTIC paused=%d force=%.6f/%.6f time=%.6f/%.6f"),UGameplayStatics::IsGamePaused(Weak.Get()),BeforeForce,Weak->Force,BeforeTime,Weak->GetWorld()->GetTimeSeconds());Weak->Check(UGameplayStatics::IsGamePaused(Weak.Get()) && FMath::IsNearlyEqual(BeforeForce,Weak->Force) && FMath::IsNearlyEqual(BeforeTime,float(Weak->GetWorld()->GetTimeSeconds()),.001f),TEXT("system pause freezes performance and resumes"));UGameplayStatics::SetGamePaused(Weak.Get(),false);}return false;}),.45f);}

 if(Stage==ELZFinaleStage::Force && Elapsed>=2 && !bRepeatChecked){bRepeatChecked=true;PauseValue=Force;const float Health=Player->GetHealth();UGameplayStatics::ApplyDamage(Player,500,nullptr,this,nullptr);Check(Player->GetHealth()==Health,TEXT("damage ignored during protected sequence"));Player->ToggleInventory();Check(!Player->IsInventoryOpen() && Player->GetController()->IsMoveInputIgnored() && Player->GetController()->IsLookInputIgnored(),TEXT("inventory movement and free look locked"));Check(LeftDoor->GetRelativeLocation().Y<0 && RightDoor->GetRelativeLocation().Y>0 && FMath::IsNearlyEqual(-LeftDoor->GetRelativeLocation().Y,RightDoor->GetRelativeLocation().Y),TEXT("door leaves open symmetrically without scaling"));}
 if(bRepeatChecked && !bPauseChecked && Elapsed>3.1f && Elapsed<3.2f){bPauseChecked=true;Check(FMath::IsNearlyEqual(PauseValue,Force,.001f) && Player->IsFinaleLocked(),TEXT("release pauses progress without returning control"));}
 if(RecordStart>0 && Now>=RecordNext){RecordNext=Now+.10f;const FString Name=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/ElevatorFinale/frame_%05d.png"),Frame++));IFileManager::Get().MakeDirectory(*FPaths::GetPath(Name),true);FScreenshotRequest::RequestScreenshot(Name,false,false);Frames.Add(FString::Printf(TEXT("%.6f,%s"),FPlatformTime::Seconds()-RecordWallStart,*Name));}
 if(Stage==ELZFinaleStage::Done && Time<2.4f)Player->AddMovementInput(FVector(0,-1,0),1);
 if((Stage==ELZFinaleStage::Done && Time>2.5f) || Stage==ELZFinaleStage::Failed || Elapsed>65){
  if(!bAudioStopped){bAudioStopped=true;UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("ElevatorFinale"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()));return;}
  if(!FPaths::FileExists(FPaths::ProjectSavedDir()/TEXT("ElevatorFinale.wav")) && Time<6)return;
  Check(Player->GetActorLocation().Y<1190 && Player==UGameplayStatics::GetPlayerCharacter(this,0),TEXT("same character walks out into garage after control returns"));
  Check(Stage==ELZFinaleStage::Done,TEXT("sequence completes without travel or black fade"));FFileHelper::SaveStringToFile(FString::Join(Checks,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("ElevatorFinaleQA.txt")));FFileHelper::SaveStringToFile(FString::Join(Frames,TEXT("\n")),*(FPaths::ProjectSavedDir()/TEXT("ElevatorFinaleFrames.csv")));FPlatformMisc::RequestExit(false);
 }
 if(Stage==ELZFinaleStage::Done)Time+=Delta;
}

void ALZElevatorFinale::TogglePause(){UGameplayStatics::SetGamePaused(this,!UGameplayStatics::IsGamePaused(this));}
