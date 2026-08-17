
using UnrealBuildTool;

public class CriWareAppleSettings : ModuleRules
{
	public CriWareAppleSettings(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "Engine",
                "CriWare",
                "CriWareCore",
                "DeveloperSettings"
            }
		);

		if (Target.bBuildEditor)
		{
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "SlateCore",
                    "Slate",
				}
            );
        }
	}
}
