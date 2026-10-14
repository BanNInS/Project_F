
using UnrealBuildTool;

public class CriWareAtomMixerEditor : ModuleRules
{
	public CriWareAtomMixerEditor(ReadOnlyTargetRules Target) : base(Target)
    {

	PrivateDependencyModuleNames.AddRange(
		new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"EditorStyle",
			"Projects",
			"AssetDefinition",
			"CriWareCore",
			"CriWareCoreEditor",
			"CriWareAtomMixer",
			}
		);
	}
}
