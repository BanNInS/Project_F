
using UnrealBuildTool;

public class CriWareAtomWidgets : ModuleRules
{
	public CriWareAtomWidgets(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"SignalProcessing",
				"AudioPlatformConfiguration",
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CriWareCore",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "AdvancedWidgets",
				"UMG",
                "InputCore",
				"EnhancedInput",
				"Projects",
				"AudioWidgets",
			}
		);
	}
}
