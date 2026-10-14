
#include "NiagaraDataInterfaceAtomPlayer.h"

#include "NiagaraTypes.h"
#include "NiagaraCustomVersion.h"
//#include "NiagaraStats.h"
#include "Internationalization/Internationalization.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraWorldManager.h"

#include "CriWareLocalization.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomAttenuation.h"
#include "Atom/AtomConcurrency.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/AtomGameplayStatics.h"

#define LOCTEXT_NAMESPACE "AtomNiagara"

DECLARE_CYCLE_STAT(TEXT("Atom DI update persistent sound"), STAT_NiagaraAtomDIUpdateSound, STATGROUP_Atom);
DECLARE_CYCLE_STAT(TEXT("Atom DI create persistent sound"), STAT_NiagaraAtomDICreateSound, STATGROUP_Atom);
DECLARE_CYCLE_STAT(TEXT("Atom DI stop persistent sound"), STAT_NiagaraAtomDIStopSound, STATGROUP_Atom);

const FName UNiagaraDataInterfaceAtomPlayer::PlayAtomName(TEXT("PlayAtomAtLocation"));
const FName UNiagaraDataInterfaceAtomPlayer::PlayPersistentAtomName(TEXT("PlayPersistentAtom"));
const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAtomVolumeName(TEXT("UpdateAtomVolume"));
const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAtomPitchName(TEXT("UpdateAtomPitch"));
const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAtomLocationName(TEXT("UpdateAtomLocation"));
const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAtomRotationName(TEXT("UpdateAtomRotation"));
//const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAudioBoolParamName(TEXT("SetBooleanParameter"));
//const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAudioIntegerParamName(TEXT("SetIntegerParameter"));
//const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAudioFloatParamName(TEXT("SetFloatParameter"));
const FName UNiagaraDataInterfaceAtomPlayer::SetPersistentAtomAisacControlValueName(TEXT("SetAisacControlValue"));
const FName UNiagaraDataInterfaceAtomPlayer::PausePersistentAtomName(TEXT("SetPaused"));

struct FNiagaraAtomPlayerDIFunctionVersion
{
	enum Type
	{
		InitialVersion = 0,
		LWCConversion = 1,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};

/**
 * Async task to play the audio on the game thread and isolate from the niagara tick
 */
class FNiagaraAtomPlayerAsyncTask
{
	TWeakObjectPtr<UAtomSoundBase> WeakSound;
	TWeakObjectPtr<UAtomAttenuation> WeakAttenuation;
	TWeakObjectPtr<UAtomConcurrency> WeakConcurrency;
	TArray<FAtomParticleData> Data;
	TWeakObjectPtr<UWorld> WeakWorld;

public:
	FNiagaraAtomPlayerAsyncTask(TWeakObjectPtr<UAtomSoundBase> InSound, TWeakObjectPtr<UAtomAttenuation> InAttenuation, TWeakObjectPtr<UAtomConcurrency> InConcurrency, TArray<FAtomParticleData>& Data, TWeakObjectPtr<UWorld> InWorld)
		: WeakSound(InSound)
	    , WeakAttenuation(InAttenuation)
		, WeakConcurrency(InConcurrency)
		, Data(Data)
		, WeakWorld(InWorld)
	{
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FNiagaraAtomPlayerAsyncTask, STATGROUP_TaskGraphTasks); }
	static FORCEINLINE ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
	static FORCEINLINE ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::FireAndForget; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		UWorld* World = WeakWorld.Get();
		if (World == nullptr)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Invalid world reference in Atom player DI, skipping play"));
			return;
		}
		
		UAtomSoundBase* Sound = WeakSound.Get();
		if (Sound == nullptr)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Invalid sound reference in Atom player DI, skipping play"));
			return;
		}

		for (const FAtomParticleData& ParticleData : Data)
		{
			UAtomGameplayStatics::PlaySoundAtLocation(World, Sound, ParticleData.Position, ParticleData.Rotation, ParticleData.Volume,
				ParticleData.Pitch, ParticleData.StartTime, WeakAttenuation.Get(), WeakConcurrency.Get());
		}
	}
};

UNiagaraDataInterfaceAtomPlayer::UNiagaraDataInterfaceAtomPlayer(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SoundToPlay = nullptr;
	Attenuation = nullptr;
	Concurrency = nullptr;
	bLimitPlaysPerTick = true;
	MaxPlaysPerTick = 10;
}

