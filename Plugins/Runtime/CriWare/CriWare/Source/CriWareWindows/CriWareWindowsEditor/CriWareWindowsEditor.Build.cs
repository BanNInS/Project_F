// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CriWareWindowsEditor : ModuleRules
{
	public CriWareWindowsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"CriWareWindows/CriWareWindowsEditor/Private",
				"CriWareWindows/CriWareWindows/Private",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CriWareCore",
				"CriWareCoreEditor",
				"CriWareWindowsSettings",
				"Core",
				"CoreUObject",
				"InputCore",
				"DesktopPlatform",
				"Engine",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"PropertyEditor",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			PrivateDependencyModuleNames.Add("CriWareWindows");
		}

		PublicDependencyModuleNames.AddRange(
			new string[] {
				
			}
		);
	}
}
