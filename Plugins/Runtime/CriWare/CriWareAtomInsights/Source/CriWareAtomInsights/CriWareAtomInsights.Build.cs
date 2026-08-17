
using UnrealBuildTool;

public class CriWareAtomInsights : ModuleRules
{
	public CriWareAtomInsights(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange
		(
			new string[]
			{
				"Core",
				"TraceInsights",
				"TraceServices",
				"ToolWidgets",
			}
		);
		
		PrivateDependencyModuleNames.AddRange
		(
			new string[]
			{
				"CoreUObject",
				"InputCore",
				"OutputLog",
				"SignalProcessing",
				"SessionServices",
				"Slate",
				"SlateCore",
				"TraceLog",
				"TraceAnalysis",
				"ToolMenus",
				"Projects",
			}
		);

		if (Target.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CriWare",
				"CriWareCore",
				"CriWareAtomWidgets",
			});
		}

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CriWareCoreEditor",
				"Engine",
				"UnrealEd",
			});
		}

	}
}
