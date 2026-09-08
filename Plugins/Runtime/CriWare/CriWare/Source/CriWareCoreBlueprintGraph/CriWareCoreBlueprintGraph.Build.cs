/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Build Settings of CriWareCoreBlueprintGraph Module
 * File     : CriWareCoreBlueprintGraph.Build.cs
 *
 ****************************************************************************/

namespace UnrealBuildTool.Rules
{
	public class CriWareCoreBlueprintGraph : ModuleRules
	{
		public CriWareCoreBlueprintGraph(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
                    "CriWareCoreEditor/Private",
                    "CriWareCore/Private",
				}
			);

	        PrivateDependencyModuleNames.AddRange(
				new string[] {
					"CriWareCore",
					"CriWareCoreEditorSettings",
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
					"BlueprintGraph",
					"GraphEditor",
					"Kismet",
					"KismetCompiler",
					"Slate",
					"SlateCore",
					"ToolMenus",
				}
			);
        }
	}
}
