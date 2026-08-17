/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2024 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : Build Settings of CriWareSoundXR Module
 * File     : CriWareSoundXREditor.Build.cs
 *
 ****************************************************************************/

using UnrealBuildTool;

public class CriWareSoundXREditor : ModuleRules
{
    public CriWareSoundXREditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(
            new string[] {
                "CriWareSoundXREditor/Private",
                "CriWareSoundXR/Private",
            }
        );

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
				"CriWareCore",
				"CriWareCoreEditor",
				"CriWareSoundXR",
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
