/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Linux plugin for Unreal Engine
 * Module   : Build Settings of CriWareLinux Module
 * File     : CriWareLinux.Build.cs
 *
 ****************************************************************************/
namespace UnrealBuildTool.Rules
{
	public class CriWareLinux : ModuleRules
	{
		public CriWareLinux(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(new string[]
				{
					"CriWareLinux/CriWareLinux/Private",
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
                    "CriWareLinuxSettings",
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