#if WITH_EDITORONLY_DATA
bool UNiagaraDataInterfaceAtomPlayer::UpgradeFunctionCall(FNiagaraFunctionSignature& FunctionSignature)
{
	// always upgrade to the latest version
	if (FunctionSignature.FunctionVersion < FNiagaraAtomPlayerDIFunctionVersion::LatestVersion)
	{
		TArray<FNiagaraFunctionSignature> AllFunctions;
		GetFunctions(AllFunctions);
		for (const FNiagaraFunctionSignature& Sig : AllFunctions)
		{
			if (FunctionSignature.Name == Sig.Name)
			{
				FunctionSignature = Sig;
				return true;
			}
		}
	}

	return false;
}
#endif

void UNiagaraDataInterfaceAtomPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
	}
}

bool UNiagaraDataInterfaceAtomPlayer::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FAtomPlayerInterface_InstanceData* PIData = new (PerInstanceData) FAtomPlayerInterface_InstanceData;
	PIData->LWCConverter = SystemInstance->GetLWCConverter();
	if (bLimitPlaysPerTick)
	{
		PIData->MaxPlaysPerTick = MaxPlaysPerTick;
	}
	PIData->bStopWhenComponentIsDestroyed = bStopWhenComponentIsDestroyed;
#if WITH_EDITORONLY_DATA
	PIData->bOnlyActiveDuringGameplay = bOnlyActiveDuringGameplay;
#endif
	return true;
}

void UNiagaraDataInterfaceAtomPlayer::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FAtomPlayerInterface_InstanceData* InstData = (FAtomPlayerInterface_InstanceData*)PerInstanceData;
	for (const auto& Entry : InstData->PersistentAtomMapping)
	{
		if (Entry.Value.IsValid())
		{
			Entry.Value->Stop();
		}
	}
	InstData->~FAtomPlayerInterface_InstanceData();
}

bool UNiagaraDataInterfaceAtomPlayer::PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
	FAtomPlayerInterface_InstanceData* PIData = (FAtomPlayerInterface_InstanceData*)PerInstanceData;
	if (!PIData)
	{
		return true;
	}
	
	if (IsValid(SoundToPlay) && SystemInstance)
	{
		PIData->SoundToPlay = SoundToPlay;
		PIData->Attenuation = Attenuation;
		PIData->Concurrency = Concurrency;
		PIData->bValidOneShotSound = SoundToPlay->IsLooping() ? bAllowLoopingOneShotSounds : true;
		for (int PatchIndex = 0; PatchIndex < AdditionalAisacPatches.Num(); PatchIndex++)
		{
			PIData->AdditionalAisacPatches.Add(AdditionalAisacPatches[PatchIndex]);
		}
	}
	else
	{
		PIData->SoundToPlay.Reset();
		PIData->Attenuation.Reset();
		PIData->Concurrency.Reset();
		PIData->bValidOneShotSound = false;
		PIData->AdditionalAisacPatches.Reset();
	}

	PIData->AisacControls = AisacControls;
	return false;
}

