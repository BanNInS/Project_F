/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareMovieScene
 * File     : MovieSceneAtomSection.cpp
 *
 ****************************************************************************/

#include "MovieSceneAtomSection.h"

#include "UObject/SequencerObjectVersion.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "GameFramework/Actor.h"
#include "MovieScene.h"
#include "MovieSceneCommonHelpers.h"
#include "Misc/FrameRate.h"
#include "Misc/GeneratedTypeName.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "MovieSceneAtomComponentTypes.h"

#include "Atom/AtomPluginUtilities.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomConfig.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

#include "MovieSceneAtomTrack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneAtomSection)

#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "AtomTrackEditor"
#endif

#if WITH_EDITOR
struct FAtomChannelEditorData
{
	FAtomChannelEditorData()
	{
		Data[0].SetIdentifiers("Volume", NSLOCTEXT("MovieSceneAtomSection", "SoundVolumeText", "Volume"));
		Data[1].SetIdentifiers("Pitch", NSLOCTEXT("MovieSceneAtomSection", "PitchText", "Pitch"));
		Data[2].SetIdentifiers("AttachActor", NSLOCTEXT("MovieSceneAtomSection", "AttachActorText", "Attach"));
	}

	FMovieSceneChannelMetaData Data[3];
};
#endif // WITH_EDITOR

namespace
{
	FFrameNumber GetStartOffsetAtTrimTime(FQualifiedFrameTime TrimTime, FFrameNumber StartOffset, FFrameNumber StartFrame)
	{
		return StartOffset + TrimTime.Time.FrameNumber - StartFrame;
	}
} // namespace

namespace UE::MovieScene
{
	/*
	 * Entity IDs are an encoded type and index, with the upper bit being the type (scalar iputs vs audio trigger),
	 * and the lower 31 bits as the entity index
	 */
	enum class EAtomSectionEntityType : uint8 { MainEntity, InputsEntity, TriggerEntity };

	uint32 EncodeEntityID(int32 InIndex, EAtomSectionEntityType InEntityType)
	{
		check(InIndex >= 0 && InIndex < int32(0x00FFFFFF));
		return static_cast<uint32>(InIndex) | ((uint8)InEntityType << 24);
	}
	void DecodeEntityID(uint32 InEntityID, int32& OutIndex, EAtomSectionEntityType& OutEntityType)
	{
		OutIndex = static_cast<int32>(InEntityID & 0x00FFFFFF);
		OutEntityType = (EAtomSectionEntityType)(InEntityID >> 24);
	}
} // namespace

FAisacControlAndCurve::FAisacControlAndCurve(const FAtomAisacControl& InAisacControl)
{
	AisacControl = InAisacControl;
}

UMovieSceneAtomSection::UMovieSceneAtomSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Sound = nullptr;
	bLooping = true;
	//bSuppressSubtitles = false;
	bOverrideAttenuation = false;
	BlendType = EMovieSceneBlendType::Absolute;

	EvalOptions.EnableAndSetCompletionMode
		(GetLinkerCustomVersion(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault ?
			EMovieSceneCompletionMode::RestoreState :
			EMovieSceneCompletionMode::ProjectDefault);

	SoundVolume.SetDefault(1.f);
	PitchMultiplier.SetDefault(1.f);
	AisacControlsAndCurves.Empty();
	FirstBlockIndex = 0;

	AtomComponentClass = UAtomComponent::StaticClass();
	bApplyExtensionSettings = false;
	ExtensionSettings = nullptr;
}

namespace MovieSceneAtomSectionPrivate
{
	template<typename ChannelType, typename ValueType>
	void AddInputChannels(UMovieSceneAtomSection* InSection, FMovieSceneChannelProxyData& InChannelProxyData)
	{
		InSection->ForEachInput([&InChannelProxyData](FName InName, const ChannelType& InChannel)
		{
#if WITH_EDITOR
			FMovieSceneChannelMetaData Data;
			FText TextName = FText::FromName(InName);
			Data.SetIdentifiers(FName(InName.ToString() + GetGeneratedTypeName<ChannelType>()), TextName, TextName);
			InChannelProxyData.Add(const_cast<ChannelType&>(InChannel), Data, TMovieSceneExternalValue<ValueType>::Make());
#else
			InChannelProxyData.Add(const_cast<ChannelType&>(InChannel));
#endif
		});
	}
}

