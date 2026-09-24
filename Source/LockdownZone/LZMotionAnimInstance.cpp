#include "LZMotionAnimInstance.h"
#include "LZEnemy.h"
#include "LZMotionProfile.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_ApplyAdditive.h"
#include "AnimNodes/AnimNode_PoseSnapshot.h"
struct FLZMotionProxy : FAnimInstanceProxy
{
    FAnimNode_BlendSpacePlayer_Standalone Movement;
    FAnimNode_SequenceEvaluator_Standalone Action;
    FAnimNode_ApplyAdditive Additive;
    FAnimNode_TwoWayBlend Root;
    FAnimNode_TwoWayBlend Recovery;
    FAnimNode_PoseSnapshot Snapshot;
    float Weight=0;
    FLZMotionProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance)
    {
        Additive.Base.SetLinkNode(&Movement);
        Additive.Additive.SetLinkNode(&Action);
        Root.A.SetLinkNode(&Additive);
        Snapshot.Mode=ESnapshotSourceMode::SnapshotPin;
        Recovery.A.SetLinkNode(&Snapshot);
        Recovery.B.SetLinkNode(&Action);
        Root.B.SetLinkNode(&Recovery);
        Action.SetTeleportToExplicitTime(true);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Root; }
    virtual void PreUpdate(UAnimInstance* Instance,float DeltaSeconds) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,DeltaSeconds);
        ALZEnemy* Enemy=Cast<ALZEnemy>(Instance->TryGetPawnOwner());
        if(!Enemy || !Enemy->GetMotionProfile()) return;
        auto* Profile=Enemy->GetMotionProfile();
        Snapshot.Snapshot=Enemy->GetRecoveryPose();
        Recovery.Alpha=Enemy->GetRecoveryBlend();
        Movement.SetBlendSpace(Profile->Locomotion);
        const FVector Local=Enemy->GetActorRotation().UnrotateVector(Enemy->GetVelocity());
        const float Direction=FMath::RadiansToDegrees(FMath::Atan2(Local.Y,Local.X));
        const float Speed=Enemy->GetVelocity().Size2D();
        // Epic template axes: direction X, speed Y; imported profiles must retain this contract.
        Movement.SetPosition(Profile->bSpeedOnXAxis?FVector(Speed,0,0):FVector(Direction,Speed,0));
        float Time=0,DesiredWeight=0;
        UAnimSequence* Clip=Enemy->GetActionPose(Time,DesiredWeight);
        // Profiles without an authored get-up blend the captured ragdoll pose
        // back to locomotion, never to the last evaluated attack animation.
        const bool bSnapshotOnly=Enemy->GetMotionState()==ELZEnemyMotionState::GettingUp && !Profile->GetUp;
        Root.B.SetLinkNode(bSnapshotOnly?static_cast<FAnimNode_Base*>(&Snapshot):static_cast<FAnimNode_Base*>(&Recovery));
        if(Clip) { Action.SetSequence(Clip); Action.SetExplicitTime(Time); }
        Weight=FMath::FInterpTo(Weight,DesiredWeight,DeltaSeconds,14);
        const bool bAdditive=Clip && Clip->GetAdditiveAnimType()!=AAT_None;
        Additive.Alpha=bAdditive?Weight:0;
        Root.Alpha=bAdditive?0:Weight;
    }
};
FAnimInstanceProxy* ULZMotionAnimInstance::CreateAnimInstanceProxy() { return new FLZMotionProxy(this); }
void ULZMotionAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
