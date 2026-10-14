
using UnrealBuildTool;

public class CriWareAtomMixer : ModuleRules
{
	public CriWareAtomMixer(ReadOnlyTargetRules Target) : base(Target)
    {

		PrivateIncludePaths.AddRange(
			new string[] {
                "CriWareAtomMixer/Private",
            }
		);

	    PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"CriWareCore",
				"CriWare",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
            }
		);
	}
}
