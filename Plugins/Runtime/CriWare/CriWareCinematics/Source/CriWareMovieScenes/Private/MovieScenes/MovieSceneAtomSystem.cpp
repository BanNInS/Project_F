/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareMovieScene
 * File     : MovieSceneAtomSystem.cpp
 *
 ****************************************************************************/

#include "MovieSceneAtomSystem.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "EntitySystem/MovieSceneSequenceInstance.h"
#include "Evaluation/PreAnimatedState/MovieScenePreAnimatedObjectStorage.h"
#include "Evaluation/PreAnimatedState/MovieScenePreAnimatedStateStorage.h"
#include "Evaluation/PreAnimatedState/MovieScenePreAnimatedStorageID.inl"
#include "GameFramework/WorldSettings.h"
#include "IMovieScenePlayer.h"
#include "MovieScene.h"
#include "MovieSceneTracksComponentTypes.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundSimple.h"
#include "Atom/AtomAttenuation.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

#include "MovieSceneAtomSection.h"
#include "MovieSceneAtomTrack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneAtomSystem)

DECLARE_CYCLE_STAT(TEXT("Atom System Evaluate"), MovieSceneEval_AtomTasks, STATGROUP_MovieSceneECS);

static float MaxSequenceAtomDesyncToleranceCVar = 0.5f;
FAutoConsoleVariableRef CVarMaxSequenceAtomDesyncTolerance(
	TEXT("Sequencer.Atom.MaxDesyncTolerance"),
	MaxSequenceAtomDesyncToleranceCVar,
	TEXT("Controls how many seconds an Atom audio track can be out of sync in a Sequence before we attempt a time correction.\n"),
	ECVF_Default);

static bool bIgnoreAtomSyncDuringWorldTimeDilationCVar = true;
FAutoConsoleVariableRef CVarIgnoreAtomSyncDuringWorldTimeDilation(
	TEXT("Sequencer.Atom.IgnoreAudioSyncDuringWorldTimeDilation"),
	bIgnoreAtomSyncDuringWorldTimeDilationCVar,
	TEXT("Ignore correcting audio if there is world time dilation.\n"),
	ECVF_Default);

static int32 UseAtomClockForSequencerDesyncCVar = 0;
FAutoConsoleVariableRef CVaUseAtomClockForSequencerDesync(
	TEXT("Sequencer.Atom.UseAtomClockForAudioDesync"),
	UseAtomClockForSequencerDesyncCVar,
	TEXT("When set to 1, we will use the Atom render thread directly to query whether audio has went out of sync with the sequence.\n"),
	ECVF_Default);

static bool bPlayAtomWhenPlaybackJumps = false;
FAutoConsoleVariableRef CVarPlayAtomWhenPlaybackJumps(
	TEXT("Sequencer.Atom.PlayAudioWhenPlaybackJumps"),
	bPlayAtomWhenPlaybackJumps,
	TEXT("Play audio when playback jumps.\n"),
	ECVF_Default);

namespace UE::MovieScene
{
	enum class EPreAnimatedAtomStateType
	{
		/** Pre-animated state manages the lifespan of the Atom component */
		AtomComponentLifespan,
		/** Pre-animated state manages whether the Atom component is playing */
		AudioPlaying
	};

	template<typename BaseTraits>
	struct FPreAnimatedAtomStateTraits : BaseTraits
	{
		using KeyType = FObjectKey;
		using StorageType = EPreAnimatedAtomStateType;

		EPreAnimatedAtomStateType CachePreAnimatedValue(FObjectKey InKey)
		{
			check(false);
			return EPreAnimatedAtomStateType::AtomComponentLifespan;
		}

		void RestorePreAnimatedValue(FObjectKey InKey, EPreAnimatedAtomStateType InStateType, const FRestoreStateParams& Params)
		{
			if (UAtomComponent* AtomComponent = Cast<UAtomComponent>(InKey.ResolveObjectPtr()))
			{
				switch (InStateType)
				{
				case EPreAnimatedAtomStateType::AudioPlaying:
					AtomComponent->Stop();
					break;
				case EPreAnimatedAtomStateType::AtomComponentLifespan:
					AtomComponent->DestroyComponent();
					break;
				}
			}
		}
	};

	using FPreAnimatedBoundObjectAtomStateTraits = FPreAnimatedAtomStateTraits<FBoundObjectPreAnimatedStateTraits>;

	struct FPreAnimatedAtomStorage : TPreAnimatedStateStorage_ObjectTraits<FPreAnimatedBoundObjectAtomStateTraits>
	{
		static TAutoRegisterPreAnimatedStorageID<FPreAnimatedAtomStorage> StorageID;
	};
	TAutoRegisterPreAnimatedStorageID<FPreAnimatedAtomStorage> FPreAnimatedAtomStorage::StorageID;

	/**
	 * Types of Atom evaluation we should run for a given sequence.
	 */
	enum class EAtomEvaluationType
	{
		Skip,
		Play,
		StopAndPlay,
		Stop
	};

	struct FGatherAtomInputs
	{
		using FInstanceObjectKey = UMovieSceneAtomSystem::FInstanceObjectKey;
		using FAtomInputsBySectionKey = UMovieSceneAtomSystem::FAtomInputsBySectionKey;
		using FAtomComponentInputEvaluationData = UMovieSceneAtomSystem::FAtomComponentInputEvaluationData;

		UMovieSceneAtomSystem* AtomSystem;

		FGatherAtomInputs(UMovieSceneAtomSystem* InAtomSystem)
			: AtomSystem(InAtomSystem)
		{
		}

		void ForEachAllocation(
			const FEntityAllocation* Allocation,
			TRead<FInstanceHandle> InstanceHandles,
			TRead<FMovieSceneAtomComponentData> AtomDatas,
			TRead<FMovieSceneAtomInputData> AtomInputDatas,
			TReadOneOrMoreOf<
			double, double, double,
			double, double, double,
			double, double, double,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7 
			FString, int64, bool>
#else
			FString, int32, bool >
#endif
			AtomInputResults) const
		{
			FAtomInputsBySectionKey& AtomInputsBySectionKey = AtomSystem->AtomInputsBySectionKey;

			const double* DoubleResults[9];
			{
				DoubleResults[0] = AtomInputResults.Get<0>();
				DoubleResults[1] = AtomInputResults.Get<1>();
				DoubleResults[2] = AtomInputResults.Get<2>();
				DoubleResults[3] = AtomInputResults.Get<3>();
				DoubleResults[4] = AtomInputResults.Get<4>();
				DoubleResults[5] = AtomInputResults.Get<5>();
				DoubleResults[6] = AtomInputResults.Get<6>();
				DoubleResults[7] = AtomInputResults.Get<7>();
				DoubleResults[8] = AtomInputResults.Get<8>();
			}
			const FString* StringResults = AtomInputResults.Get<9>();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7 
			const int64* IntegerResults = AtomInputResults.Get<10>();
#else
			const int32* IntegerResults = AtomInputResults.Get<10>();
#endif
			const bool* BoolResults = AtomInputResults.Get<11>();

			for (int32 Index = 0; Index < Allocation->Num(); ++Index)
			{
				const FMovieSceneAtomComponentData& AtomData = AtomDatas[Index];
				const FMovieSceneAtomInputData& AtomInputNames = AtomInputDatas[Index];

				FInstanceObjectKey SectionKey(InstanceHandles[Index], FObjectKey(AtomData.Section));
				FAtomComponentInputEvaluationData& AtomInputValues = AtomInputsBySectionKey.FindOrAdd(SectionKey);

				// Gather float inputs.
				for (int32 FloatIndex = 0; FloatIndex < 9; ++FloatIndex)
				{
					if (!AtomInputNames.FloatInputs[FloatIndex].IsNone() && ensure(DoubleResults[FloatIndex]))
					{
						AtomInputValues.Inputs_Float.Add(AtomInputNames.FloatInputs[FloatIndex], DoubleResults[FloatIndex][Index]);
					}
				}

				// Gather string inputs.
				if (!AtomInputNames.StringInput.IsNone() && ensure(StringResults))
				{
					AtomInputValues.Inputs_String.Add(AtomInputNames.StringInput, StringResults[Index]);
				}

				// Gather integer inputs.
				if (!AtomInputNames.IntInput.IsNone() && ensure(IntegerResults))
				{
					AtomInputValues.Inputs_Int.Add(AtomInputNames.IntInput, IntegerResults[Index]);
				}

				// Gather boolean inputs.
				if (!AtomInputNames.BoolInput.IsNone() && ensure(BoolResults))
				{
					AtomInputValues.Inputs_Bool.Add(AtomInputNames.BoolInput, BoolResults[Index]);
				}
			}
		}
	};

	struct FGatherAtomTriggers
	{
		using FInstanceObjectKey = UMovieSceneAtomSystem::FInstanceObjectKey;
		using FAtomInputsBySectionKey = UMovieSceneAtomSystem::FAtomInputsBySectionKey;
		using FAtomComponentInputEvaluationData = UMovieSceneAtomSystem::FAtomComponentInputEvaluationData;

		UMovieSceneAtomSystem* AtomSystem;

