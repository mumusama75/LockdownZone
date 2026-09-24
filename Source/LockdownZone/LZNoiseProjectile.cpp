#include "LZNoiseProjectile.h"
#include "LZAcoustics.h"
#include "LZStealthSettings.h"
#include "LZChapter.h"
#include "LZGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
ALZNoiseProjectile::ALZNoiseProjectile()
{
 Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bottle"));SetRootComponent(Shape);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 Shape->SetStaticMesh(Asset.Object);Shape->SetWorldScale3D(FVector(.065f,.065f,.20f));Shape->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 Shape->SetCanEverAffectNavigation(false);
 Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));Movement->SetUpdatedComponent(Shape);
 Movement->bShouldBounce=true;Movement->Bounciness=.35f;Movement->Friction=.4f;Movement->ProjectileGravityScale=1;
 Movement->bForceSubStepping=true;Movement->OnProjectileBounce.AddDynamic(this,&ALZNoiseProjectile::Bounce);
 InitialLifeSpan=10;
}
void ALZNoiseProjectile::Launch(FVector Velocity){Movement->Velocity=Velocity;Shape->IgnoreActorWhenMoving(GetOwner(),true);}
void ALZNoiseProjectile::Bounce(const FHitResult& Hit,const FVector& Velocity)
{
 if(++Impacts>2)return;
 if(Impacts==1){FirstImpact=Hit.ImpactPoint;Shape->SetVisibility(false);Shape->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Shape->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);Shape->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);}
 if(auto* GM=GetWorld()->GetAuthGameMode<ALZGameMode>();GM && GM->GetChapter()){GM->GetChapter()->Noise(Hit.ImpactPoint,Impacts==1?GetDefault<ULZStealthSettings>()->BottleRadius:100,TEXT("BottleBreak"),Impacts==1?TEXT("Bottle"):TEXT("Debris"));return;}
 LZAcoustics::Emit(this,Hit.ImpactPoint,Impacts==1?GetDefault<ULZStealthSettings>()->BottleRadius:100,TEXT("BottleBreak"),Impacts==1?TEXT("Bottle"):TEXT("Debris"));
}
