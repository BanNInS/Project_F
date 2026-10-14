/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Android plugin for Unreal Engine
 * Module   : Build Settings of CriWareAndroid Module
 * File     : CriWareAndroid.Build.cs
 *
 ****************************************************************************/
namespace UnrealBuildTool.Rules
{
	public class CriWareAndroid : ModuleRules
	{
		public CriWareAndroid(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(new string[]
				{
					"CriWareAndroid/CriWareAndroid/Private",
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
					"Launch",
					"CriWareCore",
                    "CriWareAndroidSettings",
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