		FGatherAtomTriggers(UMovieSceneAtomSystem* InAtomSystem)
			: AtomSystem(InAtomSystem)
		{
		}

		void ForEachAllocation(
			const FEntityAllocation* Allocation,
			TRead<FInstanceHandle> InstanceHandles,
			TRead<FMovieSceneAtomComponentData> AtomDatas,
			TRead<FName> AtomTriggerNames) const
		{
			FAtomInputsBySectionKey& AtomInputsBySectionKey = AtomSystem->AtomInputsBySectionKey;

			for (int32 Index = 0; Index < Allocation->Num(); ++Index)
			{
				const FMovieSceneAtomComponentData& AtomData = AtomDatas[Index];
				const FName& AtomTriggerName = AtomTriggerNames[Index];

				FInstanceObjectKey SectionKey(InstanceHandles[Index], FObjectKey(AtomData.Section));
				FAtomComponentInputEvaluationData& AtomInputValues = AtomInputsBySectionKey.FindOrAdd(SectionKey);

				AtomInputValues.Inputs_Trigger.Add(AtomTriggerName);
			}
		}
	};

	struct FEvaluateAtom
	{
		static EAtomEvaluationType GetAtomEvaluationType(const FMovieSceneContext& Context)
		{
			if (Context.GetStatus() == EMovieScenePlayerStatus::Jumping &&
				!bPlayAtomWhenPlaybackJumps)
			{
				return EAtomEvaluationType::Skip;
			}

			if (Context.HasJumped())
			{
				// If the status says we jumped, we always stop all sounds, then allow them to be played again 
				// naturally if status == Playing (for example)
				return EAtomEvaluationType::StopAndPlay;
			}

			if (
				(Context.GetStatus() != EMovieScenePlayerStatus::Playing &&
					Context.GetStatus() != EMovieScenePlayerStatus::Scrubbing &&
					Context.GetStatus() != EMovieScenePlayerStatus::Stepping)
				||
				Context.GetDirection() == EPlayDirection::Backwards)
			{
				// stopped, recording, etc
				return EAtomEvaluationType::Stop;
			}

			return EAtomEvaluationType::Play;
		}

		UMovieSceneAtomSystem* AtomSystem;
		const FInstanceRegistry* InstanceRegistry;

		FEvaluateAtom(UMovieSceneAtomSystem* InAtomSystem)
			: AtomSystem(InAtomSystem)
		{
			InstanceRegistry = AtomSystem->GetLinker()->GetInstanceRegistry();
		}

		void ForEachAllocation(
			const FEntityAllocation* Allocation,
			TRead<FMovieSceneEntityID> EntityIDs,
			TRead<FRootInstanceHandle> RootInstanceHandles,
			TRead<FInstanceHandle> InstanceHandles,
			TRead<FMovieSceneAtomComponentData> AtomDatas,
			TRead<double> VolumeMultipliers,
			TRead<double> PitchMultipliers,
			TReadOptional<UObject*> BoundObjects) const
		{
			const FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();

			const int32 Num = Allocation->Num();
			const bool bWantsRestoreState = Allocation->HasComponent(BuiltInComponents->Tags.RestoreState);

			for (int32 Index = 0; Index < Num; ++Index)
			{
				const FMovieSceneEntityID& EntityID = EntityIDs[Index];
				const FRootInstanceHandle& RootInstanceHandle = RootInstanceHandles[Index];
				const FInstanceHandle& InstanceHandle = InstanceHandles[Index];
				const FMovieSceneAtomComponentData& AtomData = AtomDatas[Index];

				const FSequenceInstance& Instance = InstanceRegistry->GetInstance(InstanceHandle);

				double VolumeMultiplier = VolumeMultipliers[Index];
				double PitchMultiplier = PitchMultipliers[Index];
				UObject* BoundObject = (BoundObjects.IsValid() ? BoundObjects[Index] : nullptr);

				Evaluate(EntityID, AtomData, Instance, RootInstanceHandle, VolumeMultiplier, PitchMultiplier, BoundObject, bWantsRestoreState);
			}
		}

	private:

		void Evaluate(
			const FMovieSceneEntityID& EntityID,
			const FMovieSceneAtomComponentData& AtomData,
			const FSequenceInstance& Instance,
			const FRootInstanceHandle& RootInstanceHandle,
			double VolumeMultiplier,
			double PitchMultiplier,
			UObject* BoundObject,
			bool bWantsRestoreState) const
		{
			const FMovieSceneContext& Context = Instance.GetContext();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			UObject* PlaybackContext = Instance.GetSharedPlaybackState()->GetPlaybackContext();
#else
			IMovieScenePlayer* Player = Instance.GetPlayer();
			UObject* PlaybackContext = Player->GetPlaybackContext();
#endif

			UMovieSceneAtomSection* AtomSection = AtomData.Section;
			if (!ensureMsgf(AtomSection, TEXT("No valid Atom section found in Atom track component data!")))
			{
				return;
			}

			FInstanceHandle InstanceHandle(Instance.GetInstanceHandle());
			FObjectKey ActorKey(BoundObject);
			FObjectKey SectionKey(AtomSection);

			const EAtomEvaluationType EvalType = GetAtomEvaluationType(Context);
			if (EvalType == EAtomEvaluationType::StopAndPlay)
			{
				AtomSystem->StopSound(InstanceHandle, ActorKey, AtomData.Section);
			}
			else if (EvalType == EAtomEvaluationType::Stop)
			{
				AtomSystem->StopSound(InstanceHandle, ActorKey, AtomData.Section);
				return;
			}
			else if (EvalType == EAtomEvaluationType::Skip)
			{
				return;
			}

			// Root Atom track
			if (BoundObject == nullptr)
			{
				const FMovieSceneActorReferenceData& AttachActorData = AtomSection->GetAttachActorData();

				USceneComponent* AttachComponent = nullptr;
				FMovieSceneActorReferenceKey AttachKey;
				AttachActorData.Evaluate(Context.GetTime(), AttachKey);
				FMovieSceneObjectBindingID AttachBindingID = AttachKey.Object;
				if (AttachBindingID.IsValid())
				{
					// If the transform is set, otherwise use the bound actor's transform
					for (TWeakObjectPtr<> WeakObject : AttachBindingID.ResolveBoundObjects(Instance.GetSequenceID()
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
						, Instance.GetSharedPlaybackState()
#else
						, *Player
#endif
					))
					{
						AActor* AttachActor = Cast<AActor>(WeakObject.Get());
						if (AttachActor)
						{
							AttachComponent = AtomSection->GetAttachComponent(AttachActor, AttachKey);
						}
						if (AttachComponent)
						{
							break;
						}
					}
				}

				FAtomComponentEvaluationData* EvaluationData = AtomSystem->GetAtomComponentEvaluationData(InstanceHandle, FObjectKey(), SectionKey);
				if (!EvaluationData)
				{
					// Initialize the sound
					UWorld* World = PlaybackContext ? PlaybackContext->GetWorld() : nullptr;
					EvaluationData = AtomSystem->AddRootAtomComponent(InstanceHandle, AtomSection, World);
					UAtomComponent* AtomComponent = EvaluationData ? EvaluationData->AtomComponent.Get() : nullptr;

					if (ensure(AtomComponent))
					{
						AtomSystem->PreAnimatedStorage->BeginTrackingEntity(EntityID, bWantsRestoreState, RootInstanceHandle, AtomComponent);
						AtomSystem->PreAnimatedStorage->CachePreAnimatedValue(
							FCachePreAnimatedValueParams(), AtomComponent,
							[](FObjectKey InKey) { return EPreAnimatedAtomStateType::AtomComponentLifespan; });

						//if (AtomSection->GetOnQueueSubtitles().IsBound())
						//{
						//	AtomComponent->OnQueueSubtitles = AtomSection->GetOnQueueSubtitles();
						//}
						if (AtomSection->GetOnAtomSoundFinished().IsBound())
						{
							AtomComponent->OnAtomSoundFinished = AtomSection->GetOnAtomSoundFinished();
						}
						if (AtomSection->GetOnAtomSoundPlaybackPercent().IsBound())
						{
							AtomComponent->OnAtomSoundPlaybackPercent = AtomSection->GetOnAtomSoundPlaybackPercent();
						}
					}
				}

				if (EvaluationData)
				{
					UAtomComponent* AtomComponent = EvaluationData->AtomComponent.Get();

					if (AtomComponent && AttachComponent &&
						(AtomComponent->GetAttachParent() != AttachComponent || AtomComponent->GetAttachSocketName() != AttachKey.SocketName))
					{
						AtomComponent->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachKey.SocketName);
					}

					EvaluationData->VolumeMultiplier = VolumeMultiplier * AtomSection->EvaluateEasing(Context.GetTime());
					EvaluationData->PitchMultiplier = PitchMultiplier;

					//EnsureAudioIsPlaying(nullptr, *AtomSection, *EvaluationData, Context, *Player);
					EnsureAudioIsPlaying(nullptr, InstanceHandle, *AtomSection, *EvaluationData, Context, PlaybackContext);
				}
			}