void UMovieSceneAtomSection::PostEditImport()
{
	Super::PostEditImport();

	CacheChannelProxy();
}

TArray<FAisacControlAndCurve>& UMovieSceneAtomSection::GetAisacControlsAndCurves()
{
	return AisacControlsAndCurves;
}

const TArray<FAisacControlAndCurve>& UMovieSceneAtomSection::GetAisacControlsAndCurves() const
{
	return AisacControlsAndCurves;
}

EMovieSceneChannelProxyType  UMovieSceneAtomSection::CacheChannelProxy()
{
	// Set up the channel proxy
	FMovieSceneChannelProxyData Channels;

	UMovieSceneAtomTrack* AtomTrack = Cast<UMovieSceneAtomTrack>(GetOuter());
	UMovieScene* MovieScene = AtomTrack ? Cast<UMovieScene>(AtomTrack->GetOuter()) : nullptr;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	const bool bHasAttachData = MovieScene && MovieScene->ContainsTrack(*AtomTrack);
#else
	const bool bHasAttachData = AtomTrack && MovieScene->IsAMasterTrack(*AtomTrack);
#endif

#if WITH_EDITOR
	FAtomChannelEditorData EditorData;
	
	int32 SortOrder = 0;
	EditorData.Data[0].SortOrder = SortOrder++;
	EditorData.Data[1].SortOrder = SortOrder++;
	EditorData.Data[2].SortOrder = SortOrder++;
	
	Channels.Add(SoundVolume, EditorData.Data[0], TMovieSceneExternalValue<float>());
	Channels.Add(PitchMultiplier, EditorData.Data[1], TMovieSceneExternalValue<float>());
	if (bHasAttachData)
	{
		Channels.Add(AttachActorData, EditorData.Data[2]);
	}
#else
	Channels.Add(SoundVolume);
	Channels.Add(PitchMultiplier);
	if (bHasAttachData)
	{
		Channels.Add(AttachActorData);
	}
#endif

	if (auto SoundCue = Cast<UAtomSoundCue>(Sound))
	{
		for (FAisacControlAndCurve& AisacControlAndCurve : AisacControlsAndCurves)
		{
#if WITH_EDITOR

			FName AisacAcfName = AisacControlAndCurve.AisacControl.Name;
			if (GCriWare)
			{
				if (auto CurrentAtomConfig = GCriWare->GetAtomConfiguration())
				{
					if (auto ControlPtr = CurrentAtomConfig->GetAisacControlByID(AisacControlAndCurve.AisacControl.ID))
					{
						AisacAcfName = ControlPtr->Name;
					}
				}
			}

			auto GetAisacControlDisplayText = [](FName ControlName)
			{
				return FText::Format(LOCTEXT("AisacControlDisplayText", "AISAC: {0}"), FText::FromName(ControlName));
			};
			auto GetAisacControlInternalName = [](const FAtomAisacControl& AisacControl)
			{
				return FName(AisacControl.Name, AisacControl.ID);
			};
			auto GetAisacControlTooltipText = [](IMovieScenePlayer* Player, FGuid BindingID, FMovieSceneSequenceID SequenceID, const FAtomAisacControl& AisacControl, FName ControlName)
			{
				// in case name mismatches
				if (AisacControl.Name != ControlName)
				{
					return FText::Format(LOCTEXT("AisacAcfControlTooltipText", "AISAC: Acf {0} Cue {1} (ID: {2})"), FText::FromName(ControlName), FText::FromName(AisacControl.Name), FText::FromString(FString::FromInt(AisacControl.ID)));
				}

				return FText::Format(LOCTEXT("AisacControlTooltipText", "AISAC: {0} (ID: {1})"), FText::FromName(ControlName), FText::FromString(FString::FromInt(AisacControl.ID)));
			};

			FName AisacInternalName = GetAisacControlInternalName(AisacControlAndCurve.AisacControl);
			FMovieSceneChannelMetaData MetaData(AisacInternalName, GetAisacControlDisplayText(AisacAcfName));
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
			MetaData.GetTooltipTextDelegate.BindLambda(GetAisacControlTooltipText, AisacControlAndCurve.AisacControl, AisacAcfName);
#endif
			MetaData.bCanCollapseToTrack = false;
			MetaData.SortOrder = SortOrder++;
			AisacControlAndCurve.AisacValueCurve.SetDefault(0.0f);
			Channels.Add(AisacControlAndCurve.AisacValueCurve, MetaData, TMovieSceneExternalValue<float>());
#else
			AisacControlAndCurve.AisacValueCurve.SetDefault(0.0f);
			Channels.Add(AisacControlAndCurve.AisacValueCurve);
#endif
		}
	}

	//using namespace MovieSceneAtomSectionPrivate;
	//SetupSoundInputParameters(Sound);
	//AddInputChannels<FMovieSceneFloatChannel, float>(this, Channels);
	//AddInputChannels<FMovieSceneBoolChannel, bool>(this, Channels);
	//AddInputChannels<FMovieSceneIntegerChannel, int32>(this, Channels);
	//AddInputChannels<FMovieSceneStringChannel, FString>(this, Channels);
	//AddInputChannels<FMovieSceneAtomTriggerChannel, bool>(this, Channels);

	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));

	return EMovieSceneChannelProxyType::Dynamic;
}

