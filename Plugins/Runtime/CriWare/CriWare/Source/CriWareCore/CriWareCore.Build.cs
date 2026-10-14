/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Build Settings of CriWareCore Module
 * File     : CriWareCore.Build.cs
 *
 ****************************************************************************/
using EpicGames.Core;
using System;
using System.IO;
using System.Linq;
using UnrealBuildBase;

namespace UnrealBuildTool.Rules
{
    public class CriWareCore : ModuleRules
    {
		public CriWareCore(ReadOnlyTargetRules Target) : base(Target)
        {
			PrivateIncludePaths.AddRange(new string[]
                {
                    "CriWareCore/Private/Atom",
                    "CriWareCore/Private",
                }
            );
            /* LE */
            {
                System.Console.WriteLine("CRIWARE_UE_LE=1");
                PublicDefinitions.Add("CRIWARE_UE_LE=1");
            }

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "CoreUObject",
                    "RHI",
                    "RenderCore",
                    "SlateCore",
                    "DeveloperSettings",
                    "HeadMountedDisplay",
                    "Projects",
					"SignalProcessing",
					"AudioPlatformConfiguration",
					"GameplayTags",
					"Networking",
					"TraceLog",
				}
            );

            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Engine",
                    "UMG",
                    "Core",
					"WaveTable",
					"CriWare",
				}
            );

            if (Target.bBuildEditor == true)
            {
                PrivateIncludePathModuleNames.AddRange(
                    new string[] {
                        "CriWareCoreEditor"
                    }
                );

                PublicIncludePathModuleNames.AddRange(
                    new string[] {
                        "TargetPlatform",
                    }
                );

                PrivateDependencyModuleNames.AddRange(
                    new string[] {
                        "CriWareCoreEditorSettings"
                    }
                );

                PublicDependencyModuleNames.AddRange(
                    new string[] {
                        "UnrealEd",
                        "Slate",
                    }
                );
            }

			CriWareAudioLink.Apply(this, Target);
		}

		public void AddModule(string Module, string ModuleRootDir = "", bool AddPublic = true)
		{
			string ModuleDir = Path.Combine(ModuleRootDir, Module);

			ConditionalAddModuleDirectory(EpicGames.Core.DirectoryReference.Combine(new EpicGames.Core.DirectoryReference(ModuleDirectory), "..", ModuleDir));

			ExternalDependencies.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, Module + ".Build.cs"));
			if (AddPublic)
			{
				PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, "Public"));
			}
			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", ModuleDir, "Private"));
		}
	}
}
