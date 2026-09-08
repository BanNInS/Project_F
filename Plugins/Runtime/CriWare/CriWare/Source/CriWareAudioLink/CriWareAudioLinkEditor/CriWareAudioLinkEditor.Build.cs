/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLink plugin for Unreal Engine
 * Module   : Build Settings of CriWareAudioLinkEditor Module
 * File     : CriWareAudioLinkEditor.Build.cs
 *
 ****************************************************************************/

namespace UnrealBuildTool.Rules
{
	public struct CriWareAudioLinkEditor
	{
    	public static void Apply(UnrealBuildTool.Rules.CriWareCoreEditor CoreEditorModule, ReadOnlyTargetRules Target)
    	{
#if UE_5_1_OR_LATER
			CoreEditorModule.AddModule("CriWareAudioLinkEditor", "CriWareAudioLink", false);
#endif
    	}
	}
}
