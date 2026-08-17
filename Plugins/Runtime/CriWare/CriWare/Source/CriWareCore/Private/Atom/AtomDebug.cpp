/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomDebug.cpp
 *
 ****************************************************************************/

#include "Atom/AtomDebug.h"

#include "CanvasTypes.h"
#include "Components/AudioComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Engine/ViewportStatsSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Serialization/Archive.h"
#include "UnrealEngine.h"

#include "CriWareCoreSettings.h"
#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomVirtualLoop.h"
#include "Atom/AtomAttenuation.h"
#include "Atom/AtomSoundClass.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"
#include "Atom/AtomConcurrency.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWavePlayer.h"
#include "Atom/Modulation/AtomModulationSystem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#endif // WITH_EDITOR


#ifndef ENABLE_ATOM_DEBUG
#error "Please define ENABLE_ATOM_DEBUG"
#endif //ENABLE_ATOM_DEBUG

#if ENABLE_ATOM_DEBUG

// Console variables
static int32 AtomActiveSoundVisualizeModeCVar = 1;
FAutoConsoleVariableRef CVarAtomVisualizeActiveSoundsMode(
	TEXT("atom.3dVisualize.ActiveSounds"),
	AtomActiveSoundVisualizeModeCVar,
	TEXT("Visualization mode for Atom active sounds. \n")
	TEXT("0: Not Enabled, 1: Volume (Lin), 2: Volume (dB), 3: Distance, 4: Random color, 5: Occlusion"),
	ECVF_Default);