			// Object binding audio track
			else
			{
				FAtomComponentEvaluationData* EvaluationData = AtomSystem->GetAtomComponentEvaluationData(InstanceHandle, ActorKey, SectionKey);
				if (!EvaluationData)
				{
					// Initialize the sound
					EvaluationData = AtomSystem->AddBoundObjectAtomComponent(InstanceHandle, AtomSection, BoundObject);
					UAtomComponent* AtomComponent = EvaluationData ? EvaluationData->AtomComponent.Get() : nullptr;

					if (AtomComponent)
					{
						AtomSystem->PreAnimatedStorage->BeginTrackingEntity(EntityID, bWantsRestoreState, RootInstanceHandle, AtomComponent);
						AtomSystem->PreAnimatedStorage->CachePreAnimatedValue(
							FCachePreAnimatedValueParams(), AtomComponent,
							[](FObjectKey InKey) { return EPreAnimatedAtomStateType::AtomComponentLifespan; });

						//if (AtomSection->GetOnQueueSubtitles().IsBound())
						//{
						//	AtomComponent->OnQueueSubtitles = AtomSection->GetOnQueueSubtitles();
						//}
						if (AtomSection->GetOnAtomSoundFinished().IsBound())
						{
							AtomComponent->OnAtomSoundFinished = AtomSection->GetOnAtomSoundFinished();
						}
						if (AtomSection->GetOnAtomSoundPlaybackPercent().IsBound())
						{
							AtomComponent->OnAtomSoundPlaybackPercent = AtomSection->GetOnAtomSoundPlaybackPercent();
						}
					}
				}

				if (EvaluationData)
				{
					EvaluationData->VolumeMultiplier = VolumeMultiplier;
					EvaluationData->PitchMultiplier = PitchMultiplier;

					EnsureAudioIsPlaying(BoundObject, InstanceHandle, *AtomSection, *EvaluationData, Context, PlaybackContext);
				}
			}
		}

		void EnsureAudioIsPlaying(
			UObject* BoundObject,
			FInstanceHandle InstanceHandle,
			UMovieSceneAtomSection& AtomSection,
			FAtomComponentEvaluationData& EvaluationData,
			const FMovieSceneContext& Context,
			UObject* PlaybackContext) const
		{
			using FAtomInputsBySectionKey = UMovieSceneAtomSystem::FAtomInputsBySectionKey;
			using FAtomComponentInputEvaluationData = UMovieSceneAtomSystem::FAtomComponentInputEvaluationData;

			ensureMsgf(EvaluationData.AtomComponent.IsValid(), TEXT("Trying to evaluate audio track on an invalid audio component"));
			UAtomComponent& AtomComponent = *EvaluationData.AtomComponent.Get();

			AtomSystem->PreAnimatedStorage->CachePreAnimatedValue(
				FCachePreAnimatedValueParams(), &AtomComponent,
				[](FObjectKey InKey) { return EPreAnimatedAtomStateType::AudioPlaying; });

			if (AtomComponent.VolumeMultiplier != EvaluationData.VolumeMultiplier)
			{
				AtomComponent.SetVolumeMultiplier(EvaluationData.VolumeMultiplier);
			}

			if (AtomComponent.PitchMultiplier != EvaluationData.PitchMultiplier)
			{
				AtomComponent.SetPitchMultiplier(EvaluationData.PitchMultiplier);
			}

			//AtomComponent.bSuppressSubtitles = AtomSection.GetSuppressSubtitles();

			// Allow spatialization if we have any object we've been attached to.
			const bool bAllowSpatialization = (BoundObject != nullptr || AtomComponent.GetAttachParent() != nullptr);

			// Apply the input params.
			/*FAtomInputsBySectionKey& AtomInputsBySectionKey = AtomSystem->AtomInputsBySectionKey;
			FObjectKey SectionKey(&AtomSection);
			FAtomComponentInputEvaluationData* AtomInputs = AtomInputsBySectionKey.Find(SectionKey);
			if (AtomInputs)
			{
				SetAtomInputParameters(AtomInputs->Inputs_Float, AtomComponent);
				SetAtomInputParameters(AtomInputs->Inputs_String, AtomComponent);
				SetAtomInputParameters(AtomInputs->Inputs_Bool, AtomComponent);
				SetAtomInputParameters(AtomInputs->Inputs_Int, AtomComponent);
			}*/

			// Apply the AISAC params.
			TArray<FAisacControlAndCurve> AisacControlsAndCurves = AtomSection.GetAisacControlsAndCurves();
			for (auto& AisacControlAndCurve : AisacControlsAndCurves)
			{
				float AisacValue = 0.0f;
				AisacControlAndCurve.AisacValueCurve.Evaluate(Context.GetTime(), AisacValue);

				AtomComponent.SetAisacControlValue(AisacControlAndCurve.AisacControl, AisacValue);
			}

			FFrameNumber SectionStartFrame = (AtomSection.HasStartFrame() ? AtomSection.GetInclusiveStartFrame() : 0);
			float SectionStartTimeSeconds = SectionStartFrame / AtomSection.GetTypedOuter<UMovieScene>()->GetTickResolution();

			const FFrameNumber AudioStartOffset = AtomSection.GetStartOffset();
			UAtomSoundBase* Sound = AtomSection.GetSound();

			float AudioTime = (Context.GetTime() / Context.GetFrameRate())
				- SectionStartTimeSeconds
				+ (float)Context.GetFrameRate().AsSeconds(AudioStartOffset);
			if (AudioTime >= 0.f && Sound)
			{
				const float Duration = Sound ? FMath::Max(0.0f, Sound->GetDuration()) : 0.0f;

				if (!AtomSection.GetLooping() && AudioTime > Duration && Duration != 0.f)
				{
					AtomComponent.Stop();
					return;
				}

				AudioTime = Duration > 0.f ? FMath::Fmod(AudioTime, Duration) : AudioTime;
			}

			// If the Atom component is not playing we (may) need a state change. If the Atom component is playing
			// the wrong sound then we need a state change. If the audio playback time is significantly out of sync 
			// with the desired time then we need a state change.
			const bool bSoundsNeedPlaying = !AtomComponent.IsPlaying();
			const bool bSoundNeedsStateChange = AtomComponent.Sound != Sound;
			bool bSoundNeedsTimeSync = false;

			UWorld* World = PlaybackContext ? PlaybackContext->GetWorld() : nullptr;

			// Sync only if there is no time dilation because otherwise the system will constantly resync because audio 
			// playback is not dilated and will never match the expected playback time.
			const bool bDoTimeSync =
				World && World->GetWorldSettings() &&
				(FMath::IsNearlyEqual(World->GetWorldSettings()->GetEffectiveTimeDilation(), 1.f) ||
					!bIgnoreAtomSyncDuringWorldTimeDilationCVar);

			if (bDoTimeSync)
			{
				float CurrentGameTime = 0.0f;

				FAtomRuntime* AtomRuntime = World ? FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World) : nullptr;
				if (UseAtomClockForSequencerDesyncCVar && AtomRuntime)
				{
					CurrentGameTime = AtomRuntime->GetAtomClock();
				}
				else
				{
					CurrentGameTime = World ? World->GetAudioTimeSeconds() : 0.f;
				}

				// This tells us how much time has passed in the game world (and thus, reasonably, the audio playback)
				// so if we calculate that we should be playing say, 15s into the section during evaluation, but
				// we're only 5s since the last Play call, then we know we're out of sync. 
				if (EvaluationData.PartialDesyncComputation.IsSet())
				{
					const float PartialDesyncComputation = EvaluationData.PartialDesyncComputation.GetValue();
					float Desync = PartialDesyncComputation + AudioTime - CurrentGameTime;

					if (!FMath::IsNearlyZero(MaxSequenceAtomDesyncToleranceCVar) && FMath::Abs(Desync) > MaxSequenceAtomDesyncToleranceCVar)
					{
						UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component detected a significant mismatch in (assumed) playback time versus the desired time. Desync: %6.2f(s) Desired Time: %6.2f(s). Component: %s sound: %s"), Desync, AudioTime, *AtomComponent.GetName(), *GetNameSafe(AtomComponent.Sound));
						bSoundNeedsTimeSync = true;
					}
				}
			}

