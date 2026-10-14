/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Apple plugin for Unreal Engine
 * Module   : Build Settings of CriWareApple Module
 * File     : CriWareApple.Build.cs
 *
 ****************************************************************************/
namespace UnrealBuildTool.Rules
{
	public class CriWareApple : ModuleRules
	{
		public CriWareApple(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(new string[]
				{
					"CriWareApple/CriWareApple/Private",
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
                    "CriWareAppleSettings",
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
