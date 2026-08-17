/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2025 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Build Settings of CriWare Module
 * File     : CriWare.Build.cs
 *
 ****************************************************************************/

using EpicGames.Core;
using UnrealBuildBase;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace UnrealBuildTool.Rules
{
	public class CriWare : ModuleRules
	{
		public readonly string CriSDKPath = Path.Combine("Source", "CriWare", "SDK");
		public virtual string CriSDKPlatformLabel { get; private set; } = "unknown";

		protected List<CriSDKModule> SDKModules;
		protected CriSDKModule CoreLE;
		protected CriSDKModule CoreAtom;
		protected CriSDKModule SoundXRExtension;
		public CriWare(ReadOnlyTargetRules Target) : base(Target)
		{
			Type = ModuleType.External;

			// Supported CriWare platform.
			if (CriSDKPlatformLabel == "unknown")
			{
				CriSDKPlatformLabel = GetCriSDKPlatformLabel();
			}

			PublicDefinitions.Add(string.Format("WITH_CRIWARE={0}", CriSDKPlatformLabel! == "unsupported" ? 0 : 1));

			// Declare Cri SDK modules
			CoreLE = new CriSDKModule(this, "CriWare", "cri_le_xpt.h", "CRIWARE_UE_LE=1", true);
			CoreAtom = new CriSDKModule(this, "CriWare", "cri_le_atom.h", "CRIWARE_USE_ATOM=1");
			SoundXRExtension = new CriSDKModule(this, "CriWareSoundXR", "crisoundxr_le_plugin.h", "CRIWARE_USE_SOUNDXR=1");

			SDKModules = new List<CriSDKModule> {
				CoreLE,
				CoreAtom,
				SoundXRExtension,
			};

			// Per platform rules
			UnrealTargetPlatform TargetPlatformValue;

			// Win64
			if (Target.Platform == UnrealTargetPlatform.Win64 ||
			   (UnrealTargetPlatform.TryParse("WinGDK", out TargetPlatformValue) && Target.Platform == TargetPlatformValue))
			{
				PublicSystemLibraries.Add("mfplat.lib");
				PublicSystemLibraries.Add("mfuuid.lib");
				PublicSystemLibraries.Add("propsys.lib");
				if (Target.WindowsPlatform.bNeedsLegacyStdioDefinitionsLib)
				{
					PublicSystemLibraries.Add("legacy_stdio_definitions.lib");
				}

				SDKModules.ForEach(Module => Module.SDKTargetArchs.Add("x64"));

				if (Target.Configuration == UnrealTargetConfiguration.Shipping)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"cri_ware_pcx64_le_import.lib",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Test)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"cri_ware_pcx64_le_import.lib",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"cri_ware_pcx64_le_import.lib",
						}
					);
				}
				else
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"cri_ware_pcx64_le_import.lib",
						}
					);
				}
				PublicDelayLoadDLLs.Add("cri_ware_pcx64_le.dll");
				RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "SDK", "pc", "libs", "x64", "cri_ware_pcx64_le.dll"));
			}
			// Mac
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PublicFrameworks.AddRange(new string[] { "CoreAudio", "AudioToolbox", "AudioUnit", "CoreGraphics", "VideoToolbox", "OpenGL" });

				if (Target.Configuration == UnrealTargetConfiguration.Shipping)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_macosx_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Test)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_macosx_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_macosx_le.a",
						}
					);
				}
				else
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_macosx_le.a",
						}
					);
				}
			}
			// IOS
			else if (Target.Platform == UnrealTargetPlatform.IOS)
			{
				PublicFrameworks.AddRange(new string[] { "CoreAudio", "AudioToolbox", "CoreGraphics", "VideoToolbox", "OpenGLES" });
				
				if (Target.Configuration == UnrealTargetConfiguration.Shipping)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_ios_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Test)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_ios_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_ios_le.a",
						}
					);
				}
				else
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_ios_le.a",
						}
					);
				}
			}
			// Android
			else if (Target.Platform == UnrealTargetPlatform.Android)
			{
				SDKModules.ForEach(Module => Module.SDKTargetArchs.AddRange(
					new string[] {
						"armeabi-v7a",
						"arm64-v8a",
						"x86_64"
					})
				);

				if (Target.Configuration == UnrealTargetConfiguration.Shipping)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_android_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Test)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_android_le.a",
						}
					);
				}
				else if (Target.Configuration == UnrealTargetConfiguration.Debug)
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_android_le.a",
						}
					);
				}
				else
				{
					CoreAtom.AddPublicSDKLibraries(
						new string[] {
							"libcri_ware_android_le.a",
						}
					);
				}
			}
		}

		public bool IsCriWarePluginEnabled(string PluginName)
		{
			var UPluginInfo = Plugins.GetPlugin(PluginName);
			if (UPluginInfo != null)
			{
				if (Target.ProjectFile == null)
				{
					// Engine build, everything is linked
					return true;
				}

				var ProjectDesc = ProjectDescriptor.FromFile(Target.ProjectFile);
				var Result = Plugins.IsPluginEnabledForTarget(UPluginInfo, ProjectDesc, Target.Platform, Target.Configuration, Target.Type);
				return Result;
			}

			return false;
		}

		public string GetCriSDKPathForPlugin(string PluginName)
		{
			var UPluginInfo = Plugins.GetPlugin(PluginName);
			if (UPluginInfo != null)
			{
				var File = UPluginInfo.ChildFiles.FirstOrDefault(x => x.GetFileName().EndsWith(Target.Platform.ToString() + ".uplugin"), UPluginInfo.File);
				return Path.Combine(File.Directory.FullName, CriSDKPath, CriSDKPlatformLabel);
			}

			return "";
		}

		public string GetCriSDKPlatformLabel()
		{
			UnrealTargetPlatform TargetPlatformValue;

			string SDKPlatformLabel = "unsupported";
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				SDKPlatformLabel = "pc";
			}
			else if (UnrealTargetPlatform.TryParse("WinGDK", out TargetPlatformValue) && Target.Platform == TargetPlatformValue)
			{
				SDKPlatformLabel = "pc";
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				SDKPlatformLabel = "macosx";
			}
			else if (Target.Platform == UnrealTargetPlatform.IOS)
			{
				SDKPlatformLabel = "ios";
			}
			else if (Target.IsInPlatformGroup(UnrealPlatformGroup.Android))
			{
				SDKPlatformLabel = "android";
			}

			return SDKPlatformLabel;
		}

		protected class CriSDKModule
		{
			public bool IsEnabled { get; }
			public string PluginName { get; }
			public string SDKLibFilename { get; }
			public string SDKPath { get; }
			public string SDKIncludePath { get => Path.Combine(SDKPath, "include"); }
			public string SDKLibrariesPath { get => Path.Combine(SDKPath, "libs"); }
			public List<string> SDKTargetArchs { get; set; }
			private CriWare OwnerModule;

			public CriSDKModule(CriWare CriWareModule, string PluginName, string SDKLibFilename, string PublicDefinition, bool IsOptionalLibExtension = false)
			{
				OwnerModule = CriWareModule;
				this.PluginName = PluginName;
				this.SDKLibFilename = SDKLibFilename;
				SDKTargetArchs = new List<string>();
				IsEnabled = false;

				if (OwnerModule.IsCriWarePluginEnabled(PluginName))
				{
					SDKPath = OwnerModule.GetCriSDKPathForPlugin(PluginName);
					if (SDKPath != null && File.Exists(Path.Combine(SDKIncludePath, SDKLibFilename)))
					{
						IsEnabled = true;
						System.Console.WriteLine("CriWare: SDK module for {0} added. ({1})", PluginName, PublicDefinition);
						OwnerModule.PublicDefinitions.Add(PublicDefinition);
						if (!IsOptionalLibExtension)
						{
							OwnerModule.PublicSystemIncludePaths.Add(SDKIncludePath);
						}
					}
					else if (!IsOptionalLibExtension)
					{
						ConsoleUtils.WriteError($"Missing header file {SDKLibFilename} for {PluginName} plugin. Please check your plugin installation.");
					}
				}
			}

			public void AddPublicSDKLibraries(string[] LibFilenames)
			{
				if (IsEnabled)
				{
					if (SDKTargetArchs.Count() == 0)
					{
						foreach (var LibFilename in LibFilenames)
						{
							OwnerModule.PublicAdditionalLibraries.Add(Path.Combine(SDKLibrariesPath, LibFilename));
						}
					}

					foreach (var Arch in SDKTargetArchs)
					{
						foreach (var LibFilename in LibFilenames)
						{
							OwnerModule.PublicAdditionalLibraries.Add(Path.Combine(SDKLibrariesPath, Arch, LibFilename));
						}
					}
				}
			}

			public void AddPublicSDKLibrary(string LibFilename)
			{
				AddPublicSDKLibraries(new string[] { LibFilename });
			}
		}
	}
}