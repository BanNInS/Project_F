/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLink plugin for Unreal Engine
 * Module   : Build Settings of CriWareAudioLink Module
 * File     : CriWareAudioLink.Build.cs
 *
 ****************************************************************************/

namespace UnrealBuildTool.Rules
{
	public struct CriWareAudioLink
	{
		public static void Apply(UnrealBuildTool.Rules.CriWareCore CoreModule, ReadOnlyTargetRules Target)
		{
#if UE_5_1_OR_LATER
			CoreModule.AddModule("CriWareAudioLink", "CriWareAudioLink");
			CoreModule.PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AudioLinkCore",
					"AudioLinkEngine",
					"SignalProcessing"
				}
			);
#endif
		}
	}
}
