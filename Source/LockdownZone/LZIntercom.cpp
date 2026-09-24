#include "LZIntercom.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "LZCharacter.h"
#include "LZStealthSettings.h"
#include "LZAcoustics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
ALZIntercom::ALZIntercom()
{
 PrimaryActorTick.bCanEverTick=true;Mesh->SetRelativeScale3D(FVector(.22f,.18f,.28f));Mesh->SetCanEverAffectNavigation(false);
 SetLabel(TEXT(""),FColor(50,75,65));Glow->SetAbsolute(false,false,true);Glow->SetRelativeLocation(FVector(0,-50,35));Glow->SetIntensity(4);Glow->SetLightColor(FLinearColor(.12f,.8f,.3f));Glow->SetAttenuationRadius(60);
 Indicator=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PowerLED"));Indicator->SetupAttachment(Mesh);Indicator->SetStaticMesh(Mesh->GetStaticMesh());Indicator->SetAbsolute(false,false,true);Indicator->SetRelativeLocation(FVector(24,-53,24));Indicator->SetRelativeScale3D(FVector(.045f,.012f,.025f));Indicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);Indicator->SetCanEverAffectNavigation(false);
 Speaker=CreateDefaultSubobject<UAudioComponent>(TEXT("RadioSpeaker"));Speaker->SetupAttachment(Mesh);Speaker->bAutoActivate=false;
}
void ALZIntercom::BeginPlay()
{
 Super::BeginPlay();
 if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/ZeroTower/Materials/M_ZT_LEDGreen.M_ZT_LEDGreen"))){auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.08f,.7f,.25f));M->SetScalarParameterValue(TEXT("EmissiveStrength"),2);Indicator->SetMaterial(0,M);}
 CallInterval=GetDefault<ULZStealthSettings>()->RadioInterval;SoundRadius=GetDefault<ULZStealthSettings>()->RadioRadius;NextCallAt=GetWorld()->GetTimeSeconds()+2;
}
bool ALZIntercom::IsPlaying() const{return Speaker->IsPlaying();}
void ALZIntercom::Tick(float Delta)
{
 Super::Tick(Delta);auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(!GM || GM->IsRunOver()){Speaker->Stop();return;}
 if(!bPowered || !GM->IsCombatUnlocked() || GetWorld()->GetTimeSeconds()<NextCallAt)return;
 const float Now=GetWorld()->GetTimeSeconds();NextCallAt=Now+FMath::Max(4.f,CallInterval);
 if(LZAcoustics::Emit(this,GetActorLocation(),SoundRadius,TEXT("RadioCall"),TEXT("Radio"),GetDefault<ULZStealthSettings>()->RadioVolume,Speaker)){++EmissionCount;LastEmissionAt=Now;}
}
void ALZIntercom::Interact(ALZCharacter* P)
{
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();
 if(!P || !GM || GM->IsRunOver() || P->IsInventoryOpen() || !bPowered || FVector::Dist(P->GetActorLocation(),GetActorLocation())>230)return;
 bPowered=false;Speaker->Stop();Glow->SetIntensity(0);if(auto* M=Cast<UMaterialInstanceDynamic>(Indicator->GetMaterial(0))){M->SetScalarParameterValue(TEXT("EmissiveStrength"),0);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.01f,.02f,.01f));}
 if(auto* C=GM->GetChapter())C->ShowFeedback(TEXT("对讲机已关闭"));
}
FString ALZIntercom::GetInteractionPrompt(const ALZCharacter* P) const{return bPowered?TEXT("[E] 关闭值班对讲机"):TEXT("对讲机已关闭");}
FString ALZIntercom::AudibleSubtitle(const ALZCharacter* P) const
{
 if(!bPowered || !P || GetWorld()->GetTimeSeconds()-LastEmissionAt>GetDefault<ULZStealthSettings>()->RadioSubtitleSeconds || FVector::Dist(P->GetActorLocation(),GetActorLocation())>SoundRadius)return FString();
 return TEXT("对讲机：东侧电梯停运，值守人员请回应……");
}
ALZAccessCard::ALZAccessCard()
{
 Mesh->SetRelativeScale3D(FVector(.10f,.15f,.012f));Mesh->SetCanEverAffectNavigation(false);
 SetLabel(TEXT(""),FColor(200,190,140));Glow->SetIntensity(0);
}
void ALZAccessCard::Interact(ALZCharacter* P)
{
 auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();auto* C=GM?GM->GetChapter():nullptr;
 if(!P || !C || GM->IsRunOver() || P->IsInventoryOpen() || FVector::Dist(P->GetActorLocation(),GetActorLocation())>220)return;
 C->GrantAccess(Permission);C->ShowFeedback(TEXT("取得行政区门禁卡"));
 TArray<AActor*> Decorations;GetAttachedActors(Decorations);for(auto* A:Decorations)A->Destroy();Destroy();
}
FString ALZAccessCard::GetInteractionPrompt(const ALZCharacter* P) const{return TEXT("[E] 取下保安门禁卡");}