			if (bSoundsNeedPlaying || bSoundNeedsStateChange || bSoundNeedsTimeSync)
			{
#if !NO_LOGGING
				FString ReasonMessage;
				if (bSoundsNeedPlaying)
				{
					ReasonMessage += TEXT("playing");
				}
				else if (bSoundNeedsStateChange)
				{
					ReasonMessage += TEXT("state change");
				}
				else
				{
					ReasonMessage += TEXT("time sync");
				}
				UE_LOG(LogMovieScene, Verbose, TEXT("Atom component needs %s. Component: %s"), *ReasonMessage, *AtomComponent.GetName());
#endif
				AtomComponent.bAllowSpatialization = bAllowSpatialization;
				AtomComponent.bOverrideAttenuation = AtomSection.GetOverrideAttenuation();
				AtomComponent.AttenuationSettings = AtomSection.GetAttenuationSettings();
				AtomComponent.AttenuationOverrides = AtomSection.GetAttenuationOverrides();

				// Apply extension settings for AtomComponent if available.
				if (AtomSection.IsApplyingExtensionSettings())
				{
					if (auto AtomComponentAsExtension = Cast<IAtomComponentExtension>(&AtomComponent))
					{
						AtomComponentAsExtension->ApplyExtensionSettings(AtomSection.GetAtomComponentExtensionSettings());
					}
				}

				// Only call stop on the sound if it is actually playing. This prevents spamming
				// stop calls when a sound cue with a duration of zero is played.
				if (AtomComponent.IsPlaying() || bSoundNeedsTimeSync)
				{
					UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component stopped due to needing a state change bIsPlaying: %d bNeedsTimeSync: %d. Component: %s sound: %s"), AtomComponent.IsPlaying(), bSoundNeedsTimeSync, *AtomComponent.GetName(), *GetNameSafe(AtomComponent.Sound));
					AtomComponent.Stop();
				}

				// Only change the sound clip if it has actually changed. This calls Stop internally if needed.
				if (AtomComponent.Sound != Sound)
				{
					UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component calling SetSound due to new sound. Component: %s OldSound: %s NewSound: %s"), *AtomComponent.GetName(), *GetNameSafe(AtomComponent.Sound), *GetNameSafe(Sound));
					AtomComponent.SetSound(Sound);
				}
#if WITH_EDITOR
				if (GIsEditor && World != nullptr && !World->IsPlayInEditor())
				{
					AtomComponent.bIsUISound = true;
					AtomComponent.bIsPreviewSound = true;
				}
				else
#endif // WITH_EDITOR
				{
					AtomComponent.bIsUISound = false;
				}

				if (AudioTime >= 0.f)
				{
					AtomComponent.CueSelectors = AtomSection.GetSelectorLabels();
					AtomComponent.CueFirstBlockIndex = AtomSection.GetBlockIndex();

					UE_LOG(LogMovieScene, Verbose, TEXT("Audio Component Play at Local Time: %6.2f CurrentTime: %6.2f(s) SectionStart: %6.2f(s), SoundDur: %6.2f OffsetIntoClip: %6.2f sound: %s"), AudioTime, (Context.GetTime() / Context.GetFrameRate()), SectionStartTimeSeconds, AtomComponent.Sound ? AtomComponent.Sound->GetDuration() : 0.0f, (float)Context.GetFrameRate().AsSeconds(AudioStartOffset), *GetNameSafe(AtomComponent.Sound));
					AtomComponent.Play(AudioTime);

					// Keep track of when we asked this audio clip to play (in game time) so that we can figure 
					// out if there's a significant desync in the future.
					//
					// The goal is later to compare:
					//   (NewAudioTime - PreviousAudioTime) and 
					//   (NewGameTime - PreviousGameTime)
					//
					// If their difference is larger than some threshold, we have a desync. NewGameTime and 
					// NewAudioTime will be known next update, but PreviousGameTime and PreviousAudioTime
					// are known now. Let's store (-PreviousAudioTime + PreviousGameTime), and we will only 
					// need to add (NewAudioTime - NewGameTime).
					if (World)
					{
						FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);
						if (UseAtomClockForSequencerDesyncCVar && AtomRuntime)
						{
							EvaluationData.PartialDesyncComputation = AtomRuntime->GetAtomClock() - AudioTime;
						}
						else
						{
							EvaluationData.PartialDesyncComputation = World->GetAudioTimeSeconds() - AudioTime;
						}
					}
				}
			}

			if (Context.GetStatus() != EMovieScenePlayerStatus::Playing)
			{
				float ScrubDuration = AtomTrackConstants::ScrubDuration;
				if (FAtomRuntime* AtomRuntime = AtomComponent.GetAtomRuntime())
				{
					constexpr float MinScrubFrameRateFactor = 1.5f;
					float RuntimeDeltaTime = AtomRuntime->GetGameDeltaTime();

					// When operating at very low frame-rates (<20fps), a single frame will be
					// longer than the hard coded scrub duration of 50ms in which case the delayed
					// stop will trigger on the same frame that the sound starts playing and
					// no audio will be heard. Here we increase the scrub duration to be greater than
					// a single frame if necessary.
					ScrubDuration = FMath::Max(ScrubDuration, RuntimeDeltaTime * MinScrubFrameRateFactor);
				}

				// While scrubbing, play the sound for a short time and then cut it.
				AtomComponent.StopDelayed(ScrubDuration);
			}

			/*if (AtomComponent.IsPlaying() && AtomInputs)
			{
				SetAtomInputTriggers(AtomInputs->Inputs_Trigger, AtomComponent);
			}*/

			if (bAllowSpatialization)
			{
				if (FAtomRuntime* AtomRuntime = AtomComponent.GetAtomRuntime())
				{
					DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.MovieSceneUpdateAtomTransform"), STAT_MovieSceneUpdateAtomTransform, STATGROUP_TaskGraphTasks);
					AtomRuntime->SendCommandToActiveSounds(AtomComponent.GetAtomComponentID(), [ActorTransform = AtomComponent.GetComponentTransform()](FAtomActiveSound& ActiveSound)
						{
							ActiveSound.bLocationDefined = true;
							ActiveSound.Transform = ActorTransform;
						}, GET_STATID(STAT_MovieSceneUpdateAtomTransform));
				}
			}
		}

		// Helper method for firing all triggered audio triggers.
		/*void SetAtomInputTriggers(const TArray<FName>& Inputs, IAtomParameterControllerInterface& InParamaterInterface) const
		{
			for (const FName& TriggerName : Inputs)
			{
				InParamaterInterface.SetTriggerParameter(TriggerName);
			};
		}*/

		// Helper template to set all audio input values previously evaluated.
		/*template<typename ValueType>
		void SetAtomInputParameters(TMap<FName, ValueType>& Inputs, IAtomParameterControllerInterface& InParamaterInterface) const
		{
			for (TPair<FName, ValueType>& Pair : Inputs)
			{
				InParamaterInterface.SetParameter<ValueType>(Pair.Key, MoveTempIfPossible(Pair.Value));
			};
		}*/
	};

} // namespace UE::MovieScene

UMovieSceneAtomSystem::UMovieSceneAtomSystem(const FObjectInitializer& ObjInit)
	: UMovieSceneEntitySystem(ObjInit)
{
	using namespace UE::MovieScene;

	RelevantComponent = FMovieSceneAtomComponentTypes::Get()->Atom;
	Phase = ESystemPhase::Scheduling;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		const FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
		const FMovieSceneAtomComponentTypes* TrackComponents = FMovieSceneAtomComponentTypes::Get();

		// We consume the result of all possible audio input channels.
		for (int32 Index = 0; Index < 9; ++Index)
		{
			DefineComponentConsumer(GetClass(), BuiltInComponents->DoubleResult[Index]);
		}
		DefineComponentConsumer(GetClass(), BuiltInComponents->StringResult);
		DefineComponentConsumer(GetClass(), BuiltInComponents->IntegerResult);
		DefineComponentConsumer(GetClass(), BuiltInComponents->BoolResult);
		DefineComponentConsumer(GetClass(), TrackComponents->AtomTriggerName);
	}
}

void UMovieSceneAtomSystem::OnLink()
{
	using namespace UE::MovieScene;

	PreAnimatedStorage = Linker->PreAnimatedState.GetOrCreateStorage<FPreAnimatedAtomStorage>();
}

void UMovieSceneAtomSystem::OnUnlink()
{
	using namespace UE::MovieScene;

	for (const TPair<FObjectKey, FAtomComponentBySectionKey>& AtomComponentsForActor : AtomComponentsByActorKey)
	{
		for (const TPair<FInstanceObjectKey, FAtomComponentEvaluationData>& AtomComponentForSection : AtomComponentsForActor.Value)
		{
			UAtomComponent* AtomComponent = AtomComponentForSection.Value.AtomComponent.Get();
			if (AtomComponent)
			{
				UObject* Actor = AtomComponentsForActor.Key.ResolveObjectPtr();
				UObject* Section = AtomComponentForSection.Key.Value.ResolveObjectPtr();
				UE_LOG(LogMovieScene, Warning, TEXT("Cleaning Atom component '%s' for section '%s' on actor '%s'"),
					*AtomComponent->GetPathName(),
					Section ? *Section->GetPathName() : TEXT("<null>"),
					Actor ? *Actor->GetPathName() : TEXT("<null>"));
			}
		}
	}

	AtomComponentsByActorKey.Reset();
	AtomInputsBySectionKey.Reset();
}

void UMovieSceneAtomSystem::ResetSharedData()
{
	AtomInputsBySectionKey.Reset();
	for (TPair<FObjectKey, FAtomComponentBySectionKey>& AtomComponentsForActor : AtomComponentsByActorKey)
	{
		for (TPair<FInstanceObjectKey, FAtomComponentEvaluationData>& AtomComponentForSection : AtomComponentsForActor.Value)
		{
			AtomComponentForSection.Value.bEvaluatedThisFrame = false;
		}
	}
}

