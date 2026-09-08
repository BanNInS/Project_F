/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Build Settings of CriWareCoreEditorSettings Module
 * File     : CriWareCoreEditorSettings.Build.cs
 *
 ****************************************************************************/
namespace UnrealBuildTool.Rules
{
	public class CriWareCoreEditorSettings : ModuleRules
	{
		public CriWareCoreEditorSettings(ReadOnlyTargetRules Target) : base(Target)
		{
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "DeveloperSettings",
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "SlateCore",
                    "Slate",
                }
            );
        }
	}
}