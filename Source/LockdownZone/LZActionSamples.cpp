#include "LZActionSamples.h"
#include "Engine/World.h"
bool LZActionSamples::IsLab(const UObject* C){return C && C->GetWorld() && C->GetWorld()->GetMapName().Contains(TEXT("L_Chapter1AnimationLab"));}
bool LZActionSamples::Enabled(const UObject* C){return IsLab(C) || GetDefault<ULZActionSampleSettings>()->EnableInChapter;}