void UMovieSceneAtomSystem::OnSchedulePersistentTasks(UE::MovieScene::IEntitySystemScheduler* TaskScheduler)
{
	if (!GEngine || !GEngine->UseSound())
	{
		return;
	}

	using namespace UE::MovieScene;

	const FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	const FMovieSceneAtomComponentTypes* TrackComponents = FMovieSceneAtomComponentTypes::Get();

	// Reset shared data.
	FTaskID ResetSharedDataTask = TaskScheduler->AddMemberFunctionTask(FTaskParams(TEXT("Reset Atom Data")), this, &UMovieSceneAtomSystem::ResetSharedData);

	// Gather audio input values computed by the channel evaluators.
	FTaskID GatherInputsTask = FEntityTaskBuilder()
		.Read(BuiltInComponents->InstanceHandle)
		.Read(TrackComponents->Atom)
		.Read(TrackComponents->AtomInputs)
		.ReadOneOrMoreOf(
			BuiltInComponents->DoubleResult[0], BuiltInComponents->DoubleResult[1], BuiltInComponents->DoubleResult[2],
			BuiltInComponents->DoubleResult[3], BuiltInComponents->DoubleResult[4], BuiltInComponents->DoubleResult[5],
			BuiltInComponents->DoubleResult[6], BuiltInComponents->DoubleResult[7], BuiltInComponents->DoubleResult[8],
			BuiltInComponents->StringResult,
			BuiltInComponents->IntegerResult,
			BuiltInComponents->BoolResult)
		.Schedule_PerAllocation<FGatherAtomInputs>(&Linker->EntityManager, TaskScheduler, this);

	TaskScheduler->AddPrerequisite(ResetSharedDataTask, GatherInputsTask);

	// Gather up audio triggers
	FTaskID GatherTriggersTask = FEntityTaskBuilder()
		.Read(BuiltInComponents->InstanceHandle)
		.Read(TrackComponents->Atom)
		.Read(TrackComponents->AtomTriggerName)
		.Schedule_PerAllocation<FGatherAtomTriggers>(&Linker->EntityManager, TaskScheduler, this);

	TaskScheduler->AddPrerequisite(ResetSharedDataTask, GatherTriggersTask);

	// Next, evaluate audio to play and use the gathered Atom input values to set on the Atom components.
	FTaskID EvaluateAudioTask = FEntityTaskBuilder()
		.ReadEntityIDs()
		.Read(BuiltInComponents->RootInstanceHandle)
		.Read(BuiltInComponents->InstanceHandle)
		.Read(TrackComponents->Atom)
		.Read(BuiltInComponents->DoubleResult[0]) // Volume
		.Read(BuiltInComponents->DoubleResult[1]) // Pitch multiplier
		.ReadOptional(BuiltInComponents->BoundObject)
		.SetDesiredThread(Linker->EntityManager.GetGatherThread())
		.Schedule_PerAllocation<FEvaluateAtom>(&Linker->EntityManager, TaskScheduler, this);

	TaskScheduler->AddPrerequisite(GatherInputsTask, EvaluateAudioTask);
	TaskScheduler->AddPrerequisite(GatherTriggersTask, EvaluateAudioTask);
	TaskScheduler->AddPrerequisite(ResetSharedDataTask, EvaluateAudioTask);
}

void UMovieSceneAtomSystem::OnRun(FSystemTaskPrerequisites& InPrerequisites, FSystemSubsequentTasks& Subsequents)
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_AudioTrack_Evaluate)

	if (!GEngine || !GEngine->UseSound())
	{
		return;
	}

	using namespace UE::MovieScene;

	const FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	const FMovieSceneAtomComponentTypes* TrackComponents = FMovieSceneAtomComponentTypes::Get();

	// Reset shared data.
	ResetSharedData();

	// Gather audio input values computed by the channel evaluators.
	FSystemTaskPrerequisites Prereqs;

	FGraphEventRef Task = FEntityTaskBuilder()
		.Read(BuiltInComponents->InstanceHandle)
		.Read(TrackComponents->Atom)
		.Read(TrackComponents->AtomInputs)
		.ReadOneOrMoreOf(
			BuiltInComponents->DoubleResult[0], BuiltInComponents->DoubleResult[1], BuiltInComponents->DoubleResult[2],
			BuiltInComponents->DoubleResult[3], BuiltInComponents->DoubleResult[4], BuiltInComponents->DoubleResult[5],
			BuiltInComponents->DoubleResult[6], BuiltInComponents->DoubleResult[7], BuiltInComponents->DoubleResult[8],
			BuiltInComponents->StringResult,
			BuiltInComponents->IntegerResult,
			BuiltInComponents->BoolResult)
		.template Dispatch_PerAllocation<FGatherAtomInputs>(&Linker->EntityManager, InPrerequisites, nullptr, this);
	if (Task)
	{
		Prereqs.AddRootTask(Task);
	}

	/*Task = FEntityTaskBuilder()
		.Read(TrackComponents->Audio)
		.Read(TrackComponents->AtomTriggerName)
		.template Dispatch_PerAllocation<FGatherAtomTriggers>(&Linker->EntityManager, InPrerequisites, nullptr, this);
	if (Task)
	{
		Prereqs.AddRootTask(Task);
	}*/

	// Next, evaluate audio to play and use the gathered audio input values to set on the Atom components.
	FEntityTaskBuilder()
		.ReadEntityIDs()
		.Read(BuiltInComponents->RootInstanceHandle)
		.Read(BuiltInComponents->InstanceHandle)
		.Read(TrackComponents->Atom)
		.Read(BuiltInComponents->DoubleResult[0]) // Volume
		.Read(BuiltInComponents->DoubleResult[1]) // Pitch multiplier
		.ReadOptional(BuiltInComponents->BoundObject)
		.SetDesiredThread(Linker->EntityManager.GetGatherThread())
		.template Dispatch_PerAllocation<FEvaluateAtom>(&Linker->EntityManager, Prereqs, &Subsequents, this);
}

UMovieSceneAtomSystem::FAtomComponentEvaluationData* UMovieSceneAtomSystem::GetAtomComponentEvaluationData(FInstanceHandle InstanceHandle, FObjectKey ActorKey, FObjectKey SectionKey)
{
	FAtomComponentBySectionKey* Map = AtomComponentsByActorKey.Find(ActorKey);
	if (Map != nullptr)
	{
		// First, check for an exact match for this entity
		FInstanceObjectKey DataKey{ InstanceHandle, SectionKey };
		FAtomComponentEvaluationData* ExistingData = Map->Find(DataKey);
		if (ExistingData != nullptr)
		{
			if (ExistingData->AtomComponent.IsValid())
			{
				return ExistingData;
			}
		}

		// If no exact match, check for any AtomComponent that isn't busy
		for (FAtomComponentBySectionKey::ElementType& Pair : *Map)
		{
			UAtomComponent* ExistingComponent = Pair.Value.AtomComponent.Get();
			if (ExistingComponent && !ExistingComponent->IsPlaying())
			{
				// Replace this entry with the new entity ID to claim it
				FAtomComponentEvaluationData MovedData(Pair.Value);
				Map->Remove(Pair.Key);
				MovedData.PartialDesyncComputation.Reset();
				return &Map->Add(DataKey, MovedData);
			}
		}
	}

	return nullptr;
}

UMovieSceneAtomSystem::FAtomComponentEvaluationData* UMovieSceneAtomSystem::AddBoundObjectAtomComponent(FInstanceHandle InstanceHandle, UMovieSceneAtomSection* Section, UObject* PrincipalObject)
{
	using namespace UE::MovieScene;

	FObjectKey ObjectKey(PrincipalObject);
	FObjectKey SectionKey(Section);

	FAtomComponentBySectionKey& ActorAtomComponentMap = AtomComponentsByActorKey.FindOrAdd(ObjectKey);

	FAtomComponentEvaluationData* ExistingData = GetAtomComponentEvaluationData(InstanceHandle, ObjectKey, SectionKey);
	if (!ExistingData)
	{
		UAtomSoundSimple* TempPlaybackAtomCue = NewObject<UAtomSoundSimple>();

		AActor* Actor = nullptr;
		USceneComponent* SceneComponent = nullptr;
		FString ObjectName;

		if (PrincipalObject->IsA<AActor>())
		{
			Actor = Cast<AActor>(PrincipalObject);
			SceneComponent = Actor->GetRootComponent();
			ObjectName =
#if WITH_EDITOR
				Actor->GetActorLabel();
#else
				Actor->GetName();
#endif
		}
		else if (PrincipalObject->IsA<UActorComponent>())
		{
			UActorComponent* ActorComponent = Cast<UActorComponent>(PrincipalObject);
			Actor = ActorComponent->GetOwner();
			SceneComponent = Cast<USceneComponent>(ActorComponent);
			ObjectName = ActorComponent->GetName();
		}

		if (!Actor || !SceneComponent)
		{
			const int32 RowIndex = Section->GetRowIndex();
			UE_LOG(LogMovieScene, Warning, TEXT("Failed to find scene component for spatialized Atom track (row %d)."), RowIndex);
			return nullptr;
		}

		FAtomRuntime::FCreateComponentParams Params(Actor->GetWorld(), Actor);
		Params.AtomComponentClass = Section->GetAtomComponentClass();
		UAtomComponent* NewComponent = FAtomRuntime::CreateComponent(TempPlaybackAtomCue, Params);
		
		if (!NewComponent)
		{
			const int32 RowIndex = Section->GetRowIndex();
			UE_LOG(LogMovieScene, Warning, TEXT("Failed to create Atom component for spatialized Atom track (row %d on %s)."), RowIndex, *ObjectName);
			return nullptr;
		}

		NewComponent->SetFlags(RF_Transient);
		NewComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepRelativeTransform);

		FInstanceObjectKey DataKey{ InstanceHandle, SectionKey };
		ExistingData = &ActorAtomComponentMap.Add(DataKey);
		ExistingData->AtomComponent = NewComponent;
	}

	return ExistingData;
}