void UMovieSceneAtomSection::AddAisacChannel(const FAtomAisacControl& AisacControl)
{
	AisacControlsAndCurves.Add(FAisacControlAndCurve(AisacControl));
	
	CacheChannelProxy();
}

bool UMovieSceneAtomSection::DeleteAisacChannel(FName AisacControlName)
{
	const int32 Index = AisacControlsAndCurves.IndexOfByPredicate([AisacControlName](const FAisacControlAndCurve& Param) { return Param.AisacControl.ID == AisacControlName.GetNumber(); });
	if (Index != INDEX_NONE)
	{
		AisacControlsAndCurves.RemoveAt(Index);
		CacheChannelProxy();
		return true;
	}

	return false;
}

TOptional<FFrameTime> UMovieSceneAtomSection::GetOffsetTime() const
{
	return TOptional<FFrameTime>(StartFrameOffset);
}

void UMovieSceneAtomSection::MigrateFrameTimes(FFrameRate SourceRate, FFrameRate DestinationRate)
{
	if (StartFrameOffset.Value > 0)
	{
		FFrameNumber NewStartFrameOffset = ConvertFrameTime(FFrameTime(StartFrameOffset), SourceRate, DestinationRate).FloorToFrame();
		StartFrameOffset = NewStartFrameOffset;
	}
}

void UMovieSceneAtomSection::PostLoad()
{
	Super::PostLoad();

	CacheChannelProxy();

#if WITH_EDITOR
	if (AtomComponentClass && !ExtensionSettings)
	{
		auto ExtensionClass = UAtomComponentExtensionSettings::FindExtensionSettingsClassForAtomComponent(AtomComponentClass);
		ExtensionSettings = ExtensionClass ? NewObject<UAtomComponentExtensionSettings>(this, ExtensionClass, NAME_None, RF_Public) : nullptr;
	}
#endif
}

TOptional<TRange<FFrameNumber>> UMovieSceneAtomSection::GetAutoSizeRange() const
{
	if (!Sound)
	{
		return TRange<FFrameNumber>();
	}

	// get frame rate
	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();

	// get length of audio data in seconds
	float SoundDuration = Sound->GetDuration();
	if (SoundDuration == ATOM_INDEFINITELY_LOOPING_DURATION)
	{
		SoundDuration = Sound->WaveInfo.GetDuration();
	}

	// Convert to frame units
	FFrameTime DurationToUse = SoundDuration * FrameRate;

	return TRange<FFrameNumber>(GetInclusiveStartFrame(), GetInclusiveStartFrame() + DurationToUse.FrameNumber);
}