bool UNiagaraDataInterfaceAtomPlayer::PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds)
{
	FAtomPlayerInterface_InstanceData* PIData = (FAtomPlayerInterface_InstanceData*) PerInstanceData;
	UNiagaraSystem* System = SystemInstance->GetSystem();

	const FNiagaraWorldManager* NiagaraWorldManager = SystemInstance->GetWorldManager();
	UWorld* World = NiagaraWorldManager->GetWorld();

#if WITH_EDITORONLY_DATA
	if (World->HasBegunPlay() == false && PIData->bOnlyActiveDuringGameplay)
	{
		PIData->PlayAtomQueue.Empty();
		PIData->PersistentAtomMapping.Empty();
		return false;
	}
#endif
	
	if (!PIData->PlayAtomQueue.IsEmpty() && System)
	{
		//Drain the queue into an array here
		TArray<FAtomParticleData> Data;
		FAtomParticleData Value;
		while (PIData->PlayAtomQueue.Dequeue(Value))
		{
			Data.Add(Value);
			if (PIData->MaxPlaysPerTick > 0 && Data.Num() >= PIData->MaxPlaysPerTick)
			{
				// discard the rest of the queue if over the tick limit
				PIData->PlayAtomQueue.Empty();
				break;
			}
		}
		
		TGraphTask<FNiagaraAtomPlayerAsyncTask>::CreateTask().ConstructAndDispatchWhenReady(PIData->SoundToPlay, PIData->Attenuation, PIData->Concurrency, Data, World);
	}

	// process the persistent audio updates
	FPersistentAtomParticleData Value;
	TSet<int32> UpdatedAtomHandles;
	while (PIData->PersistentAtomActionQueue.Dequeue(Value))
	{
		UAtomComponent* AtomComponent = nullptr;
		if (Value.AtomHandle > 0)
		{
			auto MappedValue = PIData->PersistentAtomMapping.Find(Value.AtomHandle);
			if (MappedValue && MappedValue->IsValid())
			{
				AtomComponent = MappedValue->Get();
			}
		}
		
		// since we are in the game thread here, it is safe for the callback to access the Atom component
		if (Value.UpdateCallback)
		{
			Value.UpdateCallback(PIData, AtomComponent, SystemInstance);
		}
		UpdatedAtomHandles.Add(Value.AtomHandle);
	}

	// destroy all persistent Atom components that were not updated this frame - this prevents looping sounds to go on after a particle has died. It also stops sounds if an emitter is culled by scalability.
	for (auto Iterator = PIData->PersistentAtomMapping.CreateIterator(); Iterator; ++Iterator)
	{
		if (!UpdatedAtomHandles.Contains(Iterator.Key()))
		{
			SCOPE_CYCLE_COUNTER(STAT_NiagaraAtomDIStopSound);
			TWeakObjectPtr<UAtomComponent> WeakComponent = Iterator.Value();
			UAtomComponent* AtomComponent = WeakComponent.IsValid() ? WeakComponent.Get() : nullptr;
			if (AtomComponent && AtomComponent->IsPlaying())
			{
				AtomComponent->Stop();
			}
			Iterator.RemoveCurrent();
		}
	}
	
	return false;
}

bool UNiagaraDataInterfaceAtomPlayer::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	const UNiagaraDataInterfaceAtomPlayer* OtherPlayer = CastChecked<UNiagaraDataInterfaceAtomPlayer>(Other);
	return OtherPlayer->SoundToPlay == SoundToPlay && OtherPlayer->Attenuation == Attenuation && OtherPlayer->Concurrency == Concurrency && OtherPlayer->bLimitPlaysPerTick == bLimitPlaysPerTick && OtherPlayer->MaxPlaysPerTick == MaxPlaysPerTick;
}

void UNiagaraDataInterfaceAtomPlayer::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig;
	Sig.Name = PlayAtomName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("PlayAtomDIFunctionDescription", "This function plays a sound at the given location after the simulation has ticked.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Play Atom")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("PositionWS")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("RotationWS")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("VolumeFactor")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("PitchFactor")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("StartTime")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Success")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = PlayPersistentAtomName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("PlayPersistentAtomDIFunctionDescription", "This function plays a sound at the given location after the simulation has ticked. The returned handle can be used to control the sound in subsequent ticks.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Play Atom")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Existing Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("Position WS")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Rotation WS")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Volume Factor")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Pitch Factor")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Start Time")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Fade In Time")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Fade Out Time")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	OutFunctions.Add(Sig);

	/*Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAudioBoolParamName;
#if WITH_EDITORONLY_DATA
	Sig.Description = NSLOCTEXT("Niagara", "SetPersistentAudioBoolParamFunctionDescription", "If an active audio effect can be found for the given handle then the given sound cue parameter will be set on it.");
	Sig.FunctionVersion = FNiagaraAudioPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Audio Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Audio Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Parameter Name Index")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Parameter Value")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAudioIntegerParamName;
#if WITH_EDITORONLY_DATA
	Sig.Description = NSLOCTEXT("Niagara", "SetPersistentAudioIntegerParamFunctionDescription", "If an active audio effect can be found for the given handle then the given sound cue parameter will be set on it.");
	Sig.FunctionVersion = FNiagaraAudioPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Audio Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Audio Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Parameter Name Index")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Parameter Value")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAudioFloatParamName;
#if WITH_EDITORONLY_DATA
	Sig.Description = NSLOCTEXT("Niagara", "SetPersistentAudioFloatParamFunctionDescription", "If an active audio effect can be found for the given handle then the given sound cue parameter will be set on it.");
	Sig.FunctionVersion = FNiagaraAudioPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Audio Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Audio Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Parameter Name Index")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Parameter Value")));
	OutFunctions.Add(Sig);*/

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAtomAisacControlValueName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomAisacParamFunctionDescription", "If an active Atom audio effect can be found for the given handle then the given AISAC modulation will be set on it.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("AISAC Control Index")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("AISAC Control Value")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAtomVolumeName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomVolumeFunctionDescription", "If an active Atom audio effect can be found for the given handle then the this will adjusts its volume multiplier.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Volume Multiplier")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAtomPitchName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomPitchFunctionDescription", "If an active Atom audio effect can be found for the given handle then the this will adjusts its pitch multiplier.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Pitch Multiplier")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAtomLocationName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomLocationFunctionDescription", "If an active Atom audio effect can be found for the given handle then the this will adjusts its world position.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetPositionDef(), TEXT("Position WS")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = SetPersistentAtomRotationName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomRotationFunctionDescription", "If an active Atom audio effect can be found for the given handle then the this will adjusts its rotation in the world.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Rotation WS")));
	OutFunctions.Add(Sig);

	Sig = FNiagaraFunctionSignature();
	Sig.Name = PausePersistentAtomName;
