/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Build Settings of CriWareCoreEditor Module
 * File     : CriWareCoreEditor.Build.cs
 *
 ****************************************************************************/
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class CriWareCoreEditor : ModuleRules
	{
		public CriWareCoreEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
                    "CriWareCoreBlueprintGraph/Private",
                    "CriWareCoreEditor/Private",
                    "CriWareCore/Private",
				}
			);

	        PrivateDependencyModuleNames.AddRange(
				new string[] {
					"CriWareCore",
					"CriWareCoreEditorSettings",
                    "CriWareCoreBlueprintGraph",
                    "ApplicationCore",
					"EditorStyle",
					"GraphEditor",
					"ToolMenus",
					"AssetDefinition",
					"ContentBrowser",
					"InputCore",
					"DetailCustomizations",
					"PropertyEditor",
					"Slate",
					"SlateCore",
					"EditorFramework",
					"EditorWidgets",
					"DeveloperSettings",
					"UMG",
					"UMGEditor",
					"WorkspaceMenuStructure",
					"PlacementMode",
					"AssetManagerEditor",
					"Projects",
					"Networking",
					"Kismet",
					"KismetCompiler",
					"BlueprintGraph",
					"CurveEditor",
					"WaveTable",
					"WaveTableEditor",
				}
			);

            PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
				}
			);

			CriWareAudioLinkEditor.Apply(this, Target);
        }

		public void AddModule(string Module, string ModuleRootDir = "", bool AddPublic = true)
		{
			string ModuleDir = Path.Combine(ModuleRootDir, Module);

			ConditionalAddModuleDirectory(EpicGames.Core.DirectoryReference.Combine(new EpicGames.Core.DirectoryReference(ModuleDirectory), "..", ModuleDir));

			ExternalDependencies.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, Module + ".Build.cs"));
			if (AddPublic)
			{
				PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, "Public"));
			}
			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, "Private"));
		}
	}
}