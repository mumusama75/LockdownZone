#include "LZCrowbarVisual.h"
#include "GameFramework/Actor.h"
#include "GameFramework/InputSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
USceneComponent* LZCrowbarVisual::Build(AActor* Owner,USceneComponent* Parent)
{
 auto* Root=NewObject<USceneComponent>(Owner);Owner->AddInstanceComponent(Root);Root->SetupAttachment(Parent);
 Root->SetAbsolute(false,false,true);Root->RegisterComponent();
 auto* Steel=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_PaintedSteel.M_ZT_PaintedSteel")),Owner);
 Steel->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.24f,.28f,.30f));Steel->SetVectorParameterValue(TEXT("Color"),FLinearColor(.24f,.28f,.30f));
 const FVector Points[]={FVector(0,0,-36),FVector(0,0,28),FVector(2,0,34),FVector(8,0,37),FVector(13,0,33),FVector(13,0,26)};
 for(int I=0;I<5;++I)
 {
  auto* M=NewObject<UStaticMeshComponent>(Owner);Owner->AddInstanceComponent(M);M->SetupAttachment(Root);
  M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
  M->SetRelativeLocation((Points[I]+Points[I+1])*.5f);M->SetRelativeRotation(FRotationMatrix::MakeFromZ(Points[I+1]-Points[I]).Rotator());
  M->SetRelativeScale3D(FVector(.026f,.026f,FVector::Dist(Points[I],Points[I+1])/100));
  M->SetMaterial(0,Steel);M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetCanEverAffectNavigation(false);M->RegisterComponent();
 }
 return Root;
}
FString LZCrowbarVisual::InteractionKey()
{
 TArray<FInputActionKeyMapping> Mappings;GetDefault<UInputSettings>()->GetActionMappingByName(TEXT("Interact"),Mappings);
 for(const auto& M:Mappings)if(!M.Key.IsGamepadKey())return TEXT("[")+M.Key.GetDisplayName().ToString()+TEXT("] ");
 return TEXT("[交互] ");
}