#if WITH_EDITORONLY_DATA
	Sig.Description = LOCTEXT("SetPersistentAtomPausedDescription", "If an active Atom audio effect can be found for the given handle then the this will either pause or unpause the effect.");
	Sig.FunctionVersion = FNiagaraAtomPlayerDIFunctionVersion::LatestVersion;
#endif
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.bSupportsGPU = false;
	Sig.bRequiresExecPin = true;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("Atom Interface")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Atom Handle")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Pause Audio")));
	OutFunctions.Add(Sig);
}

DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, PlayOneShotAtom);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, PlayPersistentAtom);
//DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterBool);
//DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterInteger);
//DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterFloat);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetAisacControlValue);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateVolume);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdatePitch);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateLocation);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateRotation);
DEFINE_NDI_DIRECT_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetPausedState);
void UNiagaraDataInterfaceAtomPlayer::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc)
{
	if (BindingInfo.Name == PlayAtomName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, PlayOneShotAtom)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == PlayPersistentAtomName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, PlayPersistentAtom)::Bind(this, OutFunc);
	}
	/*else if (BindingInfo.Name == SetPersistentAudioBoolParamName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterBool)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAudioIntegerParamName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterInteger)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAudioFloatParamName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetParameterFloat)::Bind(this, OutFunc);
	}*/
	else if (BindingInfo.Name == SetPersistentAtomAisacControlValueName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetAisacControlValue)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAtomVolumeName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateVolume)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAtomPitchName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdatePitch)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAtomLocationName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateLocation)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == SetPersistentAtomRotationName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, UpdateRotation)::Bind(this, OutFunc);
	}
	else if (BindingInfo.Name == PausePersistentAtomName)
	{
		NDI_FUNC_BINDER(UNiagaraDataInterfaceAtomPlayer, SetPausedState)::Bind(this, OutFunc);
	}
	else
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Could not find data interface external function in %s. Expected Name: %s  Actual Name: %s"), *GetPathNameSafe(this), *PlayAtomName.ToString(), *BindingInfo.Name.ToString());
	}
}

