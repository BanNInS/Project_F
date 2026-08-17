
namespace UnrealBuildTool.Rules
{
	public class CriWareSoundscape : ModuleRules
	{
        public CriWareSoundscape(ReadOnlyTargetRules Target) : base(Target)
        {
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"DeveloperSettings",
					"GameplayTags",	
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"CoreUObject",
					"Engine",
					"CriWareCore",
				}
			);
		}
	}
}
