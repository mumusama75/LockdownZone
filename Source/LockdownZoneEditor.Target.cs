using UnrealBuildTool;
using System.Collections.Generic;

public class LockdownZoneEditorTarget : TargetRules
{
    public LockdownZoneEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("LockdownZone");
    }
}