UMovieSceneAtomSystem::FAtomComponentEvaluationData* UMovieSceneAtomSystem::AddRootAtomComponent(FInstanceHandle InstanceHandle, UMovieSceneAtomSection* Section, UWorld* World)
{
	using namespace UE::MovieScene;

	FObjectKey NullKey;
	FObjectKey SectionKey(Section);

	FAtomComponentBySectionKey& RootAtomComponentMap = AtomComponentsByActorKey.FindOrAdd(NullKey);

	FAtomComponentEvaluationData* ExistingData = GetAtomComponentEvaluationData(InstanceHandle, NullKey, SectionKey);
	if (!ExistingData)
	{
		UAtomSoundSimple* TempPlaybackAtomCue = NewObject<UAtomSoundSimple>();

		FAtomRuntime::FCreateComponentParams Params(World);
		Params.AtomComponentClass = Section->GetAtomComponentClass();
		UAtomComponent* NewComponent = FAtomRuntime::CreateComponent(TempPlaybackAtomCue, Params);

		if (!NewComponent)
		{
			const int32 RowIndex = Section->GetRowIndex();
			UE_LOG(LogMovieScene, Warning, TEXT("Failed to create Atom component for root Atom track (row %d)."), RowIndex);
			return nullptr;
		}

		NewComponent->SetFlags(RF_Transient);

		FInstanceObjectKey DataKey{ InstanceHandle, SectionKey };
		ExistingData = &RootAtomComponentMap.Add(DataKey);
		ExistingData->AtomComponent = NewComponent;
	}

	return ExistingData;
}

void UMovieSceneAtomSystem::StopSound(FInstanceHandle InstanceHandle, FObjectKey ActorKey, FObjectKey SectionKey)
{
	if (FAtomComponentBySectionKey* Map = AtomComponentsByActorKey.Find(ActorKey))
	{
		FInstanceObjectKey DataKey{ InstanceHandle, SectionKey };
		if (FAtomComponentEvaluationData* Data = Map->Find(DataKey))
		{
			if (UAtomComponent* AtomComponent = Data->AtomComponent.Get())
			{
				AtomComponent->Stop();
			}
		}
	}
}

