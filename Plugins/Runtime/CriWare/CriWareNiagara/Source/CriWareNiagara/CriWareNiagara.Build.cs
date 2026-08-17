/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : Build Settings of CriWareNiagara Module
 * File     : CriWareNiagara.Build.cs
 *
 ****************************************************************************/

using UnrealBuildTool;

public class CriWareNiagara : ModuleRules
{
	public CriWareNiagara(ReadOnlyTargetRules Target) : base(Target)
    {

		PrivateIncludePaths.AddRange(
			new string[] {
                "CriWareNiagara/Private",
            }
		);

	    PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"NiagaraCore",
				"Niagara",
				"CriWareCore",
            }
		);
	}
}
