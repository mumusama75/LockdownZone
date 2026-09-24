#include "LZVentQA.h"
#include "LZWeaponPickup.h"
#include "LZLoot.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/PointLight.h"
#include "LZFlashlightPickup.h"
#include "Components/SpotLightComponent.h"
#include "LZVentNetwork.h"
#include "LZGameMode.h"
#include "LZChapter.h"
#include "LZCharacter.h"
#include "LZEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
ALZVentQA::ALZVentQA(){PrimaryActorTick.bCanEverTick=true;}
void ALZVentQA::Check(bool V,const TCHAR* M){++Assertions;if(!V)++Failures;FString S=FString::Printf(TEXT("VENT_QA %s %s"),V?TEXT("PASS"):TEXT("FAIL"),M);Lines.Add(S);UE_LOG(LogTemp,Display,TEXT("%s"),*S);}
void ALZVentQA::View(FVector V,FVector T){auto* P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));P->GetCharacterMovement()->StopMovementImmediately();P->SetActorLocation(V);FRotator R=(T-(V+FVector(-10,0,64))).Rotation();Cast<APlayerController>(P->GetController())->SetControlRotation(R);P->FindComponentByClass<UCameraComponent>()->SetWorldRotation(R);}
void ALZVentQA::Capture(const TCHAR* N){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/WindowsEditor/Vent_")+N+TEXT(".png"),true,false);}
void ALZVentQA::Tick(float Delta)
{
 Super::Tick(Delta);const float Now=GetWorld()->GetTimeSeconds();if(Now<Next || FScreenshotRequest::IsScreenshotRequested())return;Next=Now+.8f;
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();auto* C=GM->GetChapter();auto* N=C?C->VentNetwork:nullptr;auto* P=Cast<ALZCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));if(!N || !P)return;
 auto E=[](FVector V){return ALZVentNetwork::EntryPoint(V);};
 FCollisionQueryParams Q(SCENE_QUERY_STAT(VentQA),false,P);FHitResult H;
 switch(Step++)
 {
 case 0:
 {
  for(TActorIterator<ALZEnemy> It(GetWorld());It;++It){It->SetActorTickEnabled(false);It->GetCharacterMovement()->StopMovementImmediately();}
  for(TActorIterator<ALZInteractable> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryPistol")) || It->ActorHasTag(TEXT("LZPantryMedkit")))
  {
   It->Interact(P);Check(IsValid(*It) && !P->HasFirearm() && It->GetInteractionPrompt(P).IsEmpty(),TEXT("without flashlight hidden pantry loot cannot be acquired or named by aiming"));
  }
  Check(!GM->IsCombatUnlocked(),TEXT("opening safe state remains locked"));
  Check(GetWorld()->LineTraceSingleByChannel(H,FVector(-1775,-600,180),FVector(-1775,-900,180),ECC_Visibility,Q) && H.GetActor()->ActorHasTag(TEXT("LZMeetingObservation")),TEXT("office north window faces meeting room"));
  Check(GetWorld()->LineTraceSingleByChannel(H,FVector(-1650,-1000,180),FVector(-1650,-1300,180),ECC_Visibility,Q) && H.GetActor()->ActorHasTag(TEXT("LZMeetingObservation")),TEXT("meeting frontage has aligned second observation window"));
  Check(!GetWorld()->OverlapBlockingTestByChannel(FVector(-3150,1475,160),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(500,425,130)),Q),TEXT("former reception interior is cleared of furniture and entry kit"));
  Check(!GetWorld()->SweepSingleByChannel(H,FVector(-950,-1000,90),FVector(-950,-1340,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("relocated meeting room door is passable"));
  Check(GetWorld()->SweepSingleByChannel(H,FVector(-2200,700,90),FVector(-2200,1100,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("former western reception shortcut is sealed"));
  Check(GetWorld()->SweepSingleByChannel(H,FVector(-1100,650,90),FVector(-1100,1120,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("former D18 is now a solid wall"));
  for(float Y:{-750.f,900.f})
  {
   Check(GetWorld()->SweepSingleByChannel(H,FVector(300,Y-110,90),FVector(300,Y+110,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q)==(Y>0),TEXT("A is open while B starts physically locked"));
   // Sample the full frontage, including former doors and eastern flank bypasses.
   bool Sealed=true;
   for(float X=-2440;X<=3700;X+=50)
   {
    if(X>60 && X<540)continue;
    if(!GetWorld()->SweepSingleByChannel(H,FVector(X,Y-100,58),FVector(X,Y+100,58),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,48),Q))Sealed=false;
   }
   Check(Sealed,TEXT("entire office frontage outside A or B blocks even crouched passage"));
  }
  const TArray<TPair<FVector,FVector>> GroundRoutes={
   {FVector(300,-980,90),FVector(-3200,-980,90)},
   {FVector(300,-980,90),FVector(3300,-980,90)},
   {FVector(3300,-980,90),FVector(3300,-1540,90)},
   {FVector(3300,-1540,90),FVector(3040,-1540,90)},
   {FVector(300,1120,90),FVector(-1450,1120,90)}};
  for(auto R:GroundRoutes)
  {
   bool Clear=!GetWorld()->SweepSingleByChannel(H,R.Key,R.Value,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q);
   Check(Clear,TEXT("A/B ground route connects corridor destinations without a blockage"));
   if(!Clear)UE_LOG(LogTemp,Display,TEXT("AB_ROUTE_BLOCK %s %s"),*GetNameSafe(H.GetActor()),*H.ImpactPoint.ToString());
  }
  Check(GetWorld()->LineTraceSingleByChannel(H,FVector(-3300,300,180),FVector(-3300,1100,180),ECC_Visibility,Q) && !H.GetActor()->ActorHasTag(TEXT("LZReceptionObservation")),TEXT("incorrect starting-room window is replaced by solid wall"));
  View(FVector(-2400,-610,90),E(FVector(-2000,1410,350)));Capture(TEXT("01_ExitViewWindow"));break;
 }
 case 1:
  P->AcquireCrowbar(); // Vent-only fixture begins after the office tool tutorial.
  View(E(FVector(-2000,1000,90)),N->Cabinet->GetActorLocation());N->Cabinet->Interact(P);Check(N->Cabinet->IsMoving(),TEXT("cabinet interaction starts constrained push"));Next=Now+2.8f;break;
 case 2:
  Check(N->Cabinet->IsAligned(),TEXT("wheeled cabinet reaches mouth alignment"));Check(FMath::Abs(N->Cabinet->GetActorLocation().Y+1350)<1,TEXT("cabinet remains on one axis"));
  View(FVector(-2070,-1670,90),E(FVector(-2000,1400,290)));Capture(TEXT("02_CabinetAndMouth"));break;
 case 3:
  View(E(FVector(-2000,1180,90)),N->Cabinet->GetActorLocation());N->Cabinet->Interact(P);Check(P->IsTraversing(),TEXT("cabinet top climb starts with collision validation"));Next=Now+1.4f;break;
 case 4:
  Check(P->GetActorLocation().Z>230,TEXT("player physically stands on cabinet"));N->Accesses[0]->Interact(P);Check(P->IsTraversing(),TEXT("cabinet grants reachable mouth climb"));Next=Now+1.4f;break;
 case 5:
  Check(N->bEntered && P->GetActorLocation().Z>390,TEXT("player reaches lowered ventilation mouth"));
  for(int I=0;I<200;++I){P->AddMovementInput(FVector(1,0,0));P->GetCharacterMovement()->TickComponent(1.f/60,LEVELTICK_All,nullptr);}
  Check(P->GetActorLocation().Z>595,TEXT("crouched player physically climbs connecting ramp"));
  UE_LOG(LogTemp,Display,TEXT("VENT_RAMP_POSITION %s"),*P->GetActorLocation().ToString());
  View(FVector(-1000,-1650,605),FVector(800,-1650,605));Capture(TEXT("03_NorthTrunk"));break;
 case 6:
 {
  Check(ALZVentNetwork::Outlets().Num()==1 && N->Accesses.Num()==2,TEXT("only meeting entry and archive exit remain"));
  const TArray<TPair<FVector,FVector>> Routes={
   {FVector(-1100,-1650,605),FVector(800,-1650,605)},
   {FVector(800,-1650,605),FVector(800,1550,605)},
   {FVector(800,1550,605),FVector(200,1550,605)},
   {FVector(200,1550,605),FVector(200,1850,605)},
   {FVector(-1100,-1350,605),FVector(-1150,-1350,605)},
   {FVector(-1150,-1350,605),FVector(-1150,-1650,605)}};
  for(auto R:Routes)Check(!GetWorld()->SweepSingleByChannel(H,R.Key,R.Value,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,48),Q),TEXT("single ventilation route passes crouched capsule continuously"));
  for(FVector V:{FVector(-3500,0,0),FVector(-1400,1850,0),FVector(-450,1850,0),FVector(2900,1850,0),FVector(1100,100,0),FVector(-3200,-1450,0),FVector(-2450,-1450,0),FVector(-800,-1450,0),FVector(300,-1450,0),FVector(2600,-1450,0),FVector(3450,-1450,0),FVector(-915,1550,0)})
   Check(GetWorld()->LineTraceSingleByChannel(H,V+FVector(0,0,330),V+FVector(0,0,440),ECC_Visibility,Q),TEXT("deleted room exit and old manager breach have solid ceilings"));
  Check(GetWorld()->SweepSingleByChannel(H,FVector(800,500,645),FVector(800,700,645),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("standing capsule cannot pass low duct"));
  View(FVector(800,0,605),FVector(800,1550,605));Capture(TEXT("04_LitSpine"));break;
 }
 case 7:
 {
  auto* A=N->Accesses[1];View(A->Upper,A->GetActorLocation());A->Interact(P);Check(P->IsTraversing(),TEXT("archive exit starts swept descent"));Next=Now+1.4f;break;
 }
 case 8:
  Check(P->GetActorLocation().Z<100,TEXT("archive exit reaches room floor"));
  N->Accesses[1]->Interact(P);Check(!P->IsTraversing(),TEXT("archive outlet cannot act as a second entrance"));
  Check(IsValid(C->ArchiveFlashlight) && C->ArchiveFlashlight->GuideBeam && C->ArchiveFlashlight->GuideBeam->Intensity>0,TEXT("archive pickup casts a powered guide beam before pickup"));
  View(FVector(200,1750,90),FVector(480,1850,75));Capture(TEXT("08_ArchiveGuideLight"));break;
 case 9:
  C->ArchiveFlashlight->Interact(P);Check(P->HasFlashlight() && !IsValid(C->ArchiveFlashlight),TEXT("pickup grants flashlight and removes its world light"));
  View(E(FVector(-1780,1350,90)),N->Cabinet->GetActorLocation());N->Cabinet->Interact(P);Next=Now+2.8f;break;
 case 10:
  Check(FMath::Abs(N->Cabinet->GetActorLocation().X+1950)<2,TEXT("cabinet can be pulled back along the same rail"));break;
 case 11:
  if(P->IsFlashlightOn())P->ToggleFlashlight();
  View(FVector(300,-200,90),FVector(300,-980,140));Capture(TEXT("05_ExitA"));break;
 case 12:
  View(FVector(300,350,90),FVector(300,1120,140));Capture(TEXT("06_ExitB"));break;
 case 13:
  View(FVector(-1800,1100,90),FVector(-3200,1650,140));Capture(TEXT("07_EmptyReception"));break;
 case 14:
 {
  for(auto Kind:{ELZChapterNode::Breaker,ELZChapterNode::Elevator})
  {
   auto* Node=C->FindNode(Kind);FCollisionQueryParams WallQ=Q;WallQ.AddIgnoredActor(Node);
   Check(Node && GetWorld()->LineTraceSingleByChannel(H,Node->GetActorLocation(),Node->GetActorLocation()+FVector(40,0,0),ECC_Visibility,WallQ) && FMath::Abs(H.ImpactPoint.X-3480)<2,TEXT("lift controls sit directly on the structural jamb"));
  }
  int32 Count=0;bool Clear=true;
  for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZCeilingCable")))
  {
   ++Count;const FBox Box=It->GetComponentsBoundingBox(true);FCollisionQueryParams CableQ=Q;CableQ.AddIgnoredActor(*It);
   if(GetWorld()->OverlapBlockingTestByChannel(Box.GetCenter(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(Box.GetExtent()-FVector(.25f)),CableQ))
   {
    Clear=false;GetWorld()->SweepSingleByChannel(H,Box.GetCenter(),Box.GetCenter()+FVector(.1f,0,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(Box.GetExtent()-FVector(.25f)),CableQ);
    UE_LOG(LogTemp,Warning,TEXT("CABLE_OVERLAP cable=%s bounds=%s hit=%s"),*It->GetActorNameOrLabel(),*Box.ToString(),H.GetActor()?*H.GetActor()->GetActorNameOrLabel():TEXT("none"));
   }
  }
  Check(Count==4 && Clear,TEXT("all four conduit segments clear wall and ceiling geometry"));
  View(FVector(3150,-260,90),FVector(3478,-210,175));Capture(TEXT("09_LiftNote"));break;
 }
 case 15:
  View(FVector(2900,-150,90),FVector(3410,650,270));Capture(TEXT("10_CeilingCableC"));break;
 case 16:
 {
  int32 Corpses=0,Guns=0,Kits=0,Signs=0;
  for(TActorIterator<AActor> It(GetWorld());It;++It)
  {Corpses+=It->ActorHasTag(TEXT("LZPantryGuardCorpse"));Guns+=It->ActorHasTag(TEXT("LZPantryPistol"));Kits+=It->ActorHasTag(TEXT("LZPantryMedkit"));Signs+=It->ActorHasTag(TEXT("LZPantryMedicalSign"));}
  for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryGuardCorpse")))
  {
   auto* Body=It->FindComponentByClass<USkeletalMeshComponent>();
   Check(Body && !Cast<ALZEnemy>(*It) && !Body->IsComponentTickEnabled() && Body->Bounds.BoxExtent.Z*2<100,TEXT("guard is a static floor corpse outside enemy AI"));
  }
  bool Dark=true;for(TActorIterator<APointLight> It(GetWorld());It;++It){const FVector V=It->GetActorLocation();if(V.X>-650 && V.X<1150 && V.Y<-1200 && V.Z<400 && It->GetLightComponent()->Intensity>0)Dark=false;}
  Check(Dark,TEXT("pantry general illumination is disabled"));
  Check(Corpses==1 && Guns==1 && Kits==1 && Signs==1,TEXT("dark pantry contains one guard corpse pistol medkit and medical sign"));
  Check(!GetWorld()->SweepSingleByChannel(H,FVector(300,-1040,90),FVector(300,-1350,90),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,88),Q),TEXT("D13 remains passable without blood or body blocking entrance"));
  if(P->IsFlashlightOn())P->ToggleFlashlight();
  View(FVector(300,-950,90),FVector(350,-1330,110));Capture(TEXT("11_PantryDoorClues"));break;
 }
 case 17:
  View(FVector(300,-1300,90),FVector(340,-1490,20));Capture(TEXT("12_PantryDark"));break;
 case 18:
  if(!P->IsFlashlightOn())P->ToggleFlashlight();
  View(FVector(300,-1300,90),FVector(340,-1490,20));Capture(TEXT("13_PantryFlashlight"));break;
 case 19:
  if(P->IsFlashlightOn())P->ToggleFlashlight();
  for(TActorIterator<ALZInteractable> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryPistol")) || It->ActorHasTag(TEXT("LZPantryMedkit")))
  {
   It->Interact(P);Check(IsValid(*It) && It->GetInteractionPrompt(P).IsEmpty(),TEXT("owned but switched-off flashlight does not expose or unlock pantry loot"));
   bool Unlit=true;TArray<UPointLightComponent*> Lights;It->GetComponents(Lights);for(auto* L:Lights)Unlit &= L->Intensity==0;
   Check(Unlit && It->GetActorLocation().Y<-1800,TEXT("pantry loot is deep in room with zero pickup light"));
  }
  View(FVector(975,-1710,90),FVector(980,-1870,12));Capture(TEXT("14_DeepLootDark"));break;
 case 20:
  if(!P->IsFlashlightOn())P->ToggleFlashlight();
  View(FVector(975,-1710,90),FVector(980,-1870,12));Capture(TEXT("15_DeepLootLit"));break;
 case 21:
 {
  for(TActorIterator<ALZInteractable> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryPistol")) || It->ActorHasTag(TEXT("LZPantryMedkit")))
   Check(GetWorld()->LineTraceSingleByChannel(H,P->GetActorLocation()+FVector(0,0,64),It->GetActorLocation(),ECC_Visibility,Q) && H.GetActor()==*It,TEXT("deep pantry loot has unobstructed interaction line from standing approach"));
  for(TActorIterator<ALZWeaponPickup> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryPistol"))){It->Interact(P);Check(!IsValid(*It) && P->GetSelectedWeapon()==EPlayerWeapon::Firearm && P->GetAmmoInMagazine()==3,TEXT("pantry floor pistol grants usable firearm"));break;}
  const int32 Before=P->GetInventoryEntries().Num();
  for(TActorIterator<ALZLoot> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("LZPantryMedkit"))){It->Interact(P);Check(!IsValid(*It) && P->GetInventoryEntries().Num()>Before,TEXT("pantry medical kit enters inventory and disappears from floor"));break;}
  break;
 }
 default:
  Lines.Add(FString::Printf(TEXT("VENT_QA SUMMARY %s assertions=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Assertions,Failures));UE_LOG(LogTemp,Display,TEXT("%s"),*Lines.Last());FFileHelper::SaveStringArrayToFile(Lines,*(FPaths::ProjectSavedDir()/TEXT("LZVentQA_Report.txt")));SetActorTickEnabled(false);FPlatformMisc::RequestExit(false);break;
 }
}
