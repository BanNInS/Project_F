/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomDebug.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "CriWareDefines.h"		// For ENABLE_ATOM_DEBUG
#include "Atom.h"

#if ENABLE_ATOM_DEBUG

 // Forward Declarations
struct FAtomActiveSound;
struct FAtomVirtualLoop;
struct FAtomListener;
class FAtomSource;
class UAtomSoundWave;
class UAtomSoundCue;
class UWorld;
class UViewportStatsSubsystem;
class FViewportClient;

namespace Atom
{
	namespace DebugStatNames
	{
		CRIWARECORE_API extern const FName SoundPlaybacks;
		CRIWARECORE_API extern const FName SoundWaves;
		CRIWARECORE_API extern const FName SoundCues;
		CRIWARECORE_API extern const FName Sounds;
		CRIWARECORE_API extern const FName SoundRacks;
		CRIWARECORE_API extern const FName SoundModulation;
		//CRIWARECORE_API extern const FName SoundReverb;
		CRIWARECORE_API extern const FName AudioStreaming;

		// TODO: Move to console variables
		CRIWARECORE_API extern const FName DebugSounds;
		CRIWARECORE_API extern const FName LongSoundNames;
	}

	// Declare Viewport stat subsystem draw string function.
	int32 DebugDrawString(UViewportStatsSubsystem* ViewportSubsystem, FString String, const FLinearColor& Color = FLinearColor::White, float OffsetX = 0.0f, float OffsetY = 0.0f);

	class CRIWARECORE_API FAtomDebugger
	{
	public:

		FAtomDebugger();

		/** Struct which contains debug names for run-time debugging of sounds. */
		struct FDebugNames
		{
			TArray<FName> SoloSoundClass;
			TArray<FName> SoloSoundWave;
			TArray<FName> SoloSoundCue;
			TArray<FName> MuteSoundClass;
			TArray<FName> MuteSoundWave;
			TArray<FName> MuteSoundCue;

			FString DebugAtomMixerSoundName;
			FString DebugSoundName;
			bool bDebugSoundName;

			FDebugNames()
				: bDebugSoundName(false)
			{}
		};