void UMovieSceneAtomSection::TrimSection(FQualifiedFrameTime TrimTime, bool bTrimLeft, bool bDeleteKeys)
{
	SetFlags(RF_Transactional);

	if (TryModify())
	{
		if (bTrimLeft)
		{
			StartFrameOffset = HasStartFrame() ? GetStartOffsetAtTrimTime(TrimTime, StartFrameOffset, GetInclusiveStartFrame()) : 0;
		}

		Super::TrimSection(TrimTime, bTrimLeft, bDeleteKeys);
	}
}

UMovieSceneSection* UMovieSceneAtomSection::SplitSection(FQualifiedFrameTime SplitTime, bool bDeleteKeys)
{
	const FFrameNumber InitialStartFrameOffset = StartFrameOffset;

	const FFrameNumber NewOffset = HasStartFrame() ? GetStartOffsetAtTrimTime(SplitTime, StartFrameOffset, GetInclusiveStartFrame()) : 0;

	UMovieSceneSection* NewSection = Super::SplitSection(SplitTime, bDeleteKeys);
	if (NewSection != nullptr)
	{
		UMovieSceneAtomSection* NewAtomSection = Cast<UMovieSceneAtomSection>(NewSection);
		NewAtomSection->StartFrameOffset = NewOffset;
	}

	// Restore original offset modified by splitting
	StartFrameOffset = InitialStartFrameOffset;

	return NewSection;
}

void UMovieSceneAtomSection::SetSound(UAtomSoundBase* InSound)
{
	Sound = InSound;
	CacheChannelProxy();
}

USceneComponent* UMovieSceneAtomSection::GetAttachComponent(const AActor* InParentActor, const FMovieSceneActorReferenceKey& Key) const
{
	// Get the component to which AtomCompoent should be attached
	FName AttachComponentName = Key.ComponentName;
	FName AttachSocketName = Key.SocketName;

	if (AttachSocketName != NAME_None)
	{
		// If Socket is specified, attach to Socket position
		if (AttachComponentName != NAME_None)
		{
			// If the specified component name is found and the socket exists, return that component
			TInlineComponentArray<USceneComponent*> PotentialAttachComponents(InParentActor);
			for (USceneComponent* PotentialAttachComponent : PotentialAttachComponents)
			{
				if (PotentialAttachComponent->GetFName() == AttachComponentName && PotentialAttachComponent->DoesSocketExist(AttachSocketName))
				{
					return PotentialAttachComponent;
				}
			}
		}
		else if (InParentActor->GetRootComponent()->DoesSocketExist(AttachSocketName))
		{
			return InParentActor->GetRootComponent();
		}
	}
	else if (AttachComponentName != NAME_None)
	{
		// Returns the component with the specified name if the Socket name is not specified
		TInlineComponentArray<USceneComponent*> PotentialAttachComponents(InParentActor);
		for (USceneComponent* PotentialAttachComponent : PotentialAttachComponents)
		{
			if (PotentialAttachComponent->GetFName() == AttachComponentName)
			{
				return PotentialAttachComponent;
			}
		}
	}

	if (InParentActor->GetDefaultAttachComponent())
	{
		// If the actor has a default attached component, return that component.
		return InParentActor->GetDefaultAttachComponent();
	}
	else
	{
		return InParentActor->GetRootComponent();
	}
}