/*void UNiagaraDataInterfaceAtomPlayer::SetParameterBool(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAudioPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AudioHandleInParam(Context);
	FNDIInputParam<int32> NameIndexParam(Context);
	FNDIInputParam<FNiagaraBool> ValueParam(Context);
	checkfSlow(InstData.Get(), TEXT("Audio player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AudioHandleInParam.GetAndAdvance();
		int32 NameIndex = NameIndexParam.GetAndAdvance();
		bool Value = ValueParam.GetAndAdvance();

		if (Handle > 0 && InstData->ParameterNames.IsValidIndex(NameIndex))
		{
			FName ParameterName = InstData->ParameterNames[NameIndex];
			FPersistentAudioParticleData AudioData;
			AudioData.AudioHandle = Handle;			
			AudioData.UpdateCallback = [ParameterName, Value](FAudioPlayerInterface_InstanceData*, UAudioComponent* AudioComponent, FNiagaraSystemInstance*)
			{
				if (AudioComponent && AudioComponent->IsPlaying())
				{
					AudioComponent->SetBoolParameter(ParameterName, Value);
				}
			};
			InstData->PersistentAudioActionQueue.Enqueue(AudioData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::SetParameterInteger(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAudioPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AudioHandleInParam(Context);
	FNDIInputParam<int32> NameIndexParam(Context);
	FNDIInputParam<int32> ValueParam(Context);
	checkfSlow(InstData.Get(), TEXT("Audio player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AudioHandleInParam.GetAndAdvance();
		int32 NameIndex = NameIndexParam.GetAndAdvance();
		int32 Value = ValueParam.GetAndAdvance();

		if (Handle > 0 && InstData->ParameterNames.IsValidIndex(NameIndex))
		{
			FName ParameterName = InstData->ParameterNames[NameIndex];
			FPersistentAudioParticleData AudioData;
			AudioData.AudioHandle = Handle;			
			AudioData.UpdateCallback = [ParameterName, Value](FAudioPlayerInterface_InstanceData*, UAudioComponent* AudioComponent, FNiagaraSystemInstance*)
			{
				if (AudioComponent && AudioComponent->IsPlaying())
				{
					AudioComponent->SetIntParameter(ParameterName, Value);
				}
			};
			InstData->PersistentAudioActionQueue.Enqueue(AudioData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::SetParameterFloat(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAudioPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AudioHandleInParam(Context);
	FNDIInputParam<int32> NameIndexParam(Context);
	FNDIInputParam<float> ValueParam(Context);
	checkfSlow(InstData.Get(), TEXT("Audio player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AudioHandleInParam.GetAndAdvance();
		int32 NameIndex = NameIndexParam.GetAndAdvance();
		float Value = ValueParam.GetAndAdvance();

		if (Handle > 0 && InstData->ParameterNames.IsValidIndex(NameIndex))
		{
			FName ParameterName = InstData->ParameterNames[NameIndex];
			FPersistentAudioParticleData AtomData;
			AtomData.AudioHandle = Handle;
			AtomData.UpdateCallback = [ParameterName, Value](FAudioPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					AtomComponent->SetFloatParameter(ParameterName, Value);
				}
			};
			InstData->PersistentAudioActionQueue.Enqueue(AtomData);
		}
	}
}*/