/*

/// Stop audio from playing 
struct FStopAtomPreAnimatedToken : IMovieScenePreAnimatedToken
{
	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FStopAtomPreAnimatedToken>();
	}

	virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
	{
		UAtomComponent* AtomComponent = CastChecked<UAtomComponent>(&InObject);
		if (AtomComponent)
		{
			AtomComponent->Stop();
			AtomComponent->DestroyComponent();
		}
	}

	struct FProducer : IMovieScenePreAnimatedTokenProducer
	{
		virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const override
		{
			return FStopAtomPreAnimatedToken();
		}
	};
};

/// Destroy a transient atom component 
struct FDestroyAtomPreAnimatedToken : IMovieScenePreAnimatedToken
{
	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FDestroyAtomPreAnimatedToken>();
	}

	virtual void RestoreState(UObject& InObject, const UE::MovieScene::FRestoreStateParams& Params) override
	{
		UAtomComponent* AtomComponent = CastChecked<UAtomComponent>(&InObject);
		if (AtomComponent)
		{
			AtomComponent->DestroyComponent();
		}
	}

	struct FProducer : IMovieScenePreAnimatedTokenProducer
	{
		virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const override
		{
			return FDestroyAtomPreAnimatedToken();
		}
	};
};

struct FCachedAtomTrackData : IPersistentEvaluationData
{
	TMap<FName, FMoveSceneAtomTriggerState> TriggerStateMap;

	TMap<FObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAtomComponent>>> AtomComponentsByActorKey;

	FCachedAtomTrackData()
	{
		// Create the container for root tracks, which do not have an actor to attach to
		AtomComponentsByActorKey.Add(FObjectKey(), TMap<FObjectKey, TWeakObjectPtr<UAtomComponent>>());
	}

	/// Set whenever we ask the atom component to start playing a sound. Used to detect desyncs caused when Sequencer evaluates at more-than-real-time. 
	TMap<TWeakObjectPtr<UAtomComponent>, float> SoundLastPlayedAtTime;

	UAtomComponent* GetAtomComponent(FObjectKey ActorKey, FObjectKey SectionKey)
	{
		if (TMap<FObjectKey, TWeakObjectPtr<UAtomComponent>>* Map = AtomComponentsByActorKey.Find(ActorKey))
		{
			// First, check for an exact match for this section
			TWeakObjectPtr<UAtomComponent> ExistingComponentPtr = Map->FindRef(SectionKey);
			if (ExistingComponentPtr.IsValid())
			{
				return ExistingComponentPtr.Get();
			}

			// If no exact match, check for any AtomComponent that isn't busy
			for (TPair<FObjectKey, TWeakObjectPtr<UAtomComponent >> Pair : *Map)
			{
				UAtomComponent* ExistingComponent = Map->FindRef(Pair.Key).Get();
				if (ExistingComponent && !ExistingComponent->IsPlaying())
				{
					// Replace this entry with the new SectionKey to claim it
					Map->Remove(Pair.Key);
					Map->Add(SectionKey, ExistingComponent);
					return ExistingComponent;
				}
			}
		}

		return nullptr;
	}

	// Only to be called on the game thread 
	UAtomComponent* AddAtomComponentForRow(int32 RowIndex, FObjectKey SectionKey, UObject& PrincipalObject, IMovieScenePlayer& Player)
	{
		FObjectKey ObjectKey(&PrincipalObject);

		if (!AtomComponentsByActorKey.Contains(ObjectKey))
		{
			AtomComponentsByActorKey.Add(ObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAtomComponent>>());
		}

		UAtomComponent* ExistingComponent = GetAtomComponent(ObjectKey, SectionKey);
		if (!ExistingComponent)
		{
			UAtomSoundSimple* TempPlaybackAtomCue = NewObject<UAtomSoundSimple>();

			AActor* Actor = nullptr;
			USceneComponent* SceneComponent = nullptr;
			FString ObjectName;

			if (PrincipalObject.IsA<AActor>())
			{
				Actor = Cast<AActor>(&PrincipalObject);
				SceneComponent = Actor->GetRootComponent();
				ObjectName =
#if WITH_EDITOR
					Actor->GetActorLabel();
#else
					Actor->GetName();
#endif
			}
			else if (PrincipalObject.IsA<UActorComponent>())
			{
				UActorComponent* ActorComponent = Cast<UActorComponent>(&PrincipalObject);
				Actor = ActorComponent->GetOwner();
				SceneComponent = Cast<USceneComponent>(ActorComponent);
				ObjectName = ActorComponent->GetName();
			}

			if (!Actor || !SceneComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to find scene component for spatialized atom track (row %d)."), RowIndex);
				return nullptr;
			}

			FAtomRuntime::FCreateComponentParams Params(Actor->GetWorld(), Actor);
			ExistingComponent = FAtomRuntime::CreateComponent(TempPlaybackAtomCue, Params);

			if (!ExistingComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to create atom component for spatialized atom track (row %d on %s)."), RowIndex, *ObjectName);
				return nullptr;
			}

			Player.SavePreAnimatedState(*ExistingComponent, FMovieSceneAnimTypeID::Unique(), FDestroyAtomPreAnimatedToken::FProducer());

			AtomComponentsByActorKey[ObjectKey].Add(SectionKey, ExistingComponent);

			ExistingComponent->SetFlags(RF_Transient);
			ExistingComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		return ExistingComponent;
	}

	// Only to be called on the game thread 
	UAtomComponent* AddRootAtomComponentForRow(int32 RowIndex, FObjectKey SectionKey, UWorld* World, IMovieScenePlayer& Player, const UMovieSceneAtomSection* AtomSection)
	{
		UAtomComponent* ExistingComponent = GetAtomComponent(FObjectKey(), SectionKey);
		
		if (!ExistingComponent || !ExistingComponent->IsA(AtomSection->GetAtomComponentClass()))
		{
			UAtomSoundSimple* TempPlaybackAtomCue = NewObject<UAtomSoundSimple>();
			FAtomRuntime::FCreateComponentParams Params(World);
			Params.AtomComponentClass = AtomSection->GetAtomComponentClass();

			ExistingComponent = FAtomRuntime::CreateComponent(TempPlaybackAtomCue, Params);

			if (!ExistingComponent)
			{
				UE_LOG(LogMovieScene, Warning, TEXT("Failed to create audio component for root audio track (row %d)."), RowIndex);
				return nullptr;
			}

			Player.SavePreAnimatedState(*ExistingComponent, FMovieSceneAnimTypeID::Unique(), FDestroyAtomPreAnimatedToken::FProducer());
			
			ExistingComponent->SetFlags(RF_Transient);

			AtomComponentsByActorKey[FObjectKey()].Add(SectionKey, ExistingComponent);
		}

		return ExistingComponent;
	}

	void StopAllSounds()
	{
		for (TPair<FObjectKey, TMap<FObjectKey, TWeakObjectPtr<UAtomComponent>>>& Map : AtomComponentsByActorKey)
		{
			for (TPair<FObjectKey, TWeakObjectPtr<UAtomComponent>>& Pair : Map.Value)
			{
				if (UAtomComponent* AtomComponent = Pair.Value.Get())
				{
					AtomComponent->Stop();
				}
			}
		}
	}
};

struct FAtomSectionExecutionToken : IMovieSceneExecutionToken
{
	FAtomSectionExecutionToken(const UMovieSceneAtomSection* InAtomSection)
		: AtomSection(InAtomSection), SectionKey(InAtomSection)
	{}

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player)
	{
		FCachedAtomTrackData& TrackData = PersistentData.GetOrAddTrackData<FCachedAtomTrackData>();

		// If the status says we jumped, we always stop all sounds, then allow them to be played again naturally below if status == Playing (for example)
		if (Context.HasJumped())
		{
			TrackData.StopAllSounds();
		}

		if ((Context.GetStatus() != EMovieScenePlayerStatus::Playing && Context.GetStatus() != EMovieScenePlayerStatus::Scrubbing && Context.GetStatus() != EMovieScenePlayerStatus::Stepping) || Context.GetDirection() == EPlayDirection::Backwards)
		{
			// stopped, recording, etc
			TrackData.StopAllSounds();
		}

		// Root atom track
		else if (!Operand.ObjectBindingID.IsValid())
		{
			UObject* PlaybackContext = Player.GetPlaybackContext();

			const FMovieSceneActorReferenceData& AttachActorData = AtomSection->GetAttachActorData();

			USceneComponent* AttachComponent = nullptr;
			FMovieSceneActorReferenceKey AttachKey;
			AttachActorData.Evaluate(Context.GetTime(), AttachKey);
			FMovieSceneObjectBindingID AttachBindingID = AttachKey.Object;
			if (AttachBindingID.IsValid())
			{
				// If the transform is set, otherwise use the bound actor's transform
				for (TWeakObjectPtr<> WeakObject : AttachBindingID.ResolveBoundObjects(Operand.SequenceID, Player))
				{
					AActor* AttachActor = Cast<AActor>(WeakObject.Get());
					if (AttachActor)
					{
						AttachComponent = AtomSection->GetAttachComponent(AttachActor, AttachKey);
					}
					if (AttachComponent)
					{
						break;
					}
				}
			}

			UAtomComponent* AtomComponent = TrackData.GetAtomComponent(FObjectKey(), SectionKey);
			if (!AtomComponent)
			{
				// Initialize the sound
				AtomComponent = TrackData.AddRootAtomComponentForRow(AtomSection->GetRowIndex(), SectionKey, PlaybackContext ? PlaybackContext->GetWorld() : nullptr, Player, AtomSection);

				if (AtomComponent)
				{
					
					//if (AtomSection->GetOnQueueSubtitles().IsBound())
					//{
					//	AtomComponent->OnQueueSubtitles = AtomSection->GetOnQueueSubtitles();
					//}
					
					if (AtomSection->GetOnAtomSoundFinished().IsBound())
					{
						AtomComponent->OnAtomSoundFinished = AtomSection->GetOnAtomSoundFinished();
					}
					if (AtomSection->GetOnAtomSoundPlaybackPercent().IsBound())
					{
						AtomComponent->OnAtomSoundPlaybackPercent = AtomSection->GetOnAtomSoundPlaybackPercent();
					}
				}
			}

			if (AtomComponent)
			{
				if (AttachComponent && (AtomComponent->GetAttachParent() != AttachComponent || AtomComponent->GetAttachSocketName() != AttachKey.SocketName))
				{
					AtomComponent->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachKey.SocketName);
				}

				EnsureAtomIsPlaying(*AtomComponent, PersistentData, Context, AtomComponent->GetAttachParent() != nullptr, Player);
			}
		}

		// Object binding atom track
		else
		{
			for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
			{
				UAtomComponent* AtomComponent = TrackData.GetAtomComponent(Object.Get(), SectionKey);
				if (!AtomComponent)
				{
					// Initialize the sound
					AtomComponent = TrackData.AddAtomComponentForRow(AtomSection->GetRowIndex(), SectionKey, *Object.Get(), Player);

					if (AtomComponent)
					{
						
						//if (AtomSection->GetOnQueueSubtitles().IsBound())
						//{
						//	AtomComponent->OnQueueSubtitles = AtomSection->GetOnQueueSubtitles();
						//}
						
						if (AtomSection->GetOnAtomSoundFinished().IsBound())
						{
							AtomComponent->OnAtomSoundFinished = AtomSection->GetOnAtomSoundFinished();
						}
						if (AtomSection->GetOnAtomSoundPlaybackPercent().IsBound())
						{
							AtomComponent->OnAtomSoundPlaybackPercent = AtomSection->GetOnAtomSoundPlaybackPercent();
						}
					}
				}
				
				if (AtomComponent)
				{
					EnsureAtomIsPlaying(*AtomComponent, PersistentData, Context, true, Player);
				}
			}
		}
	}

	// Helper template to pair channel evaluation and parameter application.
	//template<typename ChannelType, typename ValueType>
	//void EvaluateAllAndSetParameters(IAudioParameterControllerInterface& InParamaterInterface, const FFrameTime& InTime) const
	//{
	//	AtomSection->ForEachInput([&InParamaterInterface, &InTime](FName InName, const ChannelType& InChannel)
	//	{
	//		using namespace UE::MovieScene;
	//		ValueType OutValue{};
	//		if (EvaluateChannel(&InChannel, InTime, OutValue))
	//		{
	//			InParamaterInterface.SetParameter(InName, MoveTempIfPossible(OutValue));
	//		}
	//	});
	//}
	//
	//void EvaluateAllAndFireTriggers(IAudioParameterControllerInterface& InParamaterInterface, const FMovieSceneContext& InContext, FCachedAtomTrackData& InPersistentData) const
	//{
	//	AtomSection->ForEachInput([&InParamaterInterface, &InContext, &InPersistentData](FName InName, const FMovieSceneAtomTriggerChannel& InChannel)
	//	{
	//		bool OutValue = false;
	//		FMoveSceneAtomTriggerState& TriggerState = InPersistentData.TriggerStateMap.FindOrAdd(InName);
	//		if (InChannel.EvaluatePossibleTriggers(InContext, TriggerState, OutValue))
	//		{
	//			if (OutValue)
	//			{
	//				InParamaterInterface.SetTriggerParameter(InName);
	//			}
	//		}
	//	});
	//}
	
	void EnsureAtomIsPlaying(UAtomComponent& AtomComponent, FPersistentEvaluationData& PersistentData, const FMovieSceneContext& Context, bool bAllowSpatialization, IMovieScenePlayer& Player) const
	{
		Player.SavePreAnimatedState(AtomComponent, FStopAtomPreAnimatedToken::GetAnimTypeID(), FStopAtomPreAnimatedToken::FProducer());

		float AudioVolume = 1.f;
		AtomSection->GetSoundVolumeChannel().Evaluate(Context.GetTime(), AudioVolume);
		AudioVolume = AudioVolume * AtomSection->EvaluateEasing(Context.GetTime());
		if (AtomComponent.VolumeMultiplier != AudioVolume)
		{
			AtomComponent.SetVolumeMultiplier(AudioVolume);
		}

		float PitchMultiplier = 1.f;
		AtomSection->GetPitchMultiplierChannel().Evaluate(Context.GetTime(), PitchMultiplier);
		if (AtomComponent.PitchMultiplier != PitchMultiplier)
		{
			AtomComponent.SetPitchMultiplier(PitchMultiplier);
		}

		//AtomComponent.bSuppressSubtitles = AtomSection->GetSuppressSubtitles();

		//Evaluate inputs and apply the params.
		//EvaluateAllAndSetParameters<FMovieSceneFloatChannel, float>(AtomComponent, Context.GetTime());
		//EvaluateAllAndSetParameters<FMovieSceneBoolChannel, bool>(AtomComponent, Context.GetTime());
		//EvaluateAllAndSetParameters<FMovieSceneIntegerChannel, int32>(AtomComponent, Context.GetTime());
		//EvaluateAllAndSetParameters<FMovieSceneStringChannel, FString>(AtomComponent, Context.GetTime());

		TArray<FAisacControlAndCurve> AisacControlsAndCurves = AtomSection->GetAisacControlsAndCurves();
		for (auto ScalarParameterNamesAndCurve : AisacControlsAndCurves)
		{
			float AisacValue = 0.0f;
			ScalarParameterNamesAndCurve.ParameterCurve.Evaluate(Context.GetTime(), AisacValue);

			FAtomAisacControl ControlParam;
			ControlParam.Name = ScalarParameterNamesAndCurve.ParameterName;
			ControlParam.ID = -1; // TODO id -> name function 

			AtomComponent.SetAisacControlValue(ControlParam, AisacValue);
		}

		float SectionStartTimeSeconds = (AtomSection->HasStartFrame() ? AtomSection->GetInclusiveStartFrame() : 0) / AtomSection->GetTypedOuter<UMovieScene>()->GetTickResolution();

		FCachedAtomTrackData& TrackData = PersistentData.GetOrAddTrackData<FCachedAtomTrackData>();
		const FFrameNumber AudioStartOffset = AtomSection->GetStartOffset();
		UAtomSoundBase* Sound = AtomSection->GetSound();

		float AudioTime = (Context.GetTime() / Context.GetFrameRate()) - SectionStartTimeSeconds + (float)Context.GetFrameRate().AsSeconds(AudioStartOffset);
		if (AudioTime >= 0.f && Sound)
		{
			const float Duration = Sound->GetDuration();
			//const float Duration = MovieSceneHelpers::GetSoundDuration(Sound);

			if (!AtomSection->GetLooping() && AudioTime > Duration && Duration != 0.f)
			{
				AtomComponent.Stop();
				return;
			}

			AudioTime = Duration > 0.f ? FMath::Fmod(AudioTime, Duration) : AudioTime;
		}

		// If the atom component is not playing we (may) need a state change. If the atom component is playing
		// the wrong sound then we need a state change. If the audio playback time is significantly out of sync 
		// with the desired time then we need a state change.
		bool bSoundNeedsStateChange = !AtomComponent.IsPlaying() || AtomComponent.Sound != Sound;
		bool bSoundNeedsTimeSync = false;

		UObject* PlaybackContext = Player.GetPlaybackContext();
		UWorld* World = PlaybackContext ? PlaybackContext->GetWorld() : nullptr;

		// Sync only if there is no time dilation because otherwise the system will constantly resync because audio 
		// playback is not dilated and will never match the expected playback time.
		const bool bDoTimeSync =
			World && World->GetWorldSettings() &&
			(FMath::IsNearlyEqual(World->GetWorldSettings()->GetEffectiveTimeDilation(), 1.f) ||
				!bIgnoreAtomSyncDuringWorldTimeDilationCVar);

		if (bDoTimeSync)
		{
			float CurrentGameTime = 0.0f;

			FAtomRuntime* AtomRuntime = World ? FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World) : nullptr;
			if (UseAtomClockForSequencerDesyncCVar && AtomRuntime)
			{
				CurrentGameTime = AtomRuntime->GetAtomClock();
			}
			else
			{
				CurrentGameTime = World ? World->GetAudioTimeSeconds() : 0.f;
			}

			// This tells us how much time has passed in the game world (and thus, reasonably, the audio playback)
			// so if we calculate that we should be playing say, 15s into the section during evaluation, but
			// we're only 5s since the last Play call, then we know we're out of sync. 
			if (TrackData.SoundLastPlayedAtTime.Contains(&AtomComponent))
			{
				float SoundLastPlayedAtTime = TrackData.SoundLastPlayedAtTime[&AtomComponent];

				float GameTimeDelta = CurrentGameTime - SoundLastPlayedAtTime;
				if (!FMath::IsNearlyZero(MaxSequenceAtomDesyncToleranceCVar) && FMath::Abs(GameTimeDelta - AudioTime) > MaxSequenceAtomDesyncToleranceCVar)
				{
					UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component detected a significant mismatch in (assumed) playback time versus the desired time. Time since last play call: %6.2f(s) Desired Time: %6.2f(s). Component: %s sound: %s"), GameTimeDelta, AudioTime, *AtomComponent.GetName(), *GetNameSafe(AtomComponent.Sound));
					bSoundNeedsTimeSync = true;
				}
			}
		}

		if (bSoundNeedsStateChange || bSoundNeedsTimeSync)
		{
			AtomComponent.bAllowSpatialization = bAllowSpatialization;

			if (AtomSection->GetOverrideAttenuation())
			{
				AtomComponent.AttenuationSettings = AtomSection->GetAttenuationSettings();
				if (!AtomComponent.AttenuationSettings)
				{
					AtomComponent.AttenuationOverrides = AtomSection->GetAttenuationOverrides();
				}
			}


			// Call stop sound only if it's actually playing. 
			// This prevents the stop instruction from being called continuously when a sound cue with zero spawn time is playing.
			if (AtomComponent.IsPlaying() || bSoundNeedsTimeSync)
			{
				UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component stopped due to needing a state change bIsPlaying: %d bNeedsTimeSync: %d. Component: %s sound: %s"), AtomComponent.IsPlaying(), bSoundNeedsTimeSync, *AtomComponent.GetName(), *AtomComponent.Sound->GetName());
				AtomComponent.Stop();
			}

			// Only change if the sound clip actually changed. Calls Stop internally when necessary.
			if (AtomComponent.Sound != Sound)
			{
				UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component calling SetSound due to new sound. Component: %s OldSound: %s NewSound: %s"), *AtomComponent.GetName(), *GetNameSafe(AtomComponent.Sound), *AtomComponent.Sound->GetName());
				AtomComponent.SetSound(Sound);
			}
#if WITH_EDITOR
			if (GIsEditor && World != nullptr && !World->IsPlayInEditor())
			{
				AtomComponent.bIsUISound = true;
				AtomComponent.bIsPreviewSound = true;
			}
			else
#endif // WITH_EDITOR
			{
				AtomComponent.bIsUISound = false;
			}

			if (AudioTime >= 0.f)
			{
				AtomComponent.CueSelectors = AtomSection->GetSelectorLabels();
				AtomComponent.CueFirstBlockIndex = AtomSection->GetBlockIndex();

				UE_LOG(LogMovieScene, Verbose, TEXT("Atom Component Play at Local Time: %6.2f CurrentTime: %6.2f(s) SectionStart: %6.2f(s), SoundDur: %6.2f OffsetIntoClip: %6.2f sound: %s"), AudioTime, (Context.GetTime() / Context.GetFrameRate()), SectionStartTimeSeconds, AtomComponent.Sound->GetDuration(), (float)Context.GetFrameRate().AsSeconds(AudioStartOffset), *AtomComponent.Sound->GetName());
				AtomComponent.Play(AudioTime);

				// Keep track of when we asked this audio clip to play (in game time) so that we can figure out if there's a significant desync in the future.
				if (World)
				{
					if (!TrackData.SoundLastPlayedAtTime.Contains(&AtomComponent))
					{
						TrackData.SoundLastPlayedAtTime.Add(&AtomComponent);
					}

					FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);
					if (UseAtomClockForSequencerDesyncCVar && AtomRuntime)
					{
						TrackData.SoundLastPlayedAtTime[&AtomComponent] = AtomRuntime->GetAtomClock() - AudioTime;
					}
					else
					{
						TrackData.SoundLastPlayedAtTime[&AtomComponent] = World->GetAudioTimeSeconds() - AudioTime;
					}
				}
			}
		}

		if (Context.GetStatus() == EMovieScenePlayerStatus::Scrubbing || Context.GetStatus() == EMovieScenePlayerStatus::Stepping)
		{
			// While scrubbing, play the sound for a short time and then cut it.
			//AtomComponent.StopDelayed(AtomTrackConstants::ScrubDuration);
			AtomComponent.Stop();
		}

		//if (AtomComponent.IsPlaying())
		//{
		//	EvaluateAllAndFireTriggers(AtomComponent, Context, TrackData);
		//}

		if (bAllowSpatialization)
		{
			if (FAtomRuntime* AtomRuntime = AtomComponent.GetAtomRuntime())
			{
				DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.MovieSceneUpdateAtomTransform"), STAT_MovieSceneUpdateAtomTransform, STATGROUP_TaskGraphTasks);
				AtomRuntime->SendCommandToActiveSounds(AtomComponent.GetAtomComponentID(), [ActorTransform = AtomComponent.GetComponentTransform()](FAtomActiveSound& ActiveSound)
				{
					ActiveSound.bLocationDefined = true;
					ActiveSound.Transform = ActorTransform;
				}, GET_STATID(STAT_MovieSceneUpdateAtomTransform));
			}
		}
	}

	const UMovieSceneAtomSection* AtomSection;
	FObjectKey SectionKey;
};

FMovieSceneAtomSectionTemplate::FMovieSceneAtomSectionTemplate()
	: AtomSection()
{
}

FMovieSceneAtomSectionTemplate::FMovieSceneAtomSectionTemplate(const UMovieSceneAtomSection& Section)
	: AtomSection(&Section)
{
}

void FMovieSceneAtomSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_AtomTrack_Evaluate)

	if (GEngine && GEngine->UseSound() && Context.GetStatus() != EMovieScenePlayerStatus::Jumping)
	{
		ExecutionTokens.Add(FAtomSectionExecutionToken(AtomSection));
	}
}
*/