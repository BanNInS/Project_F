/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2024 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : Build Settings of CriWareSoundXR Module
 * File     : CriWareSoundXR.Build.cs
 *
 ****************************************************************************/

using System.IO;
using UnrealBuildTool;

public class CriWareSoundXR : ModuleRules
{
	public CriWareSoundXR(ReadOnlyTargetRules Target) : base(Target)
    {
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../CriWareApi/Public"));

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"CriWareCore",
				"CriWare",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
            }
		);
	}
}
