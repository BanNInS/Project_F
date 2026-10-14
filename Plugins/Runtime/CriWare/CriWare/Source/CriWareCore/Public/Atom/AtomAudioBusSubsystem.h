
#pragma once

#include "CoreMinimal.h"
#include "Templates/TypeHash.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/TVariant.h"
#include "DSP/MultithreadedPatching.h"

#include "Atom/AtomEngineSubsystem.h"

#include "AtomAudioBusSubsystem.generated.h"

// Forward Declarations
class UAtomAudioBus;

namespace Atom
{
	// Forward declarations 
	class FMixerAudioBus;
	class FMixerSourceManager;

	struct CRIWARECORE_API FAudioBusKey
	{
	public:

		uint32 ObjectId = INDEX_NONE; // from a corresponding UObject (UAtomAudioBus) if applicable
		uint32 InstanceId = INDEX_NONE;

		FAudioBusKey()
			: InstanceId(InstanceIdCounter++)
		{
		}

		// For construction with a given UObject unique id 
		FAudioBusKey(uint32 InObjectId)
			: ObjectId(InObjectId)
		{
		}

		const bool IsValid() const
		{
			return ObjectId != INDEX_NONE || InstanceId != INDEX_NONE;
		}

		FORCEINLINE friend uint32 GetTypeHash(const FAudioBusKey& Key)
		{
			return HashCombineFast(Key.ObjectId, Key.InstanceId);
		}
		 		
		FORCEINLINE friend bool operator==(const FAudioBusKey& InLHS, const FAudioBusKey& InRHS) 
		{
			return (InLHS.ObjectId == InRHS.ObjectId) && (InLHS.InstanceId == InRHS.InstanceId);
		}

		FORCEINLINE friend bool operator!=(const FAudioBusKey& InLHS, const FAudioBusKey& InRHS) 
		{
			return !(InLHS == InRHS);
		}
		
	private:

		static std::atomic<uint32> InstanceIdCounter;
	};
} // namespace

/**
*  UAtomAudioBusSubsystem
*/
UCLASS(MinimalAPI)
class UAtomAudioBusSubsystem : public UAtomEngineSubsystem
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomAudioBusSubsystem();
	virtual ~UAtomAudioBusSubsystem() = default;

	//~ Begin USubsystem interface
	CRIWARECORE_API virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	CRIWARECORE_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	CRIWARECORE_API virtual void Deinitialize() override;
	//~ End USubsystem interface

	// Audio bus API from FMixerDevice
	UE_DEPRECATED(5.0, "Use the StartAudioBus version that requires an AudioBus name.")
	CRIWARECORE_API void StartAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InNumChannels, bool bInIsAutomatic);
	CRIWARECORE_API void StartAudioBus(Atom::FAudioBusKey InAudioBusKey, const FString& InAudioBusName, int32 InNumChannels, bool bInIsAutomatic);
	CRIWARECORE_API void StopAudioBus(Atom::FAudioBusKey InAudioBusKey);
	CRIWARECORE_API bool IsAudioBusActive(Atom::FAudioBusKey InAudioBusKey) const;
	
	CRIWARECORE_API Audio::FPatchInput AddPatchInputForAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InFrames, int32 InChannels, float InGain = 1.f);
	CRIWARECORE_API Audio::FPatchOutputStrongPtr AddPatchOutputForAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InFrames, int32 InChannels, float InGain = 1.f);
	
	CRIWARECORE_API Audio::FPatchInput AddPatchInputForSoundAndAudioBus(uint64 SoundInstanceID, Atom::FAudioBusKey AudioBusKey, int32 InFrames, int32 NumChannels, float InGain = 1.f);
	CRIWARECORE_API Audio::FPatchOutputStrongPtr AddPatchOutputForSoundAndAudioBus(uint64 SoundInstanceID, Atom::FAudioBusKey AudioBusKey, int32 InFrames, int32 NumChannels, float InGain = 1.f);
	CRIWARECORE_API void ConnectPatches(uint64 SoundInstanceID);
	CRIWARECORE_API void RemoveSound(uint64 SoundInstanceID);

	CRIWARECORE_API void InitDefaultAudioBuses();
	CRIWARECORE_API void ShutdownDefaultAudioBuses();

private:

	struct FActiveBusData
	{
		Atom::FAudioBusKey BusKey = 0;
		int32 NumChannels = 0;
		bool bIsAutomatic = false;
	};

	TArray<TStrongObjectPtr<UAtomAudioBus>> DefaultAudioBuses; 
	// The active audio bus list accessible on the game thread
	TMap<Atom::FAudioBusKey, FActiveBusData> ActiveAudioBuses_GameThread;

	struct FPendingConnection
	{
		using FPatchVariant = TVariant<Audio::FPatchInput, Audio::FPatchOutputStrongPtr>;
		FPatchVariant PatchVariant;
		Atom::FAudioBusKey AudioBusKey;
		int32 BlockSizeFrames = 0;
		int32 NumChannels = 0;
		bool bIsAutomatic = false;
	};

	void AddPendingConnection(uint64 SoundInstanceID, FPendingConnection&& PendingConnection);

	struct FSoundInstanceConnections
	{
		TArray<FPendingConnection> PendingConnections;
	};

	TArray<FPendingConnection> ExtractPendingConnectionsIfReady(uint64 SoundInstanceID);

	TMap<uint64, FSoundInstanceConnections> SoundInstanceConnectionMap;
	FCriticalSection Mutex;
};
