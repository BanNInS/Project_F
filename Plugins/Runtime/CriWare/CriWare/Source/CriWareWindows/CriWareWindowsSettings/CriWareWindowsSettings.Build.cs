// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CriWareWindowsSettings : ModuleRules
{
	public CriWareWindowsSettings(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "Engine",
                "CriWare",
                "CriWareCore",
                "DeveloperSettings"
            }
		);

		if (Target.bBuildEditor)
		{
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "SlateCore",
                    "Slate",
				}
            );
        }
	}
}
