
namespace UnrealBuildTool.Rules
{
	public class CriWareSoundscapeEditor : ModuleRules
	{
		public CriWareSoundscapeEditor(ReadOnlyTargetRules Target) : base(Target)
        {
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"AssetDefinition",
					"CriWareCore",
					"CriWareSoundscape",
					"CriWareCoreEditor",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"CoreUObject",
					"Engine",
					"Slate",
					"SlateCore",
					"UnrealEd",
				}
			);
		}
	}
}
