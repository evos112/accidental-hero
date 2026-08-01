using UnrealBuildTool;
using System.Collections.Generic;

public class AccidentalHeroTarget : TargetRules
{
	public AccidentalHeroTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("AccidentalHero");
	}
}
