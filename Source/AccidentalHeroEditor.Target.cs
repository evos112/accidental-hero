using UnrealBuildTool;
using System.Collections.Generic;

public class AccidentalHeroEditorTarget : TargetRules
{
	public AccidentalHeroEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("AccidentalHero");
	}
}
