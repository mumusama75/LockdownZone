#include "LZAcoustics.h"
#include "LZGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Perception/AISense_Hearing.h"
bool LZAcoustics::Emit(UObject* Context,FVector Position,float Radius,FName Clip,FName Category,float Volume,UAudioComponent* Speaker)
{
 if(!Context || !Context->GetWorld())return false;
 auto* GM=Context->GetWorld()->GetAuthGameMode<ALZGameMode>();if(GM && GM->IsRunOver())return false;
 const FString Path=TEXT("/Game/Gameplay/ChapterAudio/")+Clip.ToString();
 auto* Sound=LoadObject<USoundBase>(nullptr,*Path);if(!Sound)return false;
 auto* Att=NewObject<USoundAttenuation>(Context);Att->Attenuation.bAttenuate=true;
 Att->Attenuation.bEnableOcclusion=true;Att->Attenuation.OcclusionVolumeAttenuation=.5f;Att->Attenuation.OcclusionLowPassFilterFrequency=1500;
 Att->Attenuation.bSpatialize=true;Att->Attenuation.AttenuationShapeExtents=FVector(80);
 Att->Attenuation.FalloffDistance=FMath::Max(250.f,Radius);
 if(Speaker){Speaker->AttenuationSettings=Att;Speaker->SetSound(Sound);Speaker->SetVolumeMultiplier(Volume);Speaker->Play();}
 else UGameplayStatics::SpawnSoundAtLocation(Context,Sound,Position,FRotator::ZeroRotator,Volume,1,0,Att);
 if(Radius>0)UAISense_Hearing::ReportNoiseEvent(Context,Position,Radius/1000.f,Cast<AActor>(Context),0,Category);
 return true;
}
