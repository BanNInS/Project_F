
using UnrealBuildTool;

public class CriWareAtomInsightsEditor : ModuleRules
{
	public CriWareAtomInsightsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange
		(
			new string[]
			{
				"CriWareCore",
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange
		(
			new string[]
			{
				"CriWare",
				"CriWareCoreEditor",
				"CriWareAtomInsights",
				"CriWareAtomWidgets",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"InputCore",
				"OutputLog",
				"SignalProcessing",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"Projects",
				"TraceAnalysis",
				"AudioWidgets",
			}
		);
	}
}