static int32 AtomActiveSoundVisualizeListenersCVar = 0;
FAutoConsoleVariableRef CVarAtomVisualizeListeners(
	TEXT("atom.3dVisualize.Listeners"),
	AtomActiveSoundVisualizeListenersCVar,
	TEXT("Whether or not listeners are visible when 3d visualize is enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomActiveSoundVisualizeTypeCVar = 0;
FAutoConsoleVariableRef CVarAtomVisualizeActiveSounds(
	TEXT("atom.3dVisualize.ActiveSounds.Type"),
	AtomActiveSoundVisualizeTypeCVar,
	TEXT("Whether to show all sounds, on AtomComponents (Components Only), or off of AtomComponents (Non-Component Only). \n")
	TEXT("0: All, 1: Components Only, 2: Non-Component Only"),
	ECVF_Default);

static int32 AtomMultiPositionSourcesVisualizeEnabledCVar = 1;
FAutoConsoleVariableRef CVarAtomMultiPositionSourcesVisualizeEnabled(
	TEXT("atom.3dVisualize.MultiPositionSources"),
	AtomMultiPositionSourcesVisualizeEnabledCVar,
	TEXT("Whether or not multi position sources are visible when 3d visualize is enabled. If not only the original source position is visisble. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSpatialSourceVisualizeEnabledCVar = 1;
FAutoConsoleVariableRef CVarAtomVisualizeSpatialSourceEnabled(
	TEXT("atom.3dVisualize.SpatialSources"),
	AtomSpatialSourceVisualizeEnabledCVar,
	TEXT("Whether or not Atom spatialized sources are visible when 3d visualize is enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomVirtualLoopsVisualizeEnabledCVar = 1;
FAutoConsoleVariableRef CVarAtomVisualizeVirtualLoopsEnabled(
	TEXT("atom.3dVisualize.VirtualLoops"),
	AtomVirtualLoopsVisualizeEnabledCVar,
	TEXT("Whether or not virtualized loops are visible when 3d visualize is enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomDebugSoundMaxNumDisplayedCVar = 32;
FAutoConsoleVariableRef CVarAtomDebugSoundMaxNumDisplayed(
	TEXT("atom.Debug.Sounds.Max"),
	AtomDebugSoundMaxNumDisplayedCVar,
	TEXT("Max number of sounds to display in full sound debugger view. \n")
	TEXT("Default: 32"),
	ECVF_Default);

static int32 AtomDebugSoundShowPathCVar = 1;
FAutoConsoleVariableRef CVarAtomDebugSoundShowPath(
	TEXT("atom.Debug.Sounds.ShowPath"),
	AtomDebugSoundShowPathCVar,
	TEXT("Display full path of sound when enabled.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static FString AtomDebugSoundSortCVarCVar = TEXT("Name");
FAutoConsoleVariableRef CVarAtomDebugSoundSortCVar(
	TEXT("atom.Debug.Sounds.Sort"),
	AtomDebugSoundSortCVarCVar,
	TEXT("Value to sort by and display when sound stats are active. \n")
	TEXT("Class, Distance, Name (Default), Priority (Highest of player instances per sound), Time, Waves, Volume"),
	ECVF_Default);

static FString AtomDebugStatSoundTextColorCVar = TEXT("White");
FAutoConsoleVariableRef CVarAtomDebugStatSoundColor(
	TEXT("atom.Debug.Sounds.TextColor"),
	AtomDebugStatSoundTextColorCVar,
	TEXT("Color of body text in Atom debug views. \n")
	TEXT("White, Red, Orange, Yellow, Green, Blue, Magenta, Purple, Black"),
	ECVF_Default);

static int32 AtomSoundCueDebugShowPathCVar = 1;
FAutoConsoleVariableRef CVarAtomSoundCueDebugShowPath(
	TEXT("atom.Debug.SoundCues.ShowPath"),
	AtomSoundCueDebugShowPathCVar,
	TEXT("Display full path of sound cue when enabled.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundCueDebugShowDistanceCVar = 0;
FAutoConsoleVariableRef CVarAtomSoundCueDebugShowDistance(
	TEXT("atom.Debug.SoundCues.ShowDistance"),
	AtomSoundCueDebugShowDistanceCVar,
	TEXT("Display distance of sound cue when enabled.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundCueDebugMinimalCVar = 0;
FAutoConsoleVariableRef CVarAtomSoundCueDebugMinimal(
	TEXT("atom.Debug.SoundCues.Minimal"),
	AtomSoundCueDebugMinimalCVar,
	TEXT("Use the compact view of sound cue debug when enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundCueDebugTabSpacingCVar = 5;
FAutoConsoleVariableRef CVarAtomSoundCueDebugTabSpacing(
	TEXT("atom.Debug.SoundCues.Spacing.Tab"),
	AtomSoundCueDebugTabSpacingCVar,
	TEXT("Size of tab (in characters) with compact view. \n")
	TEXT("Default: 5"),
	ECVF_Default);

static int32 AtomSoundCueDebugCharSpacingCVar = 7;
FAutoConsoleVariableRef CVarAtomSoundCueDebugCharSpacing(
	TEXT("atom.Debug.SoundCues.Spacing.Char"),
	AtomSoundCueDebugCharSpacingCVar,
	TEXT("Size of character (in pixels) with compact view. \n")
	TEXT("Default: 7"),
	ECVF_Default);

static int32 AtomSoundWaveDebugShowPathCVar = 1;
FAutoConsoleVariableRef CVarAtomSoundWaveDebugShowPath(
	TEXT("atom.Debug.SoundWaves.ShowPath"),
	AtomSoundWaveDebugShowPathCVar,
	TEXT("Display full path of sound wave when enabled.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundWaveDebugShowDistanceCVar = 0;
FAutoConsoleVariableRef CVarAtomSoundWaveDebugShowDistance(
	TEXT("atom.Debug.SoundWaves.ShowDistance"),
	AtomSoundWaveDebugShowDistanceCVar,
	TEXT("Display distance of sound wave when enabled.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundWaveDebugMinimalCVar = 0;
FAutoConsoleVariableRef CVarAtomSoundWaveDebugMinimal(
	TEXT("atom.Debug.SoundWaves.Minimal"),
	AtomSoundWaveDebugMinimalCVar,
	TEXT("Use the compact view of sound wave debug when enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 AtomSoundWaveDebugTabSpacingCVar = 5;
FAutoConsoleVariableRef CVarAtomSoundWaveDebugTabSpacing(
	TEXT("atom.Debug.SoundWaves.Spacing.Tab"),
	AtomSoundWaveDebugTabSpacingCVar,
	TEXT("Size of tab (in characters) with compact view. \n")
	TEXT("Default: 5"),
	ECVF_Default);

static int32 AtomSoundWaveDebugCharSpacingCVar = 7;
FAutoConsoleVariableRef CVarAtomSoundWaveDebugCharSpacing(
	TEXT("atom.Debug.SoundWaves.Spacing.Char"),
	AtomSoundWaveDebugCharSpacingCVar,
	TEXT("Size of character (in pixels) with compact view. \n")
	TEXT("Default: 7"),
	ECVF_Default);

static int32 AtomSoundDebugDisplayCornerXCVar = 100;
FAutoConsoleVariableRef CVarAtomDebugDisplayCornerX(
	TEXT("atom.Debug.Display.X"),
	AtomSoundDebugDisplayCornerXCVar,
	TEXT("X position on screen of debug statistics. \n")
	TEXT("Default: 100"),
	ECVF_Default);

static int32 AtomSoundDebugDisplayCornerYCVar = -1;
FAutoConsoleVariableRef CVarAtomDebugDisplayCornerY(
	TEXT("atom.Debug.Display.Y"),
	AtomSoundDebugDisplayCornerYCVar,
	TEXT("X position on screen of debug statistics. \n")
	TEXT("Default: -1 (Disabled, uses default debug position)"),
	ECVF_Default);

namespace Atom
{
	const FColor HeaderColor = FColor::Green;

	FColor GetBodyColor()
	{
		return FColor::White;
	}

	FColor GetStatSoundBodyColor()
	{
		if (AtomDebugStatSoundTextColorCVar == TEXT("Red"))
		{
			return FColor::Red;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Orange"))
		{
			return FColor::Orange;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Yellow"))
		{
			return FColor::Yellow;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Green"))
		{
			return FColor::Green;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Blue"))
		{
			return FColor::Blue;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Magenta"))
		{
			return FColor::Magenta;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Purple"))
		{
			return FColor::Purple;
		}
		if (AtomDebugStatSoundTextColorCVar == TEXT("Black"))
		{
			return FColor::Black;
		}

		return FColor::White;
	}

	// Whether or not respective stat data is active for any audio device (set on game thread)
	static bool bDebugPlaybacksForAllViewsEnabled = false;
	static bool bDebugWavesForAllViewsEnabled = false;
	static bool bDebugCuesForAllViewsEnabled = false;
	static bool bDebugSoundsForAllViewsEnabled = false;
	static bool bDebugRacksForAllViewsEnabled = false;
	//static bool bDebugReverbForAllViewsEnabled = false;
	static bool bDebugModulationForAllViewsEnabled = false;
	static bool bDebugStreamingForAllViewsEnabled = false;

	const int32 DebuggerTabWidth = 12;

	const float MinDisplayVolume = KINDA_SMALL_NUMBER; // -80 dB

	FAtomRuntime* GetWorldRuntime(UWorld* World)
	{
		check(IsInGameThread());

		if (!World)
		{
			return nullptr;
		}

		return FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);
	}

	namespace DebugStatNames
	{
		const FName SoundPlaybacks = "SoundPlaybacks";
		const FName SoundWaves = "SoundWaves";
		const FName SoundCues = "SoundCues";
		const FName Sounds = "Sounds";
		const FName SoundRacks = "SoundRacks";
		const FName SoundModulation = "SoundModulation";
		//const FName SoundReverb = "SoundReverb";
		const FName AudioStreaming = "AudioStreaming";

		// TODO: Move to console variables
		const FName DebugSounds = "DebugSounds";
		const FName LongSoundNames = "LogSoundNames";
	}

	struct FAtomStats
	{
		enum class EDisplaySort : uint8
		{
			Class,
			Distance,
			Name,
			Type,
			PlaybackTime,
			Priority,
			Playbacks,
			Volume
		};

		enum class EDisplayFlags : uint8
		{
			Debug = 0x01,
			Long_Names = 0x40,
		};

		struct FStatPlaybackInstanceInfo
		{
			TSharedPtr<FAtomSource::FDebugInfo, ESPMode::ThreadSafe> DebugInfo;
			FString Description;
			float Volume;
			int32 InstanceIndex;
			FName PlaybackInstanceName;
			FName SoundClassName;
			uint8 bPlayWhenSilent : 1;
		};

		struct FStatSoundInfo
		{
			FString SoundName;
			FString SoundPath;
			EAtomSoundType Type;
			FName SoundClassName;
			float Distance;
			float PlaybackTime;
			float PlaybackTimeNonVirtualized;
			float Priority;
			float Volume;
			uint32 AtomComponentID;
			FTransform Transform;
			TArray<FTransform> Positions;
			TArray<FStatPlaybackInstanceInfo> PlaybackInstanceInfos;

			TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails> ShapeDetailsMap;
		};

		struct FStatSoundBusInfo
		{
			FString BusName;
			float Volume;
			float InterpValue;
			bool bIsCurrentEQ;
			int32 RefCount;
			TArray<FName> EffectChainNames;
		};

		struct FStatSoundRackInfo
		{
			FString RackName;
			FString DspBusSettingsName;
			FString SnapshotName;
			int32 RefCount;
			TArray<FStatSoundBusInfo> BusInfos;
		};

		uint8 DisplayFlags;
		EDisplaySort DisplaySort;
		TSet<FName> EnabledStats;
		TArray<FTransform> ListenerTransforms;
		TArray<FStatSoundInfo> StatSoundInfos;
		TArray<FStatSoundRackInfo> StatSoundRackInfos;

		FAtomStats()
			: DisplayFlags(0)
			, DisplaySort(EDisplaySort::Name)
			, EnabledStats()
		{
		}
	};

	void UpdateDisplaySort(FAtomStats& InAtomStats)
	{
		if (AtomDebugSoundSortCVarCVar == TEXT("distance"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Distance;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("class"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Class;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("name"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Name;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("time"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::PlaybackTime;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("priority"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Priority;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("volume"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Volume;
		}
		else if (AtomDebugSoundSortCVarCVar == TEXT("playbacks"))
		{
			InAtomStats.DisplaySort = FAtomStats::EDisplaySort::Playbacks;
		}
	}

	struct FAtomStats_AtomThread
	{
		TSet<FName> RequestedStats;

		FAtomStats_AtomThread()
			: RequestedStats()
		{
		}
	};

	TMap<FAtomRuntimeId, FAtomStats> AtomRuntimeStats;
	TMap<FAtomRuntimeId, FAtomStats_AtomThread> AtomRuntimeStats_AtomThread;

	void HandleDumpActiveSounds(UWorld* World)
	{
		if (GCriWare && GCriWare->GetAtomRuntimeManager())
		{
			GCriWare->GetAtomRuntimeManager()->GetDebugger().DumpActiveSounds();
		}
	}

	void HandleClearMutesAndSolos(UWorld* World)
	{
		if (GCriWare && GCriWare->GetAtomRuntimeManager())
		{
			GCriWare->GetAtomRuntimeManager()->GetDebugger().ClearMutesAndSolos();
		}
	}

	template <typename SoundType>
	void PlayDebugSound(const TCHAR* Cmd, FAtomRuntime& InAtomRuntime, UWorld& InWorld, TUniqueFunction<void(SoundType& /*InSound*/)> InInitFunction)
	{
		UAtomComponent* TestComp = InAtomRuntime.GetTestComponent(&InWorld);
		if (!TestComp)
		{
			return;
		}

		bool bAssetPathSet = false;
		FString AssetPath;
		if (FParse::Value(Cmd, TEXT("Name"), AssetPath))
		{
			if (const UCriWareCoreSettings* AtomSettings = GetDefault<UCriWareCoreSettings>())
			{
				const FName SoundName(*AssetPath);
				for (const FAtomSoundDebugEntry& DebugSound : AtomSettings->DebugSounds)
				{
					const FSoftObjectPath& ObjectPath = DebugSound.Sound;
					if (DebugSound.DebugName == SoundName && ObjectPath.IsValid())
					{
						AssetPath = ObjectPath.ToString();
						bAssetPathSet = true;
						break;
					}
				}
			}
		}

		if (!bAssetPathSet)
		{
			bAssetPathSet = FParse::Value(Cmd, TEXT("Path"), AssetPath);
		}

		if (!bAssetPathSet)
		{
			TArray<FString> Tokens;
			FString CmdStr = Cmd;
			CmdStr.ParseIntoArray(Tokens, TEXT(" "));
			if (!Tokens.Num())
			{
				return;
			}

			AssetPath = Tokens[0];
		}

		// Load up an arbitrary cue
		SoundType* SoundObject = LoadObject<SoundType>(nullptr, *AssetPath, nullptr, LOAD_None, nullptr);
		if (!SoundObject)
		{
			return;
		}

		TestComp->Stop();
		TestComp->Sound = SoundObject;
		TestComp->bAutoDestroy = true;

		float Radius = 1.0f;
		float Azimuth = 0.0f;
		float Elevation = 0.0f;
		TestComp->bAllowSpatialization |= FParse::Value(Cmd, TEXT("Radius"), Radius);
		TestComp->bAllowSpatialization |= FParse::Value(Cmd, TEXT("Azimuth"), Azimuth);
		TestComp->bAllowSpatialization |= FParse::Value(Cmd, TEXT("Elevation"), Elevation);
		if (TestComp->bAllowSpatialization)
		{
			TestComp->bAllowSpatialization = true;
			FTransform TestTransform;
			InAtomRuntime.GetListenerTransform(0, TestTransform);

			static const float AziOffset = 90.0f;
			static const float ElevOffset = 90.0f;

			const FVector EulerAngles = TestTransform.GetRotation().Euler();
			Azimuth = FMath::DegreesToRadians(-1.0f * (EulerAngles.Z + Azimuth + AziOffset));
			Elevation = FMath::DegreesToRadians(Elevation + EulerAngles.Y - ElevOffset);

			const float X = Radius * FMath::Sin(Elevation) * FMath::Sin(Azimuth);
			const float Y = Radius * FMath::Sin(Elevation) * FMath::Cos(Azimuth);
			const float Z = Radius * FMath::Cos(Elevation);

			const FVector Translation(X, Y, Z);
			TestTransform.AddToTranslation(Translation);
			TestComp->SetComponentToWorld(TestTransform);
		}

		InInitFunction(*SoundObject);
		TestComp->Play();
	}

	void HandlePlayDebugSoundCue(const TArray<FString>& InArgs, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		auto PlayDebugSoundCue = [](const TCHAR* InCmd, FAtomRuntime& InAtomRuntime, UWorld& InWorld)
		{
			bool bSetLooping = false;
			PlayDebugSound<UAtomSoundCue>(InCmd, InAtomRuntime, InWorld, [InCmd](UAtomSoundCue& InCue)
			{
				/*TArray<USoundNodeWavePlayer*> WavePlayers;
				InCue.RecursiveFindNode<USoundNodeWavePlayer>(InCue.FirstNode, WavePlayers);

				for (int32 i = 0; i < WavePlayers.Num(); ++i)
				{
					if (USoundWave* SoundWave = WavePlayers[i]->GetSoundWave())
					{
						//FAtomDebugger::LogSubtitle(InCmd, *SoundWave);
					}
				}*/
			});
		};

		FString Cmd = FString::Join(InArgs, TEXT(" "));

		if (FParse::Param(*Cmd, TEXT("AllViews")))
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				RuntimeManager->IterateOverAllRuntimes([PlayDebugSoundCue, Cmd, World](FAtomRuntimeId, FAtomRuntime* AtomRuntime)
				{
					if (AtomRuntime)
					{
						PlayDebugSoundCue(*Cmd, *AtomRuntime, *World);
					}
				});
			}
		}
		else
		{
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World))
			{
				PlayDebugSoundCue(*Cmd, *AtomRuntime, *World);
			}
		}
	}

	void HandlePlayDebugSoundWave(const TArray<FString>& InArgs, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		FString Cmd = FString::Join(InArgs, TEXT(" "));

		auto PlayDebugSoundWave = [](const TCHAR* InCmd, FAtomRuntime& InAtomRuntime, UWorld& InWorld)
		{
			PlayDebugSound<UAtomSoundWave>(InCmd, InAtomRuntime, InWorld, [InCmd](UAtomSoundWave& InSoundWave)
			{
				//FAtomDebugger::LogSubtitle(InCmd, InSoundWave);
			});
		};

		if (FParse::Param(*Cmd, TEXT("AllViews")))
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				RuntimeManager->IterateOverAllRuntimes([PlayDebugSoundWave, Cmd, World](FAtomRuntimeId, FAtomRuntime* AtomRuntime)
				{
					if (AtomRuntime)
					{
						PlayDebugSoundWave(*Cmd, *AtomRuntime, *World);
					}
				});
			}
		}
		else if (World)
		{
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World))
			{
				PlayDebugSoundWave(*Cmd, *AtomRuntime, *World);
			}
		}
	}

	void HandleStopDebugSound(const TArray<FString>& InArgs, UWorld* InWorld)
	{
		if (!InWorld)
		{
			return;
		}

		FString Cmd = FString::Join(InArgs, TEXT(" "));

		if (FParse::Param(*Cmd, TEXT("AllViews")))
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				RuntimeManager->IterateOverAllRuntimes([InWorld](FAtomRuntimeId, FAtomRuntime* AtomRuntime)
					{
						if (AtomRuntime)
						{
							AtomRuntime->StopTestComponent();
						}
					});
			}
		}
		else if (InWorld)
		{
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(InWorld))
			{
				AtomRuntime->StopTestComponent();
			}
		}
	}

	void DebugSoundObject(const TArray<FString>& Args, UWorld* InWorld, const FName InStatToEnable, bool& bAllEnabled)
	{
		if (Args.Num() > 0)
		{
			bool bAllViews = false;
			for (int32 i = 1; i < Args.Num(); ++i)
			{
				bAllViews |= Args[i] == TEXT("-AllViews");
			}

			bool bEnablementRequest = Args[0].ToBool();
			bAllEnabled = bAllViews ? bEnablementRequest : false;

			TSet<FName> Stats;
			Stats.Add(InStatToEnable);

			bEnablementRequest
				? FAtomDebugger::SetStats(Stats, bAllViews ? nullptr : InWorld)
				: FAtomDebugger::ClearStat(InStatToEnable, bAllViews ? nullptr : InWorld);
		}
	}

	bool DebugShouldRenderStat(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, bool bEnablementBool, const FName InAudioStat, FAtomRuntime** OutAtomRuntime)
	{
		if ((ViewportSubsystem && !IsValid(ViewportSubsystem)) || !World)
		{
			return false;
		}

		*OutAtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);
		if (!*OutAtomRuntime)
		{
			return false;
		}

		if (bEnablementBool)
		{
			return true;
		}

		if (!bEnablementBool)
		{
			FAtomStats* AtomStats = AtomRuntimeStats.Find((*OutAtomRuntime)->GetAtomRuntimeID());
			if (!AtomStats || !(AtomStats->EnabledStats.Contains(InAudioStat)))
			{
				return false;
			}
		}

		return true;
	}

	int32 DebugDrawString(UViewportStatsSubsystem* ViewportSubsystem, FString String, const FLinearColor& Color, float OffsetX, float OffsetY)
	{
		check(ViewportSubsystem);
		ViewportSubsystem->AddTimedDisplay(FText::FromString(String), Color, 0.0f, FVector2D(OffsetX, OffsetY));
		return 20; // The fixed font height in Viewport Stats Subsystem.
	}
} // namespace Atom

// Console Commands
static FAutoConsoleCommandWithWorld GAtomDumpActiveSounds(TEXT("atom.DumpActiveSounds"), TEXT("Outputs data about all the currently active sounds."), FConsoleCommandWithWorldDelegate::CreateStatic(&Atom::HandleDumpActiveSounds), ECVF_Cheat);
static FAutoConsoleCommandWithWorld GAtomClearMutesAndSolos(TEXT("atom.ClearMutesAndSolos"), TEXT("Clears any solo-ing/mute-ing sounds"), FConsoleCommandWithWorldDelegate::CreateStatic(&Atom::HandleClearMutesAndSolos), ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs GAtomPlayDebugSoundCue
(
	TEXT("atom.Debug.PlaySoundCue"),
	TEXT("Plays a SoundCue:\n")
	TEXT("-Name <SoundName>: If a debug sound with the short name is specified in AudioSettings, plays that sound.\n")
	TEXT("-Path <ObjectPath>: Finds SoundCue asset at the provided path and if found, plays that sound.\n")
	TEXT("-Radius <Distance>: If set, enables sound spatialization and sets radial distance between listener and source emitting sound.\n")
	TEXT("-Azimuth <Angle>: If set, enables sound spatialization and sets azimuth angle between listener and source emitting sound (in degrees, where 0 is straight ahead, negative to left, positive to right).\n")
	TEXT("-Elevation <Angle>: If set, enables sound spatialization and sets azimuth angle between listener and source emitting sound (in degrees, where 0 is straight ahead, negative to left, positive to right).\n")
	TEXT("-AllViews: If option provided, plays sound through all viewports.\n")
	TEXT("-LogSubtitles: If option provided, logs sounds subtitle if set\n"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Atom::HandlePlayDebugSoundCue),
	ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomPlayDebugSoundWave
(
	TEXT("atom.Debug.PlaySoundWave"),
	TEXT("Plays a SoundWave:\n")
	TEXT("-Name <SoundName>: If a debug sound with the short name is specified in AudioSettings, plays that sound.\n")
	TEXT("-Path <ObjectPath>: Finds SoundWave asset at the provided path and if found, plays that sound.\n")
	TEXT("-Radius: If set, enables sound spatialization and sets radial distance between listener and source emitting sound.\n")
	TEXT("-Azimuth <Angle>: If set, enables sound spatialization and sets azimuth angle between listener and source emitting sound (in degrees, where 0 is straight ahead, negative to left, positive to right).\n")
	TEXT("-Elevation <Angle>: If set, enables sound spatialization and sets azimuth angle between listener and source emitting sound (in degrees, where 0 is straight ahead, negative to left, positive to right).\n")
	TEXT("-AllViews: If option provided, plays sound through all viewports.\n")
	TEXT("-LogSubtitles: If option provided, logs sounds subtitle if set\n"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Atom::HandlePlayDebugSoundWave),
	ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomStopDebugSound
(
	TEXT("atom.Debug.StopSound"),
	TEXT("Stops debug sound.\n")
	TEXT("-AllViews: If option provided, stops all debug sounds in all viewports.\n"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Atom::HandleStopDebugSound),
	ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundPlaybacks
(
	TEXT("atom.Debug.SoundPlaybacks"),
	TEXT("Post Sound Playbacks information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundPlaybacks, Atom::bDebugPlaybacksForAllViewsEnabled);
	}), ECVF_Cheat
);


static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundCues
(
	TEXT("atom.Debug.SoundCues"),
	TEXT("Post SoundCue information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundCues, Atom::bDebugCuesForAllViewsEnabled);
	}), ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSounds
(
	TEXT("atom.Debug.Sounds"),
	TEXT("Post Sound information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::Sounds, Atom::bDebugSoundsForAllViewsEnabled);
	}), ECVF_Cheat
);

static bool bAtomAttenuationVisualizeEnabledCVar = false;
FAutoConsoleCommandWithWorldAndArgs CVarAtomVisualizeAttenuation(
	TEXT("atom.3dVisualize.Attenuation"),
	TEXT("Whether or not attenuation spheres are visible when 3d visualize is enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		if (Args.Num() <= 0)
		{
			return;
		}

		bAtomAttenuationVisualizeEnabledCVar = Args[0].ToBool();

		// Internally, the visualization code requires atom.Debug.Sounds to be enabled,
		// so we force it on whenever this one is enabled.
		if (bAtomAttenuationVisualizeEnabledCVar)
		{
			Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::Sounds, Atom::bDebugSoundsForAllViewsEnabled);
		}
	}),
	ECVF_Default);

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundWaves
(
	TEXT("atom.Debug.SoundWaves"),
	TEXT("Post SoundWave information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundWaves, Atom::bDebugWavesForAllViewsEnabled);
	}), ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundRacks
(
	TEXT("atom.Debug.SoundRacks"),
	TEXT("Post SoundMix information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundRacks, Atom::bDebugRacksForAllViewsEnabled);
	}), ECVF_Cheat
);

/*static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundReverb
(
	TEXT("atom.Debug.SoundReverb"),
	TEXT("Post SoundReverb information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
		{
			Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundReverb, Atom::bDebugReverbForAllViewsEnabled);
		}), ECVF_Cheat
);*/

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugSoundModulation
(
	TEXT("atom.Debug.SoundModulators"),
	TEXT("Post SoundModulation information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::SoundModulation, Atom::bDebugModulationForAllViewsEnabled);
	}), ECVF_Cheat
);

static FAutoConsoleCommandWithWorldAndArgs GAtomDebugStreaming
(
	TEXT("atom.Debug.Streaming"),
	TEXT("Post Stream Caching information to viewport(s).\n")
	TEXT("0: Disable, 1: Enable\n")
	TEXT("(Optional) -AllViews: Enables/Disables for all viewports, not just those associated with the current world"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
		{
			Atom::DebugSoundObject(Args, InWorld, Atom::DebugStatNames::AudioStreaming, Atom::bDebugStreamingForAllViewsEnabled);
		}), ECVF_Cheat
);

namespace Atom
{
	/*
	 * Atom Debugger Implementation
	 *****************************************************************************/

	FAtomDebugger::FAtomDebugger()
		: bVisualize3dDebug(0)
	{
		WorldRegisteredWithRuntimeHandle = FAtomRuntimeWorldDelegates::OnWorldRegisteredToAtomRuntime.AddLambda([this](const UWorld* InWorld, FAtomRuntimeId InRuntimeID)
		{
			TSet<FName> StatsToSet;

			if (bDebugSoundsForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::Sounds);
			}

			if (bDebugCuesForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::SoundCues);
			}

			if (bDebugWavesForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::SoundWaves);
			}

			if (bDebugPlaybacksForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::SoundPlaybacks);
			}

			if (bDebugModulationForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::SoundModulation);
			}

			if (bDebugRacksForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::SoundRacks);
			}

			//if (bDebugReverbForAllViewsEnabled)
			//{
			//	StatsToSet.Add(DebugStatNames::SoundReverb);
			//}

			if (bDebugStreamingForAllViewsEnabled)
			{
				StatsToSet.Add(DebugStatNames::AudioStreaming);
			}

			SetStats(InRuntimeID, StatsToSet);
		});
	}

	bool FAtomDebugger::IsVisualizeDebug3dEnabled() const
	{
		return bVisualize3dDebug;
	}

	void FAtomDebugger::ToggleVisualizeDebug3dEnabled()
	{
		bVisualize3dDebug = !bVisualize3dDebug;
	}

	bool FAtomDebugger::IsVirtualLoopVisualizeEnabled()
	{
		return static_cast<bool>(AtomVirtualLoopsVisualizeEnabledCVar);
	}

#if WITH_EDITOR
	void FAtomDebugger::OnBeginPIE()
	{
		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager)
		{
			RuntimeManager->GetDebugger().ClearMutesAndSolos();
		}
	}

	void FAtomDebugger::OnEndPIE()
	{
		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager)
		{
			RuntimeManager->GetDebugger().ClearMutesAndSolos();
		}
	}
#endif // WITH_EDITOR

	void FAtomDebugger::QuerySoloMuteSoundClass(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const
	{
		GetDebugSoloMuteStateX(Name, DebugNames.SoloSoundClass, DebugNames.MuteSoundClass, bOutIsSoloed, bOutIsMuted, OutReason);
	}

	void FAtomDebugger::QuerySoloMuteSoundWave(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const
	{
		GetDebugSoloMuteStateX(Name, DebugNames.SoloSoundWave, DebugNames.MuteSoundWave, bOutIsSoloed, bOutIsMuted, OutReason);
	}

	void FAtomDebugger::QuerySoloMuteSoundCue(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const
	{
		GetDebugSoloMuteStateX(Name, DebugNames.SoloSoundCue, DebugNames.MuteSoundCue, bOutIsSoloed, bOutIsMuted, OutReason);
	}

	void FAtomDebugger::SetNameArray(FName InName, TArray<FName>& InNameArray, bool bOnOff)
	{
		ExecuteCmdOnAtomThread([InName, &InNameArray, bOnOff]
		{
			if (bOnOff)
			{
				InNameArray.AddUnique(InName);
			}
			else
			{
				InNameArray.Remove(InName);
			}
		});
	}

	void FAtomDebugger::ToggleNameArray(FName InName, TArray<FName>& InNameArray, bool bExclusive)
	{
		ExecuteCmdOnAtomThread([InName, &InNameArray, bExclusive]
		{
			// On already?
			int32 IndexOf = InNameArray.IndexOfByKey(InName);
			if (IndexOf != INDEX_NONE)
			{
				if (bExclusive)
				{
					// Turn off everything if we are exclusive.
					InNameArray.Empty();
				}
				else
				{
					InNameArray.RemoveAtSwap(IndexOf);
				}
			}
			else // Add it.
			{
				// If we are exclusive, turn off everything else first.
				if (bExclusive)
				{
					InNameArray.Empty();
				}

				// Allow for NAME_None to act as a clear
				if (InName != NAME_None)
				{
					InNameArray.Add(InName);
				}
			}
		});
	}

	void FAtomDebugger::SetAtomMixerDebugSound(const TCHAR* SoundName)
	{
		ExecuteCmdOnAtomThread([Name = FString(SoundName), this]
		{
			DebugNames.DebugAtomMixerSoundName = Name;
		});
	}

	void FAtomDebugger::SetAtomDebugSound(const TCHAR* SoundName)
	{
		ExecuteCmdOnAtomThread([Name = FString(SoundName), this]
		{
			DebugNames.DebugSoundName = Name;
			DebugNames.bDebugSoundName = DebugNames.DebugSoundName != TEXT("");
		});
	}

	const FString& FAtomDebugger::GetAtomMixerDebugSoundName() const
	{
		check(IsInAtomThread());
		return DebugNames.DebugAtomMixerSoundName;
	}

	bool FAtomDebugger::GetAtomDebugSound(FString& OutDebugSound)
	{
		check(IsInAtomThread());
		if (DebugNames.bDebugSoundName)
		{
			OutDebugSound = DebugNames.DebugSoundName;
			return true;
		}
		return false;
	}

	void FAtomDebugger::ExecuteCmdOnAtomThread(TFunction<void()> Cmd)
	{
		// If not on Atom thread, queue it.
		if (!IsInAtomThread())
		{
			FAtomThread::RunCommandOnAtomThread(Cmd);
			return;
		}

		// Otherwise, do it inline.
		Cmd();
	}

	void FAtomDebugger::GetDebugSoloMuteStateX(
		const FString& Name,
		const TArray<FName>& Solos,
		const TArray<FName>& Mutes,
		bool& bOutIsSoloed,
		bool& bOutIsMuted,
		FString& OutReason) const
	{
		check(IsInAtomThread());

		// Allow for partial matches of the name.
		auto MatchesPartOfName = [&Name](FName InName) -> bool { return Name.Contains(InName.GetPlainNameString()); };

		// Solo active?
		if (Solos.Num() > 0)
		{
			if (Solos.ContainsByPredicate(MatchesPartOfName))
			{
				bOutIsSoloed = true;
				OutReason = FString::Printf(TEXT("Sound is soloed explicitly."));
			}
			else
			{
				// Something else is soloed (record the first item in the solo list for debug reason).
				bOutIsMuted = true;
				OutReason = FString::Printf(TEXT("Sound is muted due to [%s] being soloed"), *Solos[0].ToString());
			}
		}
		// Are we explicitly muted?
		else if (Mutes.ContainsByPredicate(MatchesPartOfName))
		{
			bOutIsMuted = true;
			OutReason = FString::Printf(TEXT("Sound is explicitly muted"));
		}
	}

	void FAtomDebugger::DrawDebugInfo(const FAtomSource& SoundSource)
	{
#if ENABLE_DRAW_DEBUG
		const FAtomPlaybackInstance* PlaybackInstance = SoundSource.GetPlaybackInstance();
		if (!PlaybackInstance)
		{
			return;
		}

		const FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		if (!ActiveSound)
		{
			return;
		}

		if (!AtomSpatialSourceVisualizeEnabledCVar)
		{
			return;
		}

		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager && RuntimeManager->IsVisualizeDebug3dEnabled())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.DrawSourceDebugInfo"), STAT_AtomDrawSourceDebugInfo, STATGROUP_TaskGraphTasks);

			//const FSoundBuffer* Buffer = SoundSource.GetBuffer();
			//const bool bSpatialized = Buffer && Buffer->NumChannels == 2 && WaveInstance->GetUseSpatialization();
			const bool bSpatialized = SoundSource.GetWaveInfo().NumChannels == 2 && PlaybackInstance->GetUseSpatialization();
			if (bSpatialized)
			{
				const FRotator Rotator = ActiveSound->Transform.GetRotation().Rotator();

				TWeakObjectPtr<UWorld> WorldPtr = PlaybackInstance->ActiveSound->GetWeakWorld();
				FVector LeftChannelSourceLoc;
				FVector RightChannelSourceLoc;
				SoundSource.GetChannelLocations(LeftChannelSourceLoc, RightChannelSourceLoc);
				FAtomThread::RunCommandOnGameThread([LeftChannelSourceLoc, RightChannelSourceLoc, Rotator, WorldPtr]()
				{
					if (WorldPtr.IsValid())
					{
						UWorld* World = WorldPtr.Get();
						DrawDebugCrosshairs(World, LeftChannelSourceLoc, Rotator, 20.0f, FColor::Red, false, -1.0f, SDPG_Foreground);
						DrawDebugCrosshairs(World, RightChannelSourceLoc, Rotator, 20.0f, FColor::Green, false, -1.0f, SDPG_Foreground);
					}
				}, GET_STATID(STAT_AtomDrawSourceDebugInfo));
			}
		}
#endif // ENABLE_DRAW_DEBUG
	}

	void FAtomDebugger::DrawDebugInfo(const FAtomActiveSound& ActiveSound, const TArray<FAtomPlaybackInstance*>& ThisSoundsPlaybackInstances, const float DeltaTime)
	{
#if ENABLE_DRAW_DEBUG
		if (!AtomActiveSoundVisualizeModeCVar)
		{
			return;
		}

		// Only draw spatialized sounds
		const UAtomSoundBase* Sound = ActiveSound.GetSound();
		if (!Sound || !ActiveSound.bAllowSpatialization)
		{
			return;
		}

		const float PlaybackTime = ActiveSound.PlaybackTime;
		const float PlaybackTimeNonVirtualized = ActiveSound.PlaybackTimeNonVirtualized;
		const bool bOccluded = ActiveSound.bIsOccluded;

		// Sounds requiring culling can start and immediately stop repeatedly when subscribed
		// concurrency is flooded, so don't show the initial frame.
		if (FMath::IsNearlyZero(PlaybackTimeNonVirtualized))
		{
			return;
		}

		if (AtomActiveSoundVisualizeTypeCVar > 0)
		{
			if (AtomActiveSoundVisualizeTypeCVar == 1 && ActiveSound.GetAtomComponentID() == 0)
			{
				return;
			}

			if (AtomActiveSoundVisualizeTypeCVar == 2 && ActiveSound.GetAtomComponentID() > 0)
			{
				return;
			}
		}

		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager && RuntimeManager->IsVisualizeDebug3dEnabled())
		{
			// TODO: remove virtualized sound by atom to active list
			// Only draw if not virtualized by Atom
			if (FAtomRuntime* Runtime = ActiveSound.GetAtomRuntime())
			{
				if (Runtime->GetVirtualLoops().Contains(&ActiveSound))
				{
					return;
				}
			}

			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.DrawActiveSoundDebugInfo"), STAT_AtomDrawActiveSoundDebugInfo, STATGROUP_TaskGraphTasks);

			auto DrawSoundInfo = [&](const FString& Name, const FTransform& CurTransform) {
				FColor TextColor = FColor::White;
				const float CurMaxDistance = ActiveSound.MaxDistance;
				float DisplayValue = 0.0f;
				float FilterValue = 0.0f;
				if (AtomActiveSoundVisualizeModeCVar == 1 || AtomActiveSoundVisualizeModeCVar == 2)
				{
					for (FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
					{
						DisplayValue = FMath::Max(DisplayValue, PlaybackInstance->GetVolumeWithDistanceAndOcclusionAttenuation() * PlaybackInstance->GetDynamicVolume());
					}
				}
				else if (AtomActiveSoundVisualizeModeCVar == 3)
				{
					if (ActiveSound.GetAtomRuntime())
					{
						DisplayValue = ActiveSound.GetAtomRuntime()->GetDistanceToNearestListener(CurTransform.GetLocation()) / CurMaxDistance;
					}
				}
				else if (AtomActiveSoundVisualizeModeCVar == 4)
				{
					TextColor = ActiveSound.DebugColor;
				}
				else if (AtomActiveSoundVisualizeModeCVar == 5)
				{
					DisplayValue = ActiveSound.CurrentOcclusionVolumeAttenuation.GetValue();
					FilterValue = ActiveSound.CurrentOcclusionFilterFrequency.GetValue();
				}

				TWeakObjectPtr<UWorld> WorldPtr = ActiveSound.GetWeakWorld();
				FAtomThread::RunCommandOnGameThread([Name, TextColor, CurTransform, DisplayValue, WorldPtr, CurMaxDistance, PlaybackTime, PlaybackTimeNonVirtualized, bOccluded, FilterValue, DeltaTime]()
				{
					if (WorldPtr.IsValid())
					{
						static const float ColorRedHue = 0.0f;
						static const float ColorGreenHue = 85.0f;

						const FVector Location = CurTransform.GetLocation();
						UWorld* DebugWorld = WorldPtr.Get();
						DrawDebugSphere(DebugWorld, Location, 10.0f, 8, FColor::White, false, -1.0f, SDPG_Foreground);
						FColor Color = TextColor;

						FString Descriptor;
						if (AtomActiveSoundVisualizeModeCVar == 1 || AtomActiveSoundVisualizeModeCVar == 2)
						{
							const float DisplayDbVolume = Atom::ConvertToDecibels(DisplayValue);
							if (AtomActiveSoundVisualizeModeCVar == 1)
							{
								Descriptor = FString::Printf(TEXT(" (Vol: %.3f [Active: %.2fs, Playing: %.2fs])"), DisplayValue, PlaybackTime, PlaybackTimeNonVirtualized);
							}
							else
							{
								Descriptor = FString::Printf(TEXT(" (Vol: %.3f dB [Active: %.2fs, Playing: %.2fs])"), DisplayDbVolume, PlaybackTime, PlaybackTimeNonVirtualized);
							}
							static const float DbColorMinVol = -30.0f;
							const float DbVolume = FMath::Clamp(DisplayDbVolume, DbColorMinVol, 0.0f);
							const float Hue = FMath::Lerp(ColorRedHue, ColorGreenHue, (-1.0f * DbVolume / DbColorMinVol) + 1.0f);
							Color = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue), 255u, 255u).ToFColor(true);
						}
						else if (AtomActiveSoundVisualizeModeCVar == 3)
						{
							Descriptor = FString::Printf(TEXT(" (Dist: %.3f, Max: %.3f)"), DisplayValue * CurMaxDistance, CurMaxDistance);
							const float Hue = FMath::Lerp(ColorGreenHue, ColorRedHue, DisplayValue);
							Color = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Clamp(Hue, 0.0f, 255.f)), 255u, 255u).ToFColor(true);
						}
						else if (AtomActiveSoundVisualizeModeCVar == 5)
						{
							Descriptor = FString::Printf(TEXT(" (Occlusion Volume: %.3f, Occlusion Filter: %.3f)"), DisplayValue, FilterValue);
							if (bOccluded)
							{
								Color = FColor::Red;
							}
							else
							{
								Color = FColor::Green;
							}
						}

						const FString Description = FString::Printf(TEXT("%s%s"), *Name, *Descriptor);
						DrawDebugString(DebugWorld, Location + FVector(0, 0, 32), *Description, nullptr, Color, DeltaTime, false);
					}
				}, GET_STATID(STAT_AtomDrawActiveSoundDebugInfo));
			};

			const FString Name = Sound->GetName();
			const FTransform CurTransform = ActiveSound.Transform;
			if (ActiveSound.MultiPositionType == EAtomMultiPositionType::SingleSource || !AtomMultiPositionSourcesVisualizeEnabledCVar)
			{
				const FTransform& Transform = ActiveSound.MultiPositions.IsEmpty() ? ActiveSound.Transform : ActiveSound.MultiPositions[0];
				DrawSoundInfo(Name, Transform);
			}
			else
			{
				for (const FTransform& Transform : ActiveSound.MultiPositions)
				{
					DrawSoundInfo(Name, Transform);
				}
			}
		}
#endif // ENABLE_DRAW_DEBUG
	}

	void FAtomDebugger::DrawDebugInfo(UWorld& World, const TArray<FAtomListener>& Listeners)
	{
#if ENABLE_DRAW_DEBUG
		if (!AtomActiveSoundVisualizeListenersCVar)
		{
			return;
		}

		check(IsInAtomThread());

		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager && RuntimeManager->IsVisualizeDebug3dEnabled())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.DrawListenerDebugInfo"), STAT_AtomDrawListenerDebugInfo, STATGROUP_TaskGraphTasks);
			const TWeakObjectPtr<UWorld> WorldPtr = &World;
			for (const FAtomListener& Listener : Listeners)
			{
				const FVector ListenerPosition = Listener.GetPosition(true);
				const FVector ListenerFront = Listener.GetFront();
				const FVector ListenerUp = Listener.GetUp();
				const FVector ListenerRight = Listener.GetRight();

				FAtomThread::RunCommandOnGameThread([WorldPtr, ListenerPosition, ListenerFront, ListenerUp, ListenerRight]()
				{
					if (WorldPtr.IsValid())
					{
						static float ArrowLength = 30.0f;
						static float ArrowHeadSize = 8.0f;
						static float Lifetime = -1.0f;
						static float Thickness = 0.9f;
						static uint8 DepthPriority = SDPG_Foreground;
						static bool bPersistentLines = false;

						DrawDebugDirectionalArrow(WorldPtr.Get(), ListenerPosition, ListenerPosition + (ArrowLength * ListenerFront), ArrowHeadSize, FColor::Red, bPersistentLines, Lifetime, DepthPriority, Thickness);
						DrawDebugDirectionalArrow(WorldPtr.Get(), ListenerPosition, ListenerPosition + (ArrowLength * ListenerUp), ArrowHeadSize, FColor::Blue, bPersistentLines, Lifetime, DepthPriority, Thickness);
						DrawDebugDirectionalArrow(WorldPtr.Get(), ListenerPosition, ListenerPosition + (ArrowLength * ListenerRight), ArrowHeadSize, FColor::Green, bPersistentLines, Lifetime, DepthPriority, Thickness);
						DrawDebugSphere(WorldPtr.Get(), ListenerPosition, 5.0f, 8, FColor::Magenta, bPersistentLines, Lifetime, DepthPriority);
					}
				}, GET_STATID(STAT_AtomDrawListenerDebugInfo));
			}
		}
#endif // ENABLE_DRAW_DEBUG
	}

	void FAtomDebugger::DrawDebugInfo(const FAtomVirtualLoop& VirtualLoop)
	{
#if ENABLE_DRAW_DEBUG
		if (!GEngine)
		{
			return;
		}

		if (!AtomVirtualLoopsVisualizeEnabledCVar)
		{
			return;
		}

		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager && RuntimeManager->IsVisualizeDebug3dEnabled())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.DrawVirtualLoopDebugInfo"), STAT_AtomDrawVirtualLoopDebugInfo, STATGROUP_TaskGraphTasks);

			const FAtomActiveSound& ActiveSound = VirtualLoop.GetActiveSound();
			UAtomSoundBase* Sound = ActiveSound.GetSound();
			check(Sound);

			const FTransform Transform = ActiveSound.Transform;
			const TWeakObjectPtr<UWorld> World = ActiveSound.GetWeakWorld();
			const FString Name = Sound->GetName();
			const float DrawInterval = VirtualLoop.GetUpdateInterval();
			const float TimeVirtualized = VirtualLoop.GetTimeVirtualized();
			FAtomThread::RunCommandOnGameThread([World, Transform, Name, DrawInterval, TimeVirtualized]()
			{
				if (World.IsValid())
				{
					const FString Description = FString::Printf(TEXT("%s [Virt: %.2fs]"), *Name, TimeVirtualized);
					FVector Location = Transform.GetLocation();
					FRotator Rotation = Transform.GetRotation().Rotator();
					DrawDebugCrosshairs(World.Get(), Location, Rotation, 20.0f, FColor::Blue, false, DrawInterval, SDPG_Foreground);
					DrawDebugString(World.Get(), Location + FVector(0, 0, 32), *Description, nullptr, FColor::Blue, DrawInterval, false);
				}
			}, GET_STATID(STAT_AtomDrawVirtualLoopDebugInfo));
		}
#endif // ENABLE_DRAW_DEBUG
	}

	static UWorld* GetWorldForDebugStats()
	{
		UWorld* World = nullptr;

#if WITH_EDITOR
		UEditorEngine* EEngine = Cast<UEditorEngine>(GEngine);
		if (GIsEditor && EEngine != nullptr)
		{
			// lets use PlayWorld during PIE/Simulate and regular world from editor otherwise, to draw debug information
			World = EEngine->PlayWorld != nullptr ? ToRawPtr(EEngine->PlayWorld) : EEngine->GetEditorWorldContext().World();
		}
#endif

		if (!GIsEditor && World == nullptr && GEngine != nullptr)
		{
			World = GEngine->GetWorld();
		}

		return World;
	}

	void FAtomDebugger::DrawDebugStatsToViewportStats()
	{
		if (UWorld* World = GetWorldForDebugStats())
		{
			if (UViewportStatsSubsystem* ViewportSubsystem = World->GetSubsystem<UViewportStatsSubsystem>())
			{
				DrawDebugStats(*World, ViewportSubsystem, 0.0f);
			}
		}
	}

	int32 FAtomDebugger::DrawDebugStats(UWorld& World, UViewportStatsSubsystem* ViewportSubsystem, int32 InY)
	{
		check(IsInGameThread());

		if (ViewportSubsystem)
		{
			return DrawDebugStatsInternal(World, *ViewportSubsystem, InY);
		}

		return InY;
	}

	/*int32 FAtomDebugger::DrawDebugStats(UWorld& World, FViewport* OutViewport, int32 InY)
	{
		check(IsInGameThread());

		if (OutViewport)
		{
			return DrawDebugStatsInternal(World, *OutViewport, InCanvas, InY);
		}

		if (UGameViewportClient* GameViewport = World.GetGameViewport())
		{
			if (GameViewport->Viewport)
			{
				return DrawDebugStatsInternal(World, *GameViewport->Viewport, InCanvas, InY);
			}

			return InY;
		}

#if WITH_EDITOR
		if (GEditor)
		{
			// Level editor viewport may not be a game viewport, but should still display debug stats, so check if this case is covered and display accordingly
			const TArray<FLevelEditorViewportClient*>& LevelViewportClients = GEditor->GetLevelViewportClients();
			for (FLevelEditorViewportClient* ViewportClient : LevelViewportClients)
			{
				if (ViewportClient && ViewportClient->Viewport)
				{
					UWorld* EditorWorld = ViewportClient->GetWorld();
					if (EditorWorld && EditorWorld == &World)
					{
						const FText SystemDisplayName = NSLOCTEXT("AtomDebugger", "AtomDebugger_DrawDebugData", "Atom Debug Data");
						if (DrawDebugStatsEnabled())
						{
							ViewportClient->AddRealtimeOverride(true, SystemDisplayName);
						}

						ViewportClient->RemoveRealtimeOverride(SystemDisplayName);

						return DrawDebugStatsInternal(World, *ViewportClient->Viewport, InCanvas, InY);
					}
				}
			}
		}
#endif // WITH_EDITOR

		return InY;
	}*/

	bool FAtomDebugger::DrawDebugStatsEnabled()
	{
		return bDebugWavesForAllViewsEnabled
			|| bDebugCuesForAllViewsEnabled
			|| bDebugSoundsForAllViewsEnabled
			|| bDebugRacksForAllViewsEnabled
			//|| bDebugReverbForAllViewsEnabled
			|| bDebugModulationForAllViewsEnabled;
	}

	int32 FAtomDebugger::DrawDebugStatsInternal(UWorld& World, UViewportStatsSubsystem& ViewportSubsystem, int32 InY)
	{
		// access extension to messages
		class ViewportStatsSubsystem_ResetMessagesExt : private UViewportStatsSubsystem
		{
		public: void ResetDisplayMessages() { UniqueDisplayMessages.Reset(); }
		};

		// clear all previous messages in the subsystem.
		reinterpret_cast<ViewportStatsSubsystem_ResetMessagesExt*>(&ViewportSubsystem)->ResetDisplayMessages();

		int32 X = AtomSoundDebugDisplayCornerXCVar;
		int32 Y = AtomSoundDebugDisplayCornerYCVar < 0 ? InY : AtomSoundDebugDisplayCornerYCVar;

		Y = RenderStatRacks(&World, &ViewportSubsystem, X, Y);
		Y = RenderStatModulators(&World, &ViewportSubsystem, X, Y, nullptr, nullptr);
		//Y = RenderStatReverb(&World, &ViewportSubsystem, X, Y);
		Y = RenderStatSounds(&World, &ViewportSubsystem, X, Y);
		Y = RenderStatCues(&World, &ViewportSubsystem, X, Y);
		Y = RenderStatWaves(&World, &ViewportSubsystem, X, Y);
		Y = RenderStatPlaybacks(&World, &ViewportSubsystem, X, Y);

		return Y;
	}

	void FAtomDebugger::DumpActiveSounds() const
	{
		if (!GCriWare)
		{
			return;
		}

		if (!IsInAtomThread())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.DumpActiveSounds"), STAT_AtomDumpActiveSounds, STATGROUP_TaskGraphTasks);
			FAtomThread::RunCommandOnAtomThread([this]()
			{
				DumpActiveSounds();
			}, GET_STATID(STAT_AtomDumpActiveSounds));
			return;
		}

		FAtomRuntimeHandle AtomRuntime = GCriWare->GetAtomRuntimeManager()->GetActiveAtomRuntime();
		if (!AtomRuntime)
		{
			return;
		}

		const TArray<FAtomActiveSound*>& ActiveSounds = AtomRuntime->GetActiveSounds();
		UE_LOG(LogCriWareAtom, Display, TEXT("Active Sound Count: %d"), ActiveSounds.Num());
		UE_LOG(LogCriWareAtom, Display, TEXT("------------------------"), ActiveSounds.Num());

		for (const FAtomActiveSound* ActiveSound : ActiveSounds)
		{
			if (ActiveSound)
			{
				UE_LOG(LogCriWareAtom, Display, TEXT("%s (%.3g) - %s"), *ActiveSound->GetSound()->GetName(), ActiveSound->GetSound()->GetDuration(), *ActiveSound->GetAtomComponentName());

				for (const TPair<UPTRINT, FAtomPlaybackInstance*>& PlaybackInstancePair : ActiveSound->GetPlaybackInstances())
				{
					const FAtomPlaybackInstance* PlaybackInstance = PlaybackInstancePair.Value;
					UE_LOG(LogCriWareAtom, Display, TEXT("   %s (%.3g) (%d) - %.3g"),
						*PlaybackInstance->GetName(), PlaybackInstance->SoundData->GetDuration(),
						PlaybackInstance->SoundData->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal),
						PlaybackInstance->GetVolumeWithDistanceAndOcclusionAttenuation() * PlaybackInstance->GetDynamicVolume()); // use debug distance factor from ataom player ex (debug build)
				}
			}
		}
	}

	void FAtomDebugger::ResolveDesiredStats(FViewportClient* ViewportClient)
	{
		if (!ViewportClient)
		{
			return;
		}

		FAtomRuntime* AtomRuntime = GetWorldRuntime(ViewportClient->GetWorld());
		if (!AtomRuntime)
		{
			return;
		}

		TSet<FName> SetStatFlags;

		if (ViewportClient->IsStatEnabled(TEXT("SoundCues")))
		{
			SetStatFlags.Add(DebugStatNames::SoundCues);
		}

		if (ViewportClient->IsStatEnabled(TEXT("SoundWaves")))
		{
			SetStatFlags.Add(DebugStatNames::SoundWaves);
		}

		if (ViewportClient->IsStatEnabled(TEXT("SoundPlaybacks")))
		{
			SetStatFlags.Add(DebugStatNames::SoundPlaybacks);
		}

		if (ViewportClient->IsStatEnabled(TEXT("SoundRacks")))
		{
			SetStatFlags.Add(DebugStatNames::SoundRacks);
		}

		if (ViewportClient->IsStatEnabled(TEXT("Sounds")))
		{
			FAtomStats& Stats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());
			SetStatFlags.Add(DebugStatNames::Sounds);

			if (Stats.DisplayFlags & static_cast<uint8>(FAtomStats::EDisplayFlags::Debug))
			{
				SetStatFlags.Add(DebugStatNames::DebugSounds);
			}

			if (Stats.DisplayFlags & static_cast<uint8>(FAtomStats::EDisplayFlags::Long_Names))
			{
				SetStatFlags.Add(DebugStatNames::LongSoundNames);
			}
		}

		if (ViewportClient->IsStatEnabled(TEXT("Streaming")))
		{
			SetStatFlags.Add(DebugStatNames::AudioStreaming);
		}

		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.ResolveDesiredStats"), STAT_AtomResolveDesiredStats, STATGROUP_TaskGraphTasks);

		const uint32 RuntimeID = AtomRuntime->GetAtomRuntimeID();
		if (IsInAtomThread())
		{
			FAtomStats_AtomThread& Stats = AtomRuntimeStats_AtomThread.FindOrAdd(RuntimeID);
			Stats.RequestedStats = SetStatFlags;
		}
		else
		{
			FAtomThread::RunCommandOnAtomThread([SetStatFlags, RuntimeID]()
			{
				FAtomStats_AtomThread& Stats = AtomRuntimeStats_AtomThread.FindOrAdd(RuntimeID);
				Stats.RequestedStats = SetStatFlags;
			}, GET_STATID(STAT_AtomResolveDesiredStats));
		}
	}

	int32 FAtomDebugger::RenderStatRacks(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugRacksForAllViewsEnabled, DebugStatNames::SoundRacks, &AtomRuntime))
		{
			return Y;
		}

		const int32 FontHeight = GetStatsFont()->GetMaxCharHeight() + 2;
		FAtomStats& AtomStats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());

		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Sound Racks:"), HeaderColor, X, Y);

		bool bDisplayedSoundRacks = false;

		if (AtomStats.StatSoundRackInfos.Num() > 0)
		{
			bDisplayedSoundRacks = true;

			for (const FAtomStats::FStatSoundRackInfo& StatSoundRack : AtomStats.StatSoundRackInfos)
			{
				const FString RackString = FString::Printf(TEXT("%s - DSP Setting: %s - Snapshot: %s - Total Ref Count: %i"), *StatSoundRack.RackName, *StatSoundRack.DspBusSettingsName, *StatSoundRack.SnapshotName, StatSoundRack.RefCount);

				//const FColor& RackTextColor = (StatSoundRack.bIsCurrentEQ ? FColor::Yellow : GetBodyColor());
				const FColor& RackTextColor = GetBodyColor();

				Atom::DebugDrawString(ViewportSubsystem, RackString, RackTextColor, X + DebuggerTabWidth, Y);

				for (const FAtomStats::FStatSoundBusInfo& StatSoundBus : StatSoundRack.BusInfos)
				{
					const FString BusString = FString::Printf(TEXT("%s - Volume: %1.2f - Fade Proportion: %1.2f - Total Ref Count: %i"), *StatSoundBus.BusName, StatSoundBus.Volume, StatSoundBus.InterpValue, StatSoundBus.RefCount);

					//const FColor& BusTextColor = (StatSoundBus.bIsCurrentEQ ? FColor::Yellow : GetBodyColor());
					const FColor& BusTextColor = GetBodyColor();

					Atom::DebugDrawString(ViewportSubsystem, BusString, BusTextColor, X + DebuggerTabWidth, Y);
				}
			}
		}

		if (!bDisplayedSoundRacks)
		{
			Atom::DebugDrawString(ViewportSubsystem, TEXT("None"), GetBodyColor(), X + DebuggerTabWidth, Y);
		}

		Y += FontHeight;
		return Y;
	}

	int32 FAtomDebugger::RenderStatModulators(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugModulationForAllViewsEnabled, DebugStatNames::SoundModulation, &AtomRuntime))
		{
			return Y;
		}

		const int32 FontHeight = GetStatsFont()->GetMaxCharHeight() + 2;
		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Sound Modulation:"), HeaderColor, X, Y);

		bool bDisplayedSoundModulationInfo = false;
		if (AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem())
		{
			const int32 YInit = Y;
			Y = Modulation->OnRenderStat(ViewportSubsystem, X, Y);
			bDisplayedSoundModulationInfo = Y != YInit;
		}

		if (!bDisplayedSoundModulationInfo)
		{
			Atom::DebugDrawString(ViewportSubsystem, TEXT("None"), GetBodyColor(), X, Y);
		}

		Y += FontHeight;
		return Y;
	}

	/*int32 FAtomDebugger::RenderStatReverb(UWorld* World, UViewportStatsSubsystem& ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, Canvas, bDebugReverbForAllViewsEnabled, FAtomStats::SoundReverb, &AtomRuntime))
		{
			return Y;
		}

		const int32 Height = static_cast<int32>(GetStatsFont()->GetMaxCharHeight() + 2);

		FString TheString;
		const FLinearColor LinearBodyColor = FLinearColor(GetBodyColor());
		if (UReverbEffect* ReverbEffect = AtomRuntime->GetCurrentReverbEffect())
		{
			TheString = FString::Printf(TEXT("Active Reverb Effect: %s"), *ReverbEffect->GetName());
			Canvas->DrawShadowedString(X, Y, *TheString, GetStatsFont(), HeaderColor);
			Y += Height;

			AAudioVolume* CurrentAudioVolume = nullptr;
			const int32 ProxyCount = AtomRuntime->ListenerProxies.Num();
			for (int i = 0; i < ProxyCount; ++i)
			{
				const FTransform& Transform = AtomRuntime->ListenerProxies[i].Transform;
				AAudioVolume* PlayerAudioVolume = World->GetAudioSettings(Transform.GetLocation(), nullptr, nullptr);
				if (PlayerAudioVolume && ((CurrentAudioVolume == nullptr) || (PlayerAudioVolume->GetPriority() > CurrentAudioVolume->GetPriority())))
				{
					CurrentAudioVolume = PlayerAudioVolume;
				}
			}
			if (CurrentAudioVolume && CurrentAudioVolume->GetReverbSettings().ReverbEffect)
			{
				TheString = FString::Printf(TEXT("  Audio Volume Reverb Effect: %s (Priority: %g Volume Name: %s)"), *CurrentAudioVolume->GetReverbSettings().ReverbEffect->GetName(), CurrentAudioVolume->GetPriority(), *CurrentAudioVolume->GetName());
			}
			else
			{
				TheString = TEXT("  Audio Volume Reverb Effect: None");
			}
			Canvas->DrawShadowedString(X, Y, *TheString, GetStatsFont(), LinearBodyColor);
			Y += Height;

			const TMap<FName, FActivatedReverb>& ActivatedReverbs = AtomRuntime->GetActiveReverb();
			if (ActivatedReverbs.Num() == 0)
			{
				TheString = TEXT("  Activated Reverb: None");
				Canvas->DrawShadowedString(X, Y, *TheString, GetStatsFont(), LinearBodyColor);
				Y += Height;
			}
			else if (ActivatedReverbs.Num() == 1)
			{
				auto It = ActivatedReverbs.CreateConstIterator();
				TheString = FString::Printf(TEXT("  Activated Reverb Effect: %s (Priority: %g Tag: '%s')"), *It.Value().ReverbSettings.ReverbEffect->GetName(), It.Value().Priority, *It.Key().ToString());
				Canvas->DrawShadowedString(X, Y, *TheString, GetStatsFont(), LinearBodyColor);
				Y += Height;
			}
			else
			{
				Canvas->DrawShadowedString(X, Y, TEXT("  Activated Reverb Effects:"), GetStatsFont(), LinearBodyColor);
				Y += Height;
				TMap<int32, FString> PrioritySortedActivatedReverbs;
				for (auto It = ActivatedReverbs.CreateConstIterator(); It; ++It)
				{
					TheString = FString::Printf(TEXT("    %s (Priority: %g Tag: '%s')"), *It.Value().ReverbSettings.ReverbEffect->GetName(), It.Value().Priority, *It.Key().ToString());
					PrioritySortedActivatedReverbs.Add(It.Value().Priority, TheString);
				}
				for (auto It = PrioritySortedActivatedReverbs.CreateConstIterator(); It; ++It)
				{
					Canvas->DrawShadowedString(X, Y, *It.Value(), GetStatsFont(), LinearBodyColor);
					Y += Height;
				}
			}
		}
		else
		{
			TheString = TEXT("Active Reverb Effect: None");
			Canvas->DrawShadowedString(X, Y, *TheString, GetStatsFont(), HeaderColor);
			Y += Height;
		}

		Y += Height;
		return Y;
	}*/

	// Render all Atom active sound
	int32 FAtomDebugger::RenderStatSounds(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugSoundsForAllViewsEnabled, DebugStatNames::Sounds, &AtomRuntime))
		{
			return Y;
		}

		const int32 FontHeight = GetStatsFont()->GetMaxCharHeight() + 2;

		FAtomStats& AtomStats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());
		const uint8 bDebug = AtomStats.DisplayFlags & static_cast<uint8>(FAtomStats::EDisplayFlags::Debug);
		UpdateDisplaySort(AtomStats);

		// Sort the list.
		FString SortingName;
		switch (AtomStats.DisplaySort)
		{
		case FAtomStats::EDisplaySort::Class:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.SoundClassName.LexicalLess(B.SoundClassName); });
			SortingName = TEXT("Class");
			break;
		}
		case FAtomStats::EDisplaySort::Distance:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.Distance < B.Distance; });
			SortingName = TEXT("Distance");
			break;
		}
		case FAtomStats::EDisplaySort::PlaybackTime:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.PlaybackTime > B.PlaybackTime; });
			SortingName = TEXT("Time");
			break;
		}
		case FAtomStats::EDisplaySort::Priority:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.Priority > B.Priority; });
			SortingName = TEXT("Priority");
			break;
		}
		case FAtomStats::EDisplaySort::Playbacks:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.PlaybackInstanceInfos.Num() > B.PlaybackInstanceInfos.Num(); });
			SortingName = TEXT("Playbacks");
			break;
		}
		case FAtomStats::EDisplaySort::Volume:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.Volume > B.Volume; });
			SortingName = TEXT("Volume");
			break;
		}
		case FAtomStats::EDisplaySort::Type:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B) { return A.Type > B.Type; });
			SortingName = TEXT("Type");
			break;
		}
		case FAtomStats::EDisplaySort::Name:
		default:
		{
			AtomStats.StatSoundInfos.Sort([](const FAtomStats::FStatSoundInfo& A, const FAtomStats::FStatSoundInfo& B)
			{
				if (AtomDebugSoundShowPathCVar)
				{
					return A.SoundPath < B.SoundPath;
				}
				return A.SoundName < B.SoundName;
			});
			SortingName = TEXT("Name");
			break;
		}
		}

		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Sounds:"), HeaderColor, X, Y);

		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		const FString InfoText = FString::Printf(TEXT(" Sorting By: %s, 3D Visualization: %s"), *SortingName,
			RuntimeManager->IsVisualizeDebug3dEnabled() || bAtomAttenuationVisualizeEnabledCVar ? TEXT("Enabled") : TEXT("Disabled"));
		Atom::DebugDrawString(ViewportSubsystem, InfoText, FColor(128, 255, 128), X, Y);

		static int32 FieldLength = 40;
		static int32 NameHeaderPad = static_cast<int32>(FieldLength * 1.25f);
		if (AtomStats.DisplaySort == FAtomStats::EDisplaySort::Name || AtomStats.DisplaySort == FAtomStats::EDisplaySort::Distance)
		{
			const FString FieldName = FString(TEXT("Distance")).RightPad(NameHeaderPad).Left(NameHeaderPad);
			Atom::DebugDrawString(ViewportSubsystem, FieldName, GetBodyColor(), X, Y);
		}
		else
		{
			const FString FieldName = SortingName.RightPad(FieldLength).Left(FieldLength);
			Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Index %s Name"), *FieldName), GetBodyColor(), X, Y);
		}

		int32 TotalPlaybacksNum = 0;
		const FColor BodyColor = GetStatSoundBodyColor();
		for (int32 SoundIndex = 0; SoundIndex < AtomStats.StatSoundInfos.Num(); ++SoundIndex)
		{
			const FAtomStats::FStatSoundInfo& StatSoundInfo = AtomStats.StatSoundInfos[SoundIndex];
			const int32 PlaybackInstanceNum = StatSoundInfo.PlaybackInstanceInfos.Num();
			if (PlaybackInstanceNum == 0)
			{
				continue;
			}
			TotalPlaybacksNum += PlaybackInstanceNum;

			if (SoundIndex >= AtomDebugSoundMaxNumDisplayedCVar)
			{
				continue;
			}

			bool bDisplayPlabacks = false;
			const FString DisplayName = AtomDebugSoundShowPathCVar ? StatSoundInfo.SoundPath : StatSoundInfo.SoundName;
			FString DebugValue;
			switch (AtomStats.DisplaySort)
			{
			case FAtomStats::EDisplaySort::Class:
			{
				DebugValue = *StatSoundInfo.SoundClassName.ToString();
				break;
			}
			case FAtomStats::EDisplaySort::PlaybackTime:
			{
				if (FMath::IsNearlyEqual(StatSoundInfo.PlaybackTime, StatSoundInfo.PlaybackTimeNonVirtualized))
				{
					DebugValue = FString::Printf(TEXT("%08.2f"), StatSoundInfo.PlaybackTime);
				}
				else
				{
					DebugValue = FString::Printf(TEXT("%08.2f (%08.2f Non-Virt)"), StatSoundInfo.PlaybackTime, StatSoundInfo.PlaybackTimeNonVirtualized);
				}
				break;
			}
			case FAtomStats::EDisplaySort::Priority:
			{
				if (FMath::IsNearlyEqual(StatSoundInfo.Priority, TNumericLimits<float>::Max()))
				{
					DebugValue += TEXT("Always  ");
				}
				else
				{
					DebugValue = FString::Printf(TEXT("%06.2f"), StatSoundInfo.Priority);
				}
				break;
			}
			case FAtomStats::EDisplaySort::Volume:
			{
				DebugValue = FString::Printf(TEXT("%01.2f (%04.2f dB)"), StatSoundInfo.Volume, Atom::ConvertToDecibels(StatSoundInfo.Volume));
				break;
			}
			case FAtomStats::EDisplaySort::Playbacks:
			{
				DebugValue = FString::Printf(TEXT("%03u"), StatSoundInfo.PlaybackInstanceInfos.Num());
				bDisplayPlabacks = true;
				break;
			}
			case FAtomStats::EDisplaySort::Type:
			{
				DebugValue = StatSoundInfo.Type == EAtomSoundType::Cue ? TEXT("Cue") : TEXT("Wave");
				break;
			}
			case FAtomStats::EDisplaySort::Name:
			case FAtomStats::EDisplaySort::Distance:
			default:
			{
				DebugValue = FString::Printf(TEXT("%08.2f"), StatSoundInfo.Distance);
				break;
			}
			}

			const FString DebugStr = FString::Printf(TEXT("  %03i    %s %s"), SoundIndex, *DebugValue.RightPad(FieldLength - 4).Left(FieldLength - 4), *DisplayName);
			Atom::DebugDrawString(ViewportSubsystem, DebugStr, BodyColor, X, Y);

			if (bDisplayPlabacks)
			{
				for (int32 PlaybackIndex = 0; PlaybackIndex < PlaybackInstanceNum; PlaybackIndex++)
				{
					const FString PlaybackStr = *FString::Printf(TEXT("    %02i    %s"), PlaybackIndex, *StatSoundInfo.PlaybackInstanceInfos[PlaybackIndex].Description);
					Atom::DebugDrawString(ViewportSubsystem, PlaybackStr, FColor(205, 205, 205), X, Y);
				}
			}
		}

		Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Atom Runtime ID: %u, Max Sounds Displayed: %i"), AtomRuntime->GetAtomRuntimeID(), AtomDebugSoundMaxNumDisplayedCVar), HeaderColor, X, Y);

		Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Total Sounds: %i, Sound Waves: %i"), AtomStats.StatSoundInfos.Num(), TotalPlaybacksNum), HeaderColor, X, Y);

		for (int32 i = 0; i < AtomStats.ListenerTransforms.Num(); ++i)
		{
			FString LocStr = AtomStats.ListenerTransforms[i].GetLocation().ToString();
			Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Listener '%d' Position: %s"), i, *LocStr), HeaderColor, X, Y);
		}

		Y += FontHeight;


		for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
		{
			const int32 PlaybackInstancesNum = StatSoundInfo.PlaybackInstanceInfos.Num();

			if (StatSoundInfo.Distance > 100.0f && PlaybackInstancesNum > 0)
			{
				float SphereRadius = 0.f;
				float SphereInnerRadius = 0.f;

				if (StatSoundInfo.ShapeDetailsMap.Num() > 0)
				{
					auto DrawDebugAttenuation = [=](const FTransform& SoundTransform)
					{
						const FString DebugName = AtomDebugSoundShowPathCVar ? StatSoundInfo.SoundPath : StatSoundInfo.SoundName;
						DrawDebugString(World, SoundTransform.GetTranslation(), *DebugName, nullptr, BodyColor, 0.01f);

						for (auto ShapeDetailsIt = StatSoundInfo.ShapeDetailsMap.CreateConstIterator(); ShapeDetailsIt; ++ShapeDetailsIt)
						{
							const FAtomAttenuationSettings::AttenuationShapeDetails& ShapeDetails = ShapeDetailsIt.Value();
							switch (ShapeDetailsIt.Key())
							{
							case EAtomAttenuationShape::Sphere:
								if (ShapeDetails.Falloff > 0.f)
								{
									DrawDebugSphere(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X + ShapeDetails.Falloff, 10, FColor(155, 155, 255));
									DrawDebugSphere(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X, 10, FColor(55, 55, 255));
								}
								else
								{
									DrawDebugSphere(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X, 10, FColor(155, 155, 255));
								}
								break;

								/*case EAtomAttenuationShape::Box:
									if (ShapeDetails.Falloff > 0.f)
									{
										DrawDebugBox(World, SoundTransform.GetTranslation(), ShapeDetails.Extents + FVector(ShapeDetails.Falloff), SoundTransform.GetRotation(), FColor(155, 155, 255));
										DrawDebugBox(World, SoundTransform.GetTranslation(), ShapeDetails.Extents, SoundTransform.GetRotation(), FColor(55, 55, 255));
									}
									else
									{
										DrawDebugBox(World, SoundTransform.GetTranslation(), ShapeDetails.Extents, SoundTransform.GetRotation(), FColor(155, 155, 255));
									}
									break;*/

									/*case EAtomAttenuationShape::Capsule:

										if (ShapeDetails.Falloff > 0.f)
										{
											DrawDebugCapsule(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X + ShapeDetails.Falloff, ShapeDetails.Extents.Y + ShapeDetails.Falloff, SoundTransform.GetRotation(), FColor(155, 155, 255));
											DrawDebugCapsule(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X, ShapeDetails.Extents.Y, SoundTransform.GetRotation(), FColor(55, 55, 255));
										}
										else
										{
											DrawDebugCapsule(World, SoundTransform.GetTranslation(), ShapeDetails.Extents.X, ShapeDetails.Extents.Y, SoundTransform.GetRotation(), FColor(155, 155, 255));
										}
										break;*/

							case EAtomAttenuationShape::Cone:
							{
								const FVector Origin = SoundTransform.GetTranslation() - (SoundTransform.GetUnitAxis(EAxis::X));

								if (ShapeDetails.Falloff > 0.f || ShapeDetails.Extents.Z > 0.f)
								{
									const float OuterAngle = FMath::DegreesToRadians(ShapeDetails.Extents.Y + ShapeDetails.Extents.Z);
									const float InnerAngle = FMath::DegreesToRadians(ShapeDetails.Extents.Y);
									DrawDebugCone(World, Origin, SoundTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X + ShapeDetails.ConeSphereFalloff, OuterAngle, OuterAngle, 10, FColor(155, 155, 255));
									DrawDebugCone(World, Origin, SoundTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X, InnerAngle, InnerAngle, 10, FColor(55, 55, 255));
								}
								else
								{
									const float Angle = FMath::DegreesToRadians(ShapeDetails.Extents.Y);
									DrawDebugCone(World, Origin, SoundTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X, Angle, Angle, 10, FColor(155, 155, 255));
								}

								if (!FMath::IsNearlyZero(ShapeDetails.ConeSphereRadius, KINDA_SMALL_NUMBER))
								{
									if (ShapeDetails.ConeSphereFalloff > 0.f)
									{

										DrawDebugSphere(World, Origin, ShapeDetails.ConeSphereRadius + ShapeDetails.ConeSphereFalloff, 10, FColor(155, 155, 255));
										DrawDebugSphere(World, Origin, ShapeDetails.ConeSphereRadius, 10, FColor(55, 55, 255));
									}
									else
									{
										DrawDebugSphere(World, Origin, ShapeDetails.ConeSphereRadius, 10, FColor(155, 155, 255));
									}
								}

								break;
							}

							default:
								check(false);
							}
						}
					};

					if (StatSoundInfo.Positions.IsEmpty())
					{
						DrawDebugAttenuation(StatSoundInfo.Transform);
					}
					else
					{
						for (const FTransform& Transform : StatSoundInfo.Positions)
						{
							DrawDebugAttenuation(Transform);
						}
					}
				}
			}
		}

		return Y;
	}

	// render Atom sound cues
	int32 FAtomDebugger::RenderStatCues(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugCuesForAllViewsEnabled, DebugStatNames::SoundCues, &AtomRuntime))
		{
			return Y;
		}

		UFont* StatsFont = GetStatsFont();
		const int32 FontSpacing = 2;
		const int32 FontHeight = StatsFont->GetMaxCharHeight() + FontSpacing;

		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Atom Sound Cues:"), HeaderColor, X, Y);

		int32 ActiveSoundCount = 0;

		FAtomStats& AtomStats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());
		if (!AtomSoundCueDebugMinimalCVar)
		{
			// show any the playback instances
			for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
			{
				if (StatSoundInfo.Type == EAtomSoundType::Cue)
				{
					for (const FAtomStats::FStatPlaybackInstanceInfo& PlaybackInstanceInfo : StatSoundInfo.PlaybackInstanceInfos)
					{
						if (PlaybackInstanceInfo.Volume >= MinDisplayVolume)
						{
							FColor Color = FColor::White;
							FString MuteSoloReason;

							if (FAtomSource::FDebugInfo* DebugInfo = PlaybackInstanceInfo.DebugInfo.Get())
							{
								// Color code same as icons. Red (mute), Yellow (solo), White (normal).
								FScopeLock Lock(&DebugInfo->CS);
								Color = DebugInfo->bIsMuted ? FColor::Red : DebugInfo->bIsSoloed ? FColor::Yellow : FColor::White;
								MuteSoloReason = !DebugInfo->MuteSoloReason.IsEmpty() ? FString::Printf(TEXT(" - %s"), *DebugInfo->MuteSoloReason) : TEXT("");
							}

							const FString TheString = FString::Printf(TEXT("%4i. %6.2f %s %s %s"), ActiveSoundCount++, PlaybackInstanceInfo.Volume, *StatSoundInfo.SoundName, *StatSoundInfo.SoundClassName.ToString(), *MuteSoloReason);
							Atom::DebugDrawString(ViewportSubsystem, TheString, Color, X, Y);
							break;
						}
					}
				}
			}
		}
		else
		{
			// show only the sound cue infromations
			FString SoundPath, SoundName;
			static int32 PrevNameLength = 0;
			static int32 PrevClassLength = 0;

			const int32 TabSpacing = FMath::Clamp(AtomSoundCueDebugTabSpacingCVar, 1, AtomSoundCueDebugTabSpacingCVar);
			const int32 CharSpacing = FMath::Clamp(AtomSoundCueDebugCharSpacingCVar, 1, AtomSoundCueDebugCharSpacingCVar);
			const int32 NumberSpacing = 6 * CharSpacing;	// 6 character len for 2 decimal float + 2 spaces 'X.XX  '

			// Tab out name and class length and reset previous length counters
			int32 TabbedName = (PrevNameLength / TabSpacing + 1) * TabSpacing;
			int32 TabbedClass = (PrevClassLength / TabSpacing + 1) * TabSpacing;
			PrevNameLength = PrevClassLength = 0;

			for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
			{
				if (StatSoundInfo.Type == EAtomSoundType::Cue)
				{
					for (const FAtomStats::FStatPlaybackInstanceInfo& PlaybackInstanceInfo : StatSoundInfo.PlaybackInstanceInfos)
					{
						if (PlaybackInstanceInfo.Volume >= MinDisplayVolume)
						{
							FColor Color = FColor::White;
							FString MuteSoloReason;
							bool bMutedOrSoloed = false;

							if (!StatSoundInfo.SoundPath.Split(TEXT("."), &SoundPath, &SoundName))
							{
								if (!StatSoundInfo.SoundPath.Split(SUBOBJECT_DELIMITER, &SoundPath, &SoundName))
								{
									SoundPath = StatSoundInfo.SoundPath;
								}
							}
							SoundName = StatSoundInfo.SoundName;

							if (SoundName.Len() > PrevNameLength)
							{
								PrevNameLength = SoundName.Len();
							}

							if ((int32)StatSoundInfo.SoundClassName.GetStringLength() > PrevClassLength)
							{
								PrevClassLength = StatSoundInfo.SoundClassName.GetStringLength();
							}

							if (FAtomSource::FDebugInfo* DebugInfo = PlaybackInstanceInfo.DebugInfo.Get())
							{
								// Color code same as icons. Red (mute), Yellow (solo), White (normal).
								FScopeLock Lock(&DebugInfo->CS);
								Color = DebugInfo->bIsMuted ? FColor::Red : DebugInfo->bIsSoloed ? FColor::Yellow : FColor::White;
								bMutedOrSoloed = DebugInfo->bIsMuted || DebugInfo->bIsSoloed;
								MuteSoloReason = !DebugInfo->MuteSoloReason.IsEmpty() ? FString::Printf(TEXT(" - %s"), *DebugInfo->MuteSoloReason) : TEXT("");
							}

							const int32 SoundNameIndex = ActiveSoundCount++;
							const FString LeadingNumber = FString::Printf(TEXT("%4i. "), SoundNameIndex);
							const FString Volume = FString::Printf(TEXT("%6.2f "), PlaybackInstanceInfo.Volume);
							const FString PathAndMuting = FString::Printf(TEXT("Path: %s %s"), *SoundPath, *MuteSoloReason);

							int32 CurrentX = X;
							Y -= Atom::DebugDrawString(ViewportSubsystem, LeadingNumber, Color, CurrentX, Y);
							CurrentX += NumberSpacing;
							Y -= Atom::DebugDrawString(ViewportSubsystem, Volume, bMutedOrSoloed ? Color : FColor::Orange, CurrentX, Y);
							CurrentX += NumberSpacing;

							if (AtomSoundCueDebugShowDistanceCVar)
							{
								const FString DistanceText = FString::Printf(TEXT("%6.2f "), StatSoundInfo.Distance);
								Y -= Atom::DebugDrawString(ViewportSubsystem, DistanceText, bMutedOrSoloed ? Color : FColor::White, CurrentX, Y);
								CurrentX += (NumberSpacing * 2);
							}

							Y -= Atom::DebugDrawString(ViewportSubsystem, SoundName, bMutedOrSoloed ? Color : FColor(0, 255, 255), CurrentX, Y);
							CurrentX += (TabbedName * CharSpacing);
							Y -= Atom::DebugDrawString(ViewportSubsystem, StatSoundInfo.SoundClassName.ToString(), bMutedOrSoloed ? Color : FColor::Yellow, CurrentX, Y);

							if (AtomSoundCueDebugShowPathCVar)
							{
								CurrentX += (TabbedClass * CharSpacing);
								Y -= Atom::DebugDrawString(ViewportSubsystem, PathAndMuting, Color, CurrentX, Y);
							}

							Y += FontHeight;
							break;
						}
					}
				}
			}
		}

		Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Total: %i"), ActiveSoundCount), GetBodyColor(), X, Y);

		return Y;
	}

	int32 FAtomDebugger::RenderStatWaves(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugWavesForAllViewsEnabled, DebugStatNames::SoundWaves, &AtomRuntime))
		{
			return Y;
		}

		UFont* StatsFont = GetStatsFont();
		const int32 FontSpacing = 2;
		const int32 FontHeight = StatsFont->GetMaxCharHeight() + FontSpacing;

		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Atom Sound Waves:"), HeaderColor, X, Y);

		int32 ActiveSoundCount = 0;

		FAtomStats& AtomStats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());
		if (!AtomSoundWaveDebugMinimalCVar)
		{
			// show any the playback instances
			for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
			{
				if (StatSoundInfo.Type == EAtomSoundType::Wave)
				{
					for (const FAtomStats::FStatPlaybackInstanceInfo& PlaybackInstanceInfo : StatSoundInfo.PlaybackInstanceInfos)
					{
						if (PlaybackInstanceInfo.Volume >= MinDisplayVolume)
						{
							FColor Color = FColor::White;
							FString MuteSoloReason;

							if (FAtomSource::FDebugInfo* DebugInfo = PlaybackInstanceInfo.DebugInfo.Get())
							{
								// Color code same as icons. Red (mute), Yellow (solo), White (normal).
								FScopeLock Lock(&DebugInfo->CS);
								Color = DebugInfo->bIsMuted ? FColor::Red : DebugInfo->bIsSoloed ? FColor::Yellow : FColor::White;
								MuteSoloReason = !DebugInfo->MuteSoloReason.IsEmpty() ? FString::Printf(TEXT(" - %s"), *DebugInfo->MuteSoloReason) : TEXT("");
							}

							const FString TheString = FString::Printf(TEXT("%4i. %6.2f %s %s %s"), ActiveSoundCount++, PlaybackInstanceInfo.Volume, *StatSoundInfo.SoundName, *StatSoundInfo.SoundClassName.ToString(), *MuteSoloReason);
							Atom::DebugDrawString(ViewportSubsystem, TheString, Color, X, Y);
							break;
						}
					}
				}
			}
		}
		else
		{
			// show only the sound cue infromations
			FString SoundPath, SoundName;
			static int32 PrevNameLength = 0;
			static int32 PrevClassLength = 0;

			const int32 TabSpacing = FMath::Clamp(AtomSoundWaveDebugTabSpacingCVar, 1, AtomSoundWaveDebugTabSpacingCVar);
			const int32 CharSpacing = FMath::Clamp(AtomSoundWaveDebugCharSpacingCVar, 1, AtomSoundWaveDebugCharSpacingCVar);
			const int32 NumberSpacing = 6 * CharSpacing;	// 6 character len for 2 decimal float + 2 spaces 'X.XX  '

			// Tab out name and class length and reset previous length counters
			int32 TabbedName = (PrevNameLength / TabSpacing + 1) * TabSpacing;
			int32 TabbedClass = (PrevClassLength / TabSpacing + 1) * TabSpacing;
			PrevNameLength = PrevClassLength = 0;

			for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
			{
				if (StatSoundInfo.Type == EAtomSoundType::Wave)
				{
					for (const FAtomStats::FStatPlaybackInstanceInfo& PlaybackInstanceInfo : StatSoundInfo.PlaybackInstanceInfos)
					{
						if (PlaybackInstanceInfo.Volume >= MinDisplayVolume)
						{
							FColor Color = FColor::White;
							FString MuteSoloReason;
							bool bMutedOrSoloed = false;

							if (!StatSoundInfo.SoundPath.Split(TEXT("."), &SoundPath, &SoundName))
							{
								if (!StatSoundInfo.SoundPath.Split(SUBOBJECT_DELIMITER, &SoundPath, &SoundName))
								{
									SoundPath = StatSoundInfo.SoundPath;
								}
							}
							SoundName = StatSoundInfo.SoundName;

							if (SoundName.Len() > PrevNameLength)
							{
								PrevNameLength = SoundName.Len();
							}

							if ((int32)StatSoundInfo.SoundClassName.GetStringLength() > PrevClassLength)
							{
								PrevClassLength = StatSoundInfo.SoundClassName.GetStringLength();
							}

							if (FAtomSource::FDebugInfo* DebugInfo = PlaybackInstanceInfo.DebugInfo.Get())
							{
								// Color code same as icons. Red (mute), Yellow (solo), White (normal).
								FScopeLock Lock(&DebugInfo->CS);
								Color = DebugInfo->bIsMuted ? FColor::Red : DebugInfo->bIsSoloed ? FColor::Yellow : FColor::White;
								bMutedOrSoloed = DebugInfo->bIsMuted || DebugInfo->bIsSoloed;
								MuteSoloReason = !DebugInfo->MuteSoloReason.IsEmpty() ? FString::Printf(TEXT(" - %s"), *DebugInfo->MuteSoloReason) : TEXT("");
							}

							const int32 SoundNameIndex = ActiveSoundCount++;
							const FString LeadingNumber = FString::Printf(TEXT("%4i. "), SoundNameIndex);
							const FString Volume = FString::Printf(TEXT("%6.2f "), PlaybackInstanceInfo.Volume);
							const FString PathAndMuting = FString::Printf(TEXT("Path: %s %s"), *SoundPath, *MuteSoloReason);

							int32 CurrentX = X;
							Y -= Atom::DebugDrawString(ViewportSubsystem, LeadingNumber, Color, CurrentX, Y);
							CurrentX += NumberSpacing;
							Y -= Atom::DebugDrawString(ViewportSubsystem, Volume, bMutedOrSoloed ? Color : FColor::Orange, CurrentX, Y);
							CurrentX += NumberSpacing;

							if (AtomSoundWaveDebugShowDistanceCVar)
							{
								const FString DistanceText = FString::Printf(TEXT("%6.2f "), StatSoundInfo.Distance);
								Y -= Atom::DebugDrawString(ViewportSubsystem, DistanceText, bMutedOrSoloed ? Color : FColor::White, CurrentX, Y);
								CurrentX += (NumberSpacing * 2);
							}

							Y -= Atom::DebugDrawString(ViewportSubsystem, SoundName, bMutedOrSoloed ? Color : FColor(0, 255, 255), CurrentX, Y);
							CurrentX += (TabbedName * CharSpacing);
							Y -= Atom::DebugDrawString(ViewportSubsystem, StatSoundInfo.SoundClassName.ToString(), bMutedOrSoloed ? Color : FColor::Yellow, CurrentX, Y);

							if (AtomSoundWaveDebugShowPathCVar)
							{
								CurrentX += (TabbedClass * CharSpacing);
								Y -= Atom::DebugDrawString(ViewportSubsystem, PathAndMuting, Color, CurrentX, Y);
							}

							Y += FontHeight;
							break;
						}
					}
				}
			}
		}

		Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT("Total: %i"), ActiveSoundCount), GetBodyColor(), X, Y);

		return Y;
	}

	// render playback instances
	int32 FAtomDebugger::RenderStatPlaybacks(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugWavesForAllViewsEnabled, DebugStatNames::SoundPlaybacks, &AtomRuntime))
		{
			return Y;
		}

		const int32 FontHeight = GetStatsFont()->GetMaxCharHeight() + 2;

		FAtomStats& AtomStats = AtomRuntimeStats.FindOrAdd(AtomRuntime->GetAtomRuntimeID());
		Atom::DebugDrawString(ViewportSubsystem, TEXT("Active Atom Playbacks:"), FLinearColor::Green, X, Y);

		using FPlaybackInstancePair = TPair<const FAtomStats::FStatPlaybackInstanceInfo*, const FAtomStats::FStatSoundInfo*>;
		TArray<FPlaybackInstancePair> PlaybackInstances;
		for (const FAtomStats::FStatSoundInfo& StatSoundInfo : AtomStats.StatSoundInfos)
		{
			for (const FAtomStats::FStatPlaybackInstanceInfo& PlaybackInstanceInfo : StatSoundInfo.PlaybackInstanceInfos)
			{
				if (PlaybackInstanceInfo.Volume >= MinDisplayVolume || PlaybackInstanceInfo.bPlayWhenSilent != 0)
				{
					PlaybackInstances.Emplace(&PlaybackInstanceInfo, &StatSoundInfo);
				}
			}
		}

		PlaybackInstances.Sort([](const FPlaybackInstancePair& A, const FPlaybackInstancePair& B) { return A.Key->InstanceIndex < B.Key->InstanceIndex; });

		const FColor BodyColor = GetBodyColor();
		for (const FPlaybackInstancePair& PlaybackInstanceInfo : PlaybackInstances)
		{
			UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(PlaybackInstanceInfo.Value->AtomComponentID);
			AActor* SoundOwner = AtomComponent ? AtomComponent->GetOwner() : nullptr;

			FString TheString = *FString::Printf(TEXT("%4i.    %6.2f  %s   Owner: %s   SoundClass: %s"),
				PlaybackInstanceInfo.Key->InstanceIndex,
				PlaybackInstanceInfo.Key->Volume,
				*PlaybackInstanceInfo.Key->PlaybackInstanceName.ToString(),
				SoundOwner ? *SoundOwner->GetName() : TEXT("None"),
				*PlaybackInstanceInfo.Key->SoundClassName.ToString());
			Atom::DebugDrawString(ViewportSubsystem, TheString, PlaybackInstanceInfo.Key->bPlayWhenSilent == 0 ? BodyColor : FColor::Yellow, X, Y);
		}

		const int32 ActiveInstances = PlaybackInstances.Num();

		const int32 Max = AtomRuntime->GetMaxChannels() / 2;
		float f = FMath::Clamp<float>((float)(ActiveInstances - Max) / (float)Max, 0.f, 1.f);
		const int32 R = FMath::TruncToInt(f * 255);

		if (ActiveInstances > Max)
		{
			f = FMath::Clamp<float>((float)(Max - ActiveInstances) / (float)Max, 0.5f, 1.f);
		}
		else
		{
			f = 1.0f;
		}
		const int32 G = FMath::TruncToInt(f * 255);
		const int32 B = 0;

		Atom::DebugDrawString(ViewportSubsystem, FString::Printf(TEXT(" Total: %i"), ActiveInstances), FColor(R, G, B), X, Y);

		Y += FontHeight;
		return Y;
	}

	int32 FAtomDebugger::RenderStatStreaming(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y, const FVector* ViewLocation /*= nullptr*/, const FRotator* ViewRotation /*= nullptr*/)
	{
		FAtomRuntime* AtomRuntime = nullptr;
		if (!DebugShouldRenderStat(World, ViewportSubsystem, bDebugSoundsForAllViewsEnabled, DebugStatNames::AudioStreaming, &AtomRuntime))
		{
			return Y;
		}

		//return IStreamingManager::Get().GetAudioStreamingManager().RenderStatAudioStreaming(World, Viewport, Canvas, X, Y, ViewLocation, ViewRotation);
		return Y;
	}

	void FAtomDebugger::RemoveRuntime(const FAtomRuntime& AtomRuntime)
	{
		AtomRuntimeStats.Remove(AtomRuntime.GetAtomRuntimeID());
		AtomRuntimeStats_AtomThread.Remove(AtomRuntime.GetAtomRuntimeID());
	}

	void FAtomDebugger::ClearStat(const FName StatToClear, UWorld* InWorld)
	{
		if (!GCriWare || !InWorld)
		{
			return;
		}

		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			FAtomRuntimeId WorldRuntimeID = FAtomRuntimeManager::GetAtomRuntimeFromWorld(InWorld).GetRuntimeID();

			FAtomDebugger& DebuggerInstance = RuntimeManager->GetDebugger();
			RuntimeManager->IterateOverAllRuntimes(
				[&DebuggerInstance, WorldRuntimeID, ClearedStat = StatToClear](FAtomRuntimeId RuntimeID, const FAtomRuntime* AtomRuntime)
				{
					if (WorldRuntimeID == RuntimeID)
					{
						DebuggerInstance.ClearStat(RuntimeID, ClearedStat);
					}
				}
			);
		}
	}

	void FAtomDebugger::ClearStat(FAtomRuntimeId RuntimeID, const FName StatToClear)
	{
		if (IsInGameThread())
		{
			FAtomStats& Stats = AtomRuntimeStats.FindOrAdd(RuntimeID);
			Stats.EnabledStats.Remove(StatToClear);
		}

		if (!IsInAtomThread())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.ClearStat"), STAT_AtomClearStat, STATGROUP_TaskGraphTasks);

			FAtomThread::RunCommandOnAtomThread([this, RuntimeID, StatToClear]()
			{
				ClearStat(RuntimeID, StatToClear);
			}, GET_STATID(STAT_AtomClearStat));
			return;
		}

		FAtomStats_AtomThread& Stats = AtomRuntimeStats_AtomThread.FindOrAdd(RuntimeID);
		Stats.RequestedStats.Remove(StatToClear);
	}

	void FAtomDebugger::SetStats(const TSet<FName>& StatsToSet, UWorld* InWorld)
	{
		if (!GCriWare || !InWorld)
		{
			return;
		}

		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			FAtomRuntimeId WorldRuntimeID = FAtomRuntimeManager::GetAtomRuntimeFromWorld(InWorld).GetRuntimeID();

			FAtomDebugger& DebuggerInstance = RuntimeManager->GetDebugger();
			RuntimeManager->IterateOverAllRuntimes(
				[&DebuggerInstance, WorldRuntimeID, StatsSet = StatsToSet](FAtomRuntimeId RuntimeID, const FAtomRuntime* AtomRuntime)
				{
					if (WorldRuntimeID == RuntimeID)
					{
						DebuggerInstance.SetStats(RuntimeID, StatsSet);
					}
				}
			);
		}
	}

	void FAtomDebugger::SetStats(FAtomRuntimeId RuntimeID, const TSet<FName>& StatsToSet)
	{
		if (IsInGameThread())
		{
			FAtomStats& Stats = AtomRuntimeStats.FindOrAdd(RuntimeID);
			Stats.EnabledStats.Append(StatsToSet);
		}

		if (!IsInAtomThread())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SetStats"), STAT_AtomSetStats, STATGROUP_TaskGraphTasks);

			FAtomThread::RunCommandOnAtomThread([this, RuntimeID, StatsToSet]()
			{
				SetStats(RuntimeID, StatsToSet);
			}, GET_STATID(STAT_AtomSetStats));
			return;
		}

		FAtomStats_AtomThread& Stats = AtomRuntimeStats_AtomThread.FindOrAdd(RuntimeID);
		Stats.RequestedStats.Append(StatsToSet);
	}

	bool FAtomDebugger::PostStatModulatorHelp(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream)
	{
		// Ignore if all Viewports are closed.
		if (!ViewportClient)
		{
			return false;
		}

		if (!GCriWare)
		{
			return false;
		}

		if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World))
		{
			if (AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem())
			{
				if (!Modulation->OnPostHelp(ViewportClient, Stream))
				{
					return false;
				}
			}
		}

		return true;
	}

	void FAtomDebugger::SendUpdateResultsToGameThread(const FAtomRuntime& AtomRuntime, const int32 FirstActiveIndex)
	{
		check(IsInAtomThread());

		FAtomStats_AtomThread* Stats_AtomThread = AtomRuntimeStats_AtomThread.Find(AtomRuntime.GetAtomRuntimeID());
		if (!Stats_AtomThread)
		{
			return;
		}

		TArray<FAtomStats::FStatSoundInfo> StatSoundInfos;
		TArray<FAtomStats::FStatSoundRackInfo> StatSoundRacks;

		const TSet<FName> RequestedStats = Stats_AtomThread->RequestedStats;
		TMap<FAtomActiveSound*, int32> ActiveSoundToInfoIndex;

		const bool bDebug = RequestedStats.Contains(DebugStatNames::DebugSounds);

		if (RequestedStats.Contains(DebugStatNames::Sounds) 
			|| RequestedStats.Contains(DebugStatNames::SoundCues)
			|| RequestedStats.Contains(DebugStatNames::SoundRacks) 
			|| RequestedStats.Contains(DebugStatNames::SoundWaves)
			|| RequestedStats.Contains(DebugStatNames::SoundPlaybacks))
		{
			for (FAtomActiveSound* ActiveSound : AtomRuntime.GetActiveSounds())
			{
				if (UAtomSoundBase* SoundBase = ActiveSound->GetSound())
				{
					if (!bDebug || ActiveSound->GetSound()->bDebug)
					{
						ActiveSoundToInfoIndex.Add(ActiveSound, StatSoundInfos.AddDefaulted());
						FAtomStats::FStatSoundInfo& StatSoundInfo = StatSoundInfos.Last();
						StatSoundInfo.SoundName = SoundBase->GetName();
						StatSoundInfo.SoundPath = SoundBase->GetPathName();
						StatSoundInfo.Distance = 0.f;
						if (ActiveSound->bAllowSpatialization)
						{
							ActiveSound->GetMultiplePositions(StatSoundInfo.Positions);
							if (StatSoundInfo.Positions.Num() > 0)
							{
								for (const FTransform& Transform : StatSoundInfo.Positions)
								{
									StatSoundInfo.Distance = FMath::Max(StatSoundInfo.Distance, AtomRuntime.GetDistanceToNearestListener(Transform.GetTranslation()));
								}
							}
							else
							{
								StatSoundInfo.Distance = AtomRuntime.GetDistanceToNearestListener(ActiveSound->Transform.GetTranslation());
							}
						}
						StatSoundInfo.PlaybackTime = ActiveSound->PlaybackTime;
						StatSoundInfo.Priority = 0.0f; //ActiveSound->GetHighestPriority();
						StatSoundInfo.PlaybackTimeNonVirtualized = ActiveSound->PlaybackTimeNonVirtualized;

						StatSoundInfo.Volume = 0.0f;
						for (const TPair<UPTRINT, FAtomPlaybackInstance*>& Pair : ActiveSound->GetPlaybackInstances())
						{
							StatSoundInfo.Volume = FMath::Max(StatSoundInfo.Volume, Pair.Value->GetVolumeWithDistanceAndOcclusionAttenuation() * Pair.Value->GetDynamicVolume());
						}

						if (UAtomSoundClass* SoundClass = ActiveSound->GetSoundClass())
						{
							StatSoundInfo.SoundClassName = SoundClass->GetFName();
						}
						else
						{
							StatSoundInfo.SoundClassName = NAME_None;
						}

						StatSoundInfo.Transform = ActiveSound->Transform;
						StatSoundInfo.AtomComponentID = ActiveSound->GetAtomComponentID();
						
						if (UAtomSoundBase* Sound = ActiveSound->GetSound())
						{
							StatSoundInfo.Type = Cast<UAtomSoundCue>(Sound) ? EAtomSoundType::Cue : EAtomSoundType::Wave;

							if (bAtomAttenuationVisualizeEnabledCVar && Sound->bDebug)
							{
								ActiveSound->CollectAttenuationShapesForVisualization(StatSoundInfo.ShapeDetailsMap);
							}
						}
					}
				}
			}

			// Iterate through all playback instances.
			const TArray<FAtomPlaybackInstance*>& PlaybackInstances = AtomRuntime.GetActivePlaybackInstances();
			auto PlaybackInstanceSourceMap = AtomRuntime.GetPlaybackInstanceSourceMap();
			for (int32 InstanceIndex = FirstActiveIndex; InstanceIndex < PlaybackInstances.Num(); ++InstanceIndex)
			{
				const FAtomPlaybackInstance* PlaybackInstance = PlaybackInstances[InstanceIndex];
				const FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
				check(ActiveSound);

				if (const int32* SoundInfoIndex = ActiveSoundToInfoIndex.Find(ActiveSound))
				{
					const UAtomSoundClass* SoundClass = ActiveSound->GetSoundClass();
					if (const UAtomSoundClass* PlaybackSoundClass = PlaybackInstance->SoundClass)
					{
						SoundClass = PlaybackSoundClass;
					}

					FAtomStats::FStatPlaybackInstanceInfo PlaybackInstanceInfo;
					FAtomSource* Source = PlaybackInstanceSourceMap.FindRef(PlaybackInstance);
					PlaybackInstanceInfo.Description = Source ? Source->Describe(RequestedStats.Contains(DebugStatNames::LongSoundNames)) : FString(TEXT("No source"));
					PlaybackInstanceInfo.Volume = PlaybackInstance->GetVolumeWithDistanceAndOcclusionAttenuation() * PlaybackInstance->GetDynamicVolume();
					PlaybackInstanceInfo.InstanceIndex = InstanceIndex;
					PlaybackInstanceInfo.PlaybackInstanceName = *PlaybackInstance->GetName();
					PlaybackInstanceInfo.bPlayWhenSilent = ActiveSound->IsPlayWhenSilent() ? 1 : 0;
					PlaybackInstanceInfo.DebugInfo = Source ? Source->DebugInfo : PlaybackInstanceInfo.DebugInfo;
					PlaybackInstanceInfo.SoundClassName = SoundClass ? SoundClass->GetFName() : NAME_None;
					StatSoundInfos[*SoundInfoIndex].PlaybackInstanceInfos.Add(MoveTemp(PlaybackInstanceInfo));
				}
			}
		}

		if (RequestedStats.Contains(DebugStatNames::SoundRacks))
		{
			AtomRuntime.IterateOverRacks([&](const UAtomRackBase* Rack)
			{
				StatSoundRacks.AddDefaulted();
				FAtomStats::FStatSoundRackInfo& StatSoundRack = StatSoundRacks.Last();
				StatSoundRack.RackName = Rack->GetName();
				if (const UAtomRack* AsrRack = Cast<UAtomRack>(Rack))
				{
					StatSoundRack.DspBusSettingsName = AsrRack->DspBusSettingName;

					for (auto& Bus : AsrRack->Buses)
					{
						StatSoundRack.BusInfos.AddDefaulted();
						FAtomStats::FStatSoundBusInfo& StatSoundBus = StatSoundRack.BusInfos.Last();
						StatSoundBus.BusName = Bus->GetBusName();
						StatSoundBus.Volume = AtomRuntime.GetBusVolume(Bus);
						StatSoundBus.InterpValue = 0.0f; // fade 

						TArray<FName> EffectNames;
						for (int Index = 0; Index < Bus->GetNumEffectPresets(); ++Index)
						{
							EffectNames.Add(Bus->GetEffectPreset(Index)->GetEffectName());
						}
						StatSoundBus.EffectChainNames = EffectNames;
					}
				}
			});

			/*if (const FAtomEffectsManager* Effects = AtomRuntime.GetEffects())
			{
				const USoundMix* CurrentEQMix = Effects->GetCurrentEQMix();

				for (const TPair<USoundMix*, FSoundMixState>& SoundMixPair : AtomRuntime.GetSoundMixModifiers())
				{
					StatSoundRacks.AddDefaulted();
					FAtomStats::FStatSoundRack& StatSoundRack = StatSoundRacks.Last();
					StatSoundRack.MixName = SoundMixPair.Key->GetName();
					StatSoundRack.InterpValue = SoundMixPair.Value.InterpValue;
					StatSoundRack.RefCount = SoundMixPair.Value.ActiveRefCount + SoundMixPair.Value.PassiveRefCount;
					StatSoundRack.bIsCurrentEQ = (SoundMixPair.Key == CurrentEQMix);
				}
			}*/
		}

		DECLARE_CYCLE_STAT(TEXT("FGameThreadAtomTask.AtomSendResults"), STAT_AtomSendResults, STATGROUP_TaskGraphTasks);

		const uint32 AtomRuntimeID = AtomRuntime.GetAtomRuntimeID();

		TArray<FTransform> ListenerTransforms;
		for (const FAtomListener& Listener : AtomRuntime.GetListeners())
		{
			ListenerTransforms.Add(Listener.Transform);
		}
		FAtomThread::RunCommandOnGameThread([AtomRuntimeID, ListenerTransforms, StatSoundInfos, StatSoundRacks]()
		{
			FAtomStats& Stats = AtomRuntimeStats.FindOrAdd(AtomRuntimeID);
			Stats.ListenerTransforms = ListenerTransforms;
			Stats.StatSoundInfos = StatSoundInfos;
			Stats.StatSoundRackInfos = StatSoundRacks;
		}, GET_STATID(STAT_AtomSendResults));
	}

	void FAtomDebugger::UpdateAudibleInactiveSounds(const uint32 FirstActiveIndex, const TArray<FAtomPlaybackInstance*>& PlaybackInstances)
	{
	#if STATS
		uint32 AudibleInactiveSounds = 0;
		// Count how many sounds are not being played but were audible
		for (uint32 InstanceIndex = 0; InstanceIndex < FirstActiveIndex; ++InstanceIndex)
		{
			const FAtomPlaybackInstance* PlaybackInstance = PlaybackInstances[InstanceIndex];
			const float WaveInstanceVol = PlaybackInstance->GetVolumeWithDistanceAndOcclusionAttenuation() * PlaybackInstance->GetDynamicVolume();
			if (WaveInstanceVol > MinDisplayVolume)
			{
				AudibleInactiveSounds++;
			}
		}
		SET_DWORD_STAT(STAT_AtomAudiblePlaybacksDroppedDueToPriority, AudibleInactiveSounds);
	#endif
	}

	void FAtomDebugger::ClearMutesAndSolos()
	{
		DebugNames.MuteSoundClass.Empty();
		DebugNames.MuteSoundCue.Empty();
		DebugNames.MuteSoundWave.Empty();
		DebugNames.SoloSoundClass.Empty();
		DebugNames.SoloSoundCue.Empty();
		DebugNames.SoloSoundWave.Empty();
	}

	/*void FAtomDebugger::LogSubtitle(const TCHAR* InCmd, USoundWave& InSoundWave)
	{
		const bool bLogSubtitle = FParse::Param(InCmd, TEXT("LogSubtitle"));
		if (bLogSubtitle)
		{
			FString Subtitle;
			for (int32 i = 0; i < InSoundWave.Subtitles.Num(); i++)
			{
				Subtitle += InSoundWave.Subtitles[i].Text.ToString();
			}

			if (Subtitle.Len() == 0)
			{
				Subtitle = InSoundWave.SpokenText_DEPRECATED;
			}

			if (Subtitle.Len() == 0)
			{
				Subtitle = "<NO SUBTITLE>";
			}

			UE_LOG(LogAudio, Display, TEXT("Subtitle:  %s"), *Subtitle);
	#if WITH_EDITORONLY_DATA
			UE_LOG(LogAudio, Display, TEXT("Comment:   %s"), *InSoundWave.Comment);
	#endif // WITH_EDITORONLY_DATA
			UE_LOG(LogAudio, Display, TEXT("Mature:    %s"), InSoundWave.bMature ? TEXT("Yes") : TEXT("No"));
		}
	}*/
} // namespace Atom

#endif // ENABLE_AUDIO_DEBUG