		static void DrawDebugInfo(const FAtomSource& SoundSource);
		static void DrawDebugInfo(const FAtomActiveSound& ActiveSound, const TArray<FAtomPlaybackInstance*>& ThisSoundsPlaybackInstances, const float DeltaTime);
		static void DrawDebugInfo(UWorld& World, const TArray<FAtomListener>& Listeners);
		static void DrawDebugInfo(const FAtomVirtualLoop& VirtualLoop);
		static void DrawDebugStatsToViewportStats();
		static int32 DrawDebugStats(UWorld& World, UViewportStatsSubsystem* ViewportSubsystem, int32 Y);
		static bool DrawDebugStatsEnabled();
		static bool PostStatModulatorHelp(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static int32 RenderStatRacks(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatModulators(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation);
		//static int32 RenderStatReverb(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatSounds(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatCues(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatWaves(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatPlaybacks(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y);
		static int32 RenderStatStreaming(UWorld* World, UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y, const FVector* ViewLocation, const FRotator* ViewRotation);
		static void RemoveRuntime(const FAtomRuntime& AtomRuntime);
		static void ResolveDesiredStats(FViewportClient* ViewportClient);
		static void SendUpdateResultsToGameThread(const FAtomRuntime& AtomRuntime, const int32 FirstActiveIndex);
		static bool ToggleStatCues(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static bool ToggleStatRacks(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static bool ToggleStatModulators(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static bool ToggleStatSounds(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static bool ToggleStatWaves(UWorld* World, FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		static void UpdateAudibleInactiveSounds(const uint32 FirstIndex, const TArray<FAtomPlaybackInstance*>& PlaybackInstances);
		//static void LogSubtitle(const TCHAR* InCmd, USoundWave& InSoundWave);
		static void ClearStat(const FName StatToClear, UWorld* InWorld);
		static void SetStats(const TSet<FName>& StatsToSet, UWorld* InWorld);

		static bool IsVirtualLoopVisualizeEnabled();

		void ClearMutesAndSolos();
		void DumpActiveSounds() const;

		bool IsVisualizeDebug3dEnabled() const;
		void ToggleVisualizeDebug3dEnabled();

#if WITH_EDITOR
		static void OnBeginPIE();
		static void OnEndPIE();
#endif // WITH_EDITOR

		// Evaluate Mute/Solos
		void QuerySoloMuteSoundClass(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const;
		void QuerySoloMuteSoundWave(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const;
		void QuerySoloMuteSoundCue(const FString& Name, bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const;

		// Is Mute/Solos. (only audio thread).
		bool IsSoloSoundClass(FName InName) const { return DebugNames.SoloSoundClass.Contains(InName); }
		bool IsSoloSoundWave(FName InName) const { return DebugNames.SoloSoundWave.Contains(InName); }
		bool IsSoloSoundCue(FName InName) const { return DebugNames.SoloSoundCue.Contains(InName); }
		bool IsMuteSoundClass(FName InName) const { return DebugNames.MuteSoundClass.Contains(InName); }
		bool IsMuteSoundWave(FName InName) const { return DebugNames.MuteSoundWave.Contains(InName); }
		bool IsMuteSoundCue(FName InName) const { return DebugNames.MuteSoundCue.Contains(InName); }

		// Mute/Solos toggles. (any thread). (If exclusive, toggle-on will clear everything first, and toggle-off will clear all).
		void ToggleSoloSoundClass(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.SoloSoundClass, bExclusive); }
		void ToggleSoloSoundWave(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.SoloSoundWave, bExclusive); }
		void ToggleSoloSoundCue(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.SoloSoundCue, bExclusive); }
		void ToggleMuteSoundClass(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.MuteSoundClass, bExclusive); }
		void ToggleMuteSoundWave(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.MuteSoundWave, bExclusive); }
		void ToggleMuteSoundCue(FName InName, bool bExclusive = false) { ToggleNameArray(InName, DebugNames.MuteSoundCue, bExclusive); }

		// Set Mute/Solo. (any thread).
		void SetMuteSoundCue(FName InName, bool bInOnOff) { SetNameArray(InName, DebugNames.MuteSoundCue, bInOnOff); }
		void SetMuteSoundWave(FName InName, bool bInOnOff) { SetNameArray(InName, DebugNames.MuteSoundWave, bInOnOff); }
		void SetSoloSoundCue(FName InName, bool bInOnOff) { SetNameArray(InName, DebugNames.SoloSoundCue, bInOnOff); }
		void SetSoloSoundWave(FName InName, bool bInOnOff) { SetNameArray(InName, DebugNames.SoloSoundWave, bInOnOff); }

		void SetAtomMixerDebugSound(const TCHAR* SoundName);
		void SetAtomDebugSound(const TCHAR* SoundName);

		const FString& GetAtomMixerDebugSoundName() const;
		bool GetAtomDebugSound(FString& OutDebugSound);

	private:

		static int32 DrawDebugStatsInternal(UWorld& World, UViewportStatsSubsystem& ViewportSubsystem, int32 InY);
		void SetNameArray(FName InName, TArray<FName>& InNameArray, bool bOnOff);
		void ToggleNameArray(FName InName, TArray<FName>& NameArray, bool bExclusive);
		void ExecuteCmdOnAtomThread(TFunction<void()> Cmd);

		void GetDebugSoloMuteStateX(
			const FString& Name, const TArray<FName>& Solos, const TArray<FName>& Mutes,
			bool& bOutIsSoloed, bool& bOutIsMuted, FString& OutReason) const;

		void ClearStat(FAtomRuntimeId RuntimeID, const FName StatToClear);

		void SetStats(FAtomRuntimeId RuntimeID, const TSet<FName>& StatsToSet);

		static bool ToggleStats(UWorld* World, const TSet<FName>& StatsToToggle);
		void ToggleStats(FAtomRuntimeId RuntimeID, const TSet<FName>& StatsToToggle);

		FDelegateHandle WorldRegisteredWithRuntimeHandle;

		/** Instance of the debug names struct. */
		FDebugNames DebugNames;

		/** Whether or not 3d debug visualization is enabled. */
		uint8 bVisualize3dDebug : 1;
	};
} // namespace Atom

#endif // ENABLE_ATOM_DEBUG