bool UMovieSceneAtomSection::PopulateEvaluationFieldImpl(const TRange<FFrameNumber>& EffectiveRange, const FMovieSceneEvaluationFieldEntityMetaData& InMetaData, FMovieSceneEntityComponentFieldBuilder* OutFieldBuilder)
{
	using namespace UE::MovieScene;

	int32 MetaDataIndex = OutFieldBuilder->AddMetaData(InMetaData);

	// Add the default entity first.
	int32 MainEntityIndex = OutFieldBuilder->FindOrAddEntity(this, EncodeEntityID(0, EAtomSectionEntityType::MainEntity));
	OutFieldBuilder->AddPersistentEntity(EffectiveRange, MainEntityIndex, MetaDataIndex);

	// See how many additional entities we need to store the Atom input data.
	// We can pack 9 float channels per entity, but we can only have one string/bool/int/audio-trigger
	// channel per entity.
	int32 NumInputDataEntities = Inputs_Float.Num() % 9;
	NumInputDataEntities = FMath::Max(NumInputDataEntities, Inputs_String.Num());
	NumInputDataEntities = FMath::Max(NumInputDataEntities, Inputs_Bool.Num());
	NumInputDataEntities = FMath::Max(NumInputDataEntities, Inputs_Int.Num());

	// Add these extra entities to the evaluation field.
	for (int32 InputDataEntity = 0; InputDataEntity < NumInputDataEntities; ++InputDataEntity)
	{
		int32 EntityIndex = OutFieldBuilder->FindOrAddEntity(this, EncodeEntityID(InputDataEntity, EAtomSectionEntityType::InputsEntity));
		OutFieldBuilder->AddPersistentEntity(EffectiveRange, EntityIndex, MetaDataIndex);
	}

	// Atom triggers are added differently, as one-shot entities.
	/*TArray<FName> AtomTriggerNames;
	Inputs_Trigger.GetKeys(AtomTriggerNames);
	AudioTriggerNames.Sort(FNameLexicalLess());
	for (int32 TriggerIndex = 0; TriggerIndex < AtomTriggerNames.Num(); ++TriggerIndex)
	{
		FMovieSceneAtomTriggerChannel& TriggerChannel = Inputs_Trigger[AudioTriggerNames[TriggerIndex]];
		TArrayView<const FFrameNumber> Times = TriggerChannel.GetTimes();
		for (int32 Index = 0; Index < Times.Num(); ++Index)
		{
			if (EffectiveRange.Contains(Times[Index]))
			{
				TRange<FFrameNumber> TriggerRange(Times[Index]);
				int32 EntityIndex = OutFieldBuilder->FindOrAddEntity(this, EncodeEntityID(TriggerIndex, EAudioSectionEntityType::TriggerEntity));
				OutFieldBuilder->AddOneShotEntity(TriggerRange, EntityIndex, MetaDataIndex);
			}
		}
	}*/

	// Return true to indicate we've done everything ourselves.
	return true;
}

