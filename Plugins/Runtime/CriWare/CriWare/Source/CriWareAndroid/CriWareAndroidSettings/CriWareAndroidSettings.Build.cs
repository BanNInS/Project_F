
using UnrealBuildTool;

public class CriWareAndroidSettings : ModuleRules
{
	public CriWareAndroidSettings(ReadOnlyTargetRules Target) : base(Target)
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
