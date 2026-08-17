/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2018 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Apple plugin for Unreal Engine 4
 * Module   : Build Settings of CriWareApple Module
 * File     : CriWareApple.Build.cs
 *
 ****************************************************************************/
namespace UnrealBuildTool.Rules
{
	public class CriWareWindows : ModuleRules
	{
		public CriWareWindows(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(new string[]
				{
					"CriWareWindows/CriWareWindows/Private",
					"CriWareCore/Private",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Projects",
					"RenderCore",
					"RHI",
					"CriWareCore",
                    "CriWareWindowsSettings",
                }
			);

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Engine",
				}
			);
		}
	}
}
