/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : Build Settings of CriWareNiagara Module
 * File     : CriWareNiagaraEditor.Build.cs
 *
 ****************************************************************************/

using UnrealBuildTool;

public class CriWareNiagaraEditor : ModuleRules
{
    public CriWareNiagaraEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(
            new string[] {
                "CriWareNiagaraEditor/Private",
                "CriWareNiagara/Private",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "CriWareCore",
                "CriWareNiagara",
                "CriWareCoreEditor",
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