void UMovieSceneAtomSection::ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity)
{
	using namespace UE::MovieScene;

	if (!Sound)
	{
		return;
	}

	const FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
	const FMovieSceneAtomComponentTypes* TrackComponents = FMovieSceneAtomComponentTypes::Get();

	const FGuid ObjectBindingID = Params.GetObjectBindingID();

	int32 EntityIndex;
	EAtomSectionEntityType EntityType;
	DecodeEntityID(Params.EntityID, EntityIndex, EntityType);

	if (EntityType == EAtomSectionEntityType::MainEntity)
	{
		// Default entity... we add the main Atom component data, plus the volume and pitch channels.
		OutImportedEntity->AddBuilder(
			FEntityBuilder()
			.AddConditional(BuiltInComponents->GenericObjectBinding, ObjectBindingID, ObjectBindingID.IsValid())
			.AddTagConditional(BuiltInComponents->Tags.Root, !ObjectBindingID.IsValid())
			.Add(TrackComponents->Atom, FMovieSceneAtomComponentData{ this })
			.Add(BuiltInComponents->FloatChannel[0], &SoundVolume)
			.Add(BuiltInComponents->FloatChannel[1], &PitchMultiplier)
		);
	}
	else if (EntityType == EAtomSectionEntityType::InputsEntity)
	{
		// Additional entities for custom audio input values.
		TArray<FName> InputNames;
		FMovieSceneAtomInputData InputData;

		// There are up to 9 float channels per entity, and we know that all entities are fully packed
		// until the last one. So add as many as we can for the given entity we're building.
		int32 FloatInputStartIndex = EntityIndex * 9;
		if (FloatInputStartIndex < Inputs_Float.Num())
		{
			int32 FloatInputNum = FMath::Min(9, Inputs_Float.Num() - FloatInputStartIndex);
			Inputs_Float.GetKeys(InputNames);
			for (int32 Offset = 0; Offset < FloatInputNum; ++Offset)
			{
				InputData.FloatInputs[Offset] = InputNames[FloatInputStartIndex + Offset];
			}
		}

		// Other inputs can only be added once per entity, so add one of each type that exists.
		int32 OtherInputStartIndex = EntityIndex;
		if (OtherInputStartIndex < Inputs_String.Num())
		{
			InputNames.Reset();
			Inputs_String.GetKeys(InputNames);
			InputData.StringInput = InputNames[OtherInputStartIndex];
		}
		if (OtherInputStartIndex < Inputs_Bool.Num())
		{
			InputNames.Reset();
			Inputs_Bool.GetKeys(InputNames);
			InputData.BoolInput = InputNames[OtherInputStartIndex];
		}
		if (OtherInputStartIndex < Inputs_Int.Num())
		{
			InputNames.Reset();
			Inputs_Int.GetKeys(InputNames);
			InputData.IntInput = InputNames[OtherInputStartIndex];
		}

		// Make this additional entity by adding the component that specifies what Atom input channels
		// are present, plus all of these channels.
		OutImportedEntity->AddBuilder(
			FEntityBuilder()
			.AddConditional(BuiltInComponents->GenericObjectBinding, ObjectBindingID, ObjectBindingID.IsValid())
			.AddTagConditional(BuiltInComponents->Tags.Root, !ObjectBindingID.IsValid())
			.Add(TrackComponents->Atom, FMovieSceneAtomComponentData{ this })
			.Add(TrackComponents->AtomInputs, InputData)
		);
		for (int32 Index = 0; Index < 9; ++Index)
		{
			FName InputName = InputData.FloatInputs[Index];
			if (!InputName.IsNone())
			{
				OutImportedEntity->AddBuilder(
					FEntityBuilder()
					.Add(BuiltInComponents->FloatChannel[Index], &Inputs_Float[InputName])
				);
			}
		}
		if (!InputData.StringInput.IsNone())
		{
			OutImportedEntity->AddBuilder(
				FEntityBuilder()
				.Add(BuiltInComponents->StringChannel, &Inputs_String[InputData.StringInput])
			);
		}
		if (!InputData.BoolInput.IsNone())
		{
			OutImportedEntity->AddBuilder(
				FEntityBuilder()
				.Add(BuiltInComponents->BoolChannel, &Inputs_Bool[InputData.BoolInput])
			);
		}
		if (!InputData.IntInput.IsNone())
		{
			OutImportedEntity->AddBuilder(
				FEntityBuilder()
				.Add(BuiltInComponents->IntegerChannel, &Inputs_Int[InputData.IntInput])
			);
		}
	}
	else if (EntityType == EAtomSectionEntityType::TriggerEntity)
	{
		// Additional one-shot entities for Atom triggers.
		// The decoded index is the index of the name in the triggers map.
		/*TArray<FName> AtomTriggerNames;
		Inputs_Trigger.GetKeys(AtomTriggerNames);
		AudioTriggerNames.Sort(FNameLexicalLess());
		OutImportedEntity->AddBuilder(
			FEntityBuilder()
			.AddConditional(BuiltInComponents->GenericObjectBinding, ObjectBindingID, ObjectBindingID.IsValid())
			.AddTagConditional(BuiltInComponents->Tags.Root, !ObjectBindingID.IsValid())
			.Add(TrackComponents->Atom, FMovieSceneAtomComponentData{ this })
			.Add(TrackComponents->AtomTriggerName, AtomTriggerNames[EntityIndex])
		);*/
	}
}

#if WITH_EDITOR

void UMovieSceneAtomSection::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(UMovieSceneAtomSection, AtomComponentClass))
	{
		CachedAtomComponentClass = AtomComponentClass;
	}
}

void UMovieSceneAtomSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	CacheChannelProxy();

	// crate the extension settings object if used by the component class
	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMovieSceneAtomSection, AtomComponentClass))
	{
		if (CachedAtomComponentClass != AtomComponentClass)
		{
			auto ExtensionClass = UAtomComponentExtensionSettings::FindExtensionSettingsClassForAtomComponent(AtomComponentClass);
			ExtensionSettings = ExtensionClass ? NewObject<UAtomComponentExtensionSettings>(this, ExtensionClass, NAME_None, RF_Public) : nullptr;
		}
	}
}

#endif

#if WITH_EDITOR
#undef LOCTEXT_NAMESPACE
#endif