void UNiagaraDataInterfaceAtomPlayer::SetAisacControlValue(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<int32> AisacControlIndexParam(Context);
	FNDIInputParam<float> AisacControlValueParam(Context);
	checkfSlow(InstData.Get(), TEXT("Audio player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		int32 ControlIndex = AisacControlIndexParam.GetAndAdvance();
		float Value = AisacControlValueParam.GetAndAdvance();

		if (Handle > 0 && InstData->AisacControls.IsValidIndex(ControlIndex))
		{
			FAtomAisacControlSettings Control = InstData->AisacControls[ControlIndex];
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Control, Value](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					AtomComponent->SetAisacControlValue(Control.Control, Value);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::UpdateVolume(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<float> VolumeParam(Context);
	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		float Volume = VolumeParam.GetAndAdvance();

		if (Handle > 0)
		{
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Volume](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					AtomComponent->SetVolumeMultiplier(Volume);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::UpdatePitch(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<float> PitchParam(Context);
	checkfSlow(InstData.Get(), TEXT("Audio player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		float Pitch = PitchParam.GetAndAdvance();

		if (Handle > 0)
		{
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Pitch](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					AtomComponent->SetPitchMultiplier(Pitch);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::UpdateLocation(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<FVector3f> LocationParam(Context);
	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		FVector Location = InstData->LWCConverter.ConvertSimulationVectorToWorld(LocationParam.GetAndAdvance());

		if (Handle > 0)
		{
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Location](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					AtomComponent->SetWorldLocation(Location);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::UpdateRotation(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<FVector3f> RotationParam(Context);
	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		FVector3f Rotation = RotationParam.GetAndAdvance();

		if (Handle > 0)
		{
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Rotation](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					FRotator NewRotator(Rotation.X, Rotation.Y, Rotation.Z);
					AtomComponent->SetWorldRotation(NewRotator);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::SetPausedState(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<FNiagaraBool> PausedParam(Context);
	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		bool IsPaused = PausedParam.GetAndAdvance();

		if (Handle > 0)
		{
			FPersistentAtomParticleData AtomData;
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [IsPaused](FAtomPlayerInterface_InstanceData*, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				if (AtomComponent)
				{
					AtomComponent->SetPaused(IsPaused);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
	}
}

void UNiagaraDataInterfaceAtomPlayer::PlayOneShotAtom(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	VectorVM::FExternalFuncInputHandler<FNiagaraBool> PlayDataParam(Context);

	VectorVM::FExternalFuncInputHandler<float> PositionParamX(Context);
	VectorVM::FExternalFuncInputHandler<float> PositionParamY(Context);
	VectorVM::FExternalFuncInputHandler<float> PositionParamZ(Context);
	
	VectorVM::FExternalFuncInputHandler<float> RotationParamX(Context);
	VectorVM::FExternalFuncInputHandler<float> RotationParamY(Context);
	VectorVM::FExternalFuncInputHandler<float> RotationParamZ(Context);
	
	VectorVM::FExternalFuncInputHandler<float> VolumeParam(Context);
	VectorVM::FExternalFuncInputHandler<float> PitchParam(Context);
	VectorVM::FExternalFuncInputHandler<float> StartTimeParam(Context);

	VectorVM::FExternalFuncRegisterHandler<FNiagaraBool> OutSample(Context);

	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());
	bool ValidSoundData = InstData->SoundToPlay.IsValid() && InstData->bValidOneShotSound;

#if WITH_EDITOR
	if (InstData->SoundToPlay.IsValid() && !InstData->bValidOneShotSound)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Suppressing Niagara one-shot Atom sound %s, because it's a looping sound. Use the 'play persistent audio' module instead. Source: %s"), *InstData->SoundToPlay->GetPathName(), *GetPathNameSafe(this));
	}
#endif

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		FNiagaraBool ShouldPlay = PlayDataParam.GetAndAdvance();
		FAtomParticleData Data;
		FNiagaraPosition SimulationPosition(PositionParamX.GetAndAdvance(), PositionParamY.GetAndAdvance(), PositionParamZ.GetAndAdvance());
		Data.Position = InstData->LWCConverter.ConvertSimulationPositionToWorld(SimulationPosition);
		Data.Rotation = FRotator(RotationParamX.GetAndAdvance(), RotationParamY.GetAndAdvance(), RotationParamZ.GetAndAdvance());
		Data.Volume = VolumeParam.GetAndAdvance();
		Data.Pitch = PitchParam.GetAndAdvance();
		Data.StartTime = StartTimeParam.GetAndAdvance();

		FNiagaraBool Valid;
		if (ValidSoundData && ShouldPlay)
		{
			Valid.SetValue(InstData->PlayAtomQueue.Enqueue(Data));
		}
		*OutSample.GetDestAndAdvance() = Valid;
	}
}

void UNiagaraDataInterfaceAtomPlayer::PlayPersistentAtom(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FAtomPlayerInterface_InstanceData> InstData(Context);

	FNDIInputParam<FNiagaraBool> PlayAtomParam(Context);
	FNDIInputParam<int32> AtomHandleInParam(Context);
	FNDIInputParam<FVector3f> PositionParam(Context);
	FNDIInputParam<FVector3f> RotationParam(Context);
	FNDIInputParam<float> VolumeParam(Context);
	FNDIInputParam<float> PitchParam(Context);
	FNDIInputParam<float> StartTimeParam(Context);
	FNDIInputParam<float> FadeInParam(Context);
	FNDIInputParam<float> FadeOutParam(Context);

	FNDIOutputParam<int32> AtomHandleOutParam(Context);

	checkfSlow(InstData.Get(), TEXT("Atom player interface has invalid instance data. %s"), *GetPathName());

	for (int32 i = 0; i < Context.GetNumInstances(); ++i)
	{
		bool ShouldPlay = PlayAtomParam.GetAndAdvance();
		int32 Handle = AtomHandleInParam.GetAndAdvance();
		FVector Position = InstData->LWCConverter.ConvertSimulationVectorToWorld(PositionParam.GetAndAdvance());
		FVector3f InRot = RotationParam.GetAndAdvance();
		FRotator Rotation = FRotator(InRot.X, InRot.Y, InRot.Z);
		float Volume = VolumeParam.GetAndAdvance();
		float Pitch = PitchParam.GetAndAdvance();
		float StartTime = StartTimeParam.GetAndAdvance();
		float FadeIn = FadeInParam.GetAndAdvance();
		float FadeOut = FadeOutParam.GetAndAdvance();

		FPersistentAtomParticleData AtomData;
		if (ShouldPlay)
		{
			if (Handle <= 0)
			{
				// play a new sound
				Handle = InstData->HandleCount.Increment();
				AtomData.AtomHandle = Handle;
				AtomData.UpdateCallback = [Handle, Position, Rotation, Volume, Pitch, StartTime, FadeIn](FAtomPlayerInterface_InstanceData* InstanceData, UAtomComponent*, FNiagaraSystemInstance* SystemInstance)
				{
					SCOPE_CYCLE_COUNTER(STAT_NiagaraAtomDICreateSound);
					USceneComponent* NiagaraComponent = SystemInstance->GetAttachComponent();
					TWeakObjectPtr<UAtomSoundBase> Sound = InstanceData->SoundToPlay;
					if (NiagaraComponent && Sound.IsValid())
					{
						bool bStopWithEffect = InstanceData->bStopWhenComponentIsDestroyed || Sound->IsLooping(); // we don't allow looping effects to outlive us because then they keep playing forever
						UAtomComponent* AtomComponent = UAtomGameplayStatics::SpawnSoundAttached(Sound.Get(), NiagaraComponent, NAME_None, Position, Rotation, EAttachLocation::KeepWorldPosition, bStopWithEffect, Volume, Pitch, StartTime, InstanceData->Attenuation.Get(), InstanceData->Concurrency.Get(), true);
						if (AtomComponent == nullptr)
						{
							// looks like audio is disabled, so we'll skip adding a mapping
							return;
						}
						if (FadeIn > 0.0)
						{
							AtomComponent->FadeIn(FadeIn, Volume, StartTime);
						}
						if (InstanceData->AdditionalAisacPatches.Num() > 0)
						{
							for (auto& AisacPatch : InstanceData->AdditionalAisacPatches)
							{
								if (auto Patch = AisacPatch.Get())
								{
									AtomComponent->ModulationRouting.AisacModulations.AdditionalAisacPatches.Add(Patch);
								}
							}
						}
						if (InstanceData->AisacControls.Num() > 0)
						{
							AtomComponent->ModulationRouting.AisacModulations.AisacControlModulations = InstanceData->AisacControls;
						}
						InstanceData->PersistentAtomMapping.Add(Handle, AtomComponent);
					}
				};
			}
			else
			{
				// add a dummy entry so the handle keeps playing
				AtomData.AtomHandle = Handle;
			}
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
			AtomHandleOutParam.SetAndAdvance(Handle);
			continue;
		}

		if (Handle > 0)
		{
			// stop sound
			AtomData.AtomHandle = Handle;
			AtomData.UpdateCallback = [Handle, FadeOut](FAtomPlayerInterface_InstanceData* InstanceData, UAtomComponent* AtomComponent, FNiagaraSystemInstance*)
			{
				SCOPE_CYCLE_COUNTER(STAT_NiagaraAtomDIStopSound);
				if (AtomComponent && AtomComponent->IsPlaying())
				{
					if (FadeOut > 0.0)
					{
						AtomComponent->FadeOut(FadeOut, 0);
					}
					else
					{
						AtomComponent->Stop();
					}
					InstanceData->PersistentAtomMapping.Remove(Handle);
				}
			};
			InstData->PersistentAtomActionQueue.Enqueue(AtomData);
		}
		AtomHandleOutParam.SetAndAdvance(0);
	}
}

bool UNiagaraDataInterfaceAtomPlayer::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceAtomPlayer* OtherTyped = CastChecked<UNiagaraDataInterfaceAtomPlayer>(Destination);
	OtherTyped->SoundToPlay = SoundToPlay;
	OtherTyped->Attenuation = Attenuation;
	OtherTyped->Concurrency = Concurrency;
	OtherTyped->bLimitPlaysPerTick = bLimitPlaysPerTick;
	OtherTyped->MaxPlaysPerTick = MaxPlaysPerTick;
	OtherTyped->AdditionalAisacPatches = AdditionalAisacPatches;
	OtherTyped->AisacControls = AisacControls;
	OtherTyped->bStopWhenComponentIsDestroyed = bStopWhenComponentIsDestroyed;
#if WITH_EDITORONLY_DATA
	OtherTyped->bOnlyActiveDuringGameplay = bOnlyActiveDuringGameplay;
#endif
	return true;
}

#undef LOCTEXT_NAMESPACE
