/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBus.cpp
 *
 ****************************************************************************/

#include "Atom/AtomBus.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif // WITH_EDITOR

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBusEffectPreset.h"

#define LOCTEXT_NAMESPACE "AtomBus"

 /*
  * FAtomChannelLevelMatrix Implementation
  *****************************************************************************/
FAtomChannelLevelMatrix::FAtomChannelLevelMatrix()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomChannelLevelMatrix::FAtomChannelLevelMatrix(int NumInputChannels, int NumOutputChannels)
{
	SetupChannelArrayFromNumChannels(InputChannelArray, NumInputChannels);
	SetupChannelArrayFromNumChannels(OutputChannelArray, NumOutputChannels);
	LevelMatrix.Init(0.0f, NumInputChannels * NumOutputChannels);
	SetIdentity();

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

bool FAtomChannelLevelMatrix::SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level)
{
	if ((int)InChannel >= InputChannelArray.Num() || (int)OutChannel >= OutputChannelArray.Num())
	{
		return false;
	}

	Level = FMath::Clamp(Level, 0.f, 1.f);

	int LevelIndex = (int)InChannel * OutputChannelArray.Num() + (int)OutChannel;
	check(LevelIndex < LevelMatrix.Num())

	LevelMatrix[LevelIndex] = Level;

	return true;
}

float FAtomChannelLevelMatrix::GetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel) const
{
	if ((int)InChannel >= InputChannelArray.Num() || (int)OutChannel >= OutputChannelArray.Num())
	{
		return 0.0f;
	}

	int LevelIndex = (int)InChannel * OutputChannelArray.Num() + (int)OutChannel;
	check(LevelIndex < LevelMatrix.Num())

	return LevelMatrix[LevelIndex];
}

void FAtomChannelLevelMatrix::SetIdentity()
{
	LevelMatrix.Empty();

	for (int OutChannel = 0; OutChannel < OutputChannelArray.Num(); ++OutChannel)
	{
		for (int InChannel = 0; InChannel < InputChannelArray.Num(); ++InChannel)
		{
			LevelMatrix.Add(InChannel == OutChannel ? 1.0f : 0.0f);
		}
	}
}

void FAtomChannelLevelMatrix::UpMix()
{}

void FAtomChannelLevelMatrix::DownMix()
{}

void FAtomChannelLevelMatrix::SetupChannelArrayFromNumChannels(TArray<EAtomSpeaker>& ChannelArray, int NumChannels)
{
	for (int i = 0; i < NumChannels && i < (int)EAtomSpeaker::Count; ++i)
	{
		ChannelArray.Add(static_cast<EAtomSpeaker>(i));
	}
}

/*
 * FAtomPan Implementation
 *****************************************************************************/

FAtomPanning::FAtomPanning()
	: Volume(1.0f)
	, Angle(0.0f)
	, Distance(1.0f)
	, Wideness(1.0f)
	, Spread(0.0f)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomPanning::FAtomPanning(float InVolume, float InAngle, float InDistance, float InWideness, float InSpread)
	: Volume(InVolume)
	, Angle(InAngle)
	, Distance(InDistance)
	, Wideness(InWideness)
	, Spread(InSpread)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomBusSend Implementation
 *****************************************************************************/

FAtomBusSend::FAtomBusSend()
	: DestinationBus(nullptr)
	, Level(0.0f)
#if WITH_EDITORONLY_DATA
	, SendType(EAtomBusSendType::PostPanning)
#endif
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomBusSend::FAtomBusSend(const UAtomBus* InDestinationBus, float InLevel /* = 0.0f */, EAtomBusSendType SendType /* = EAtomBusSendType::PostPanning*/)
	: DestinationBus(InDestinationBus)
	, Level(InLevel)
#if WITH_EDITORONLY_DATA
	, SendType(SendType)
#endif
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/**
 * Mixer2 endpoint
 *****************************************************************************/
UAtomEndpointBase::UAtomEndpointBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAtomEndpoint::UAtomEndpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SoundRendererType(EAtomSoundRendererType::Default)
{

}

const UAtomEndpointSettingsBase* UAtomEndpoint::GetEndpointSettings(const TSubclassOf<UAtomEndpointSettingsBase>& EndpointSettingsClass) const
{
	for (auto& Settings : EndpointSettings)
	{
		if (Settings && EndpointSettingsClass == Settings->GetClass())
		{
			return Settings;
		}
	}

	return nullptr;
}

#if WITH_EDITORONLY_DATA
void UAtomEndpoint::Serialize(FArchive& Ar)
{
	if (Ar.IsCooking())
	{
#if WITH_ENGINE
		if (const ITargetPlatform* CookingTarget = Ar.CookingTarget())
		{
			// remove unused settings.
			TArray<TObjectPtr<UAtomEndpointSettingsBase>> NewEndpointSettings;
			for (auto Settings : EndpointSettings)
			{
				if (Settings && Settings->IsValidForPlatform(CookingTarget->PlatformName()))
				{
					NewEndpointSettings.Add(Settings);
				}
			}

			EndpointSettings = NewEndpointSettings;
		}
#endif
	}

	Super::Serialize(Ar);
}
#endif

UAtomSoundfieldEndpoint::UAtomSoundfieldEndpoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SoundfieldRendererType(EAtomSoundfieldRendererType::Default)
{

}

/*
 * UAtomBus Implementation
 *****************************************************************************/

UAtomBus::UAtomBus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EnvelopeFollowerAttackTime(10)
	, EnvelopeFollowerReleaseTime(500)
	, SpatializationType(EAtomSpatializationType::Panning)
	, ChannelLevelMatrix(12, 12)
{
	OutputVolumeModulation.Value = 0.0f;
	WetLevelModulation.Value = 0.0f;
	DryLevelModulation.Value = -96.0f;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomBus::Init(const FString& InBusName, int InBusIndex)
{
	BusName = InBusName;
	BusIndex = InBusIndex;
}

bool UAtomBus::SetEffectPreset(UAtomBusEffectPreset* InEffectPreset)
{
	// check if valid effect
	int32 Index;
	if (InEffectPreset && BusEffectChainClasses.Find(InEffectPreset->GetClass(), Index))
	{
		if (BusEffectChain.IsValidIndex(Index))
		{
			// link to this bus (TSet)
			InEffectPreset->AddTargetBus(this);
			BusEffectChain[Index] = InEffectPreset;

			// apply settings
			ApplyEffectPreset(InEffectPreset);

			return true;
		}
	}

	return false;
}

bool UAtomBus::RemoveEffectPreset(UAtomBusEffectPreset* InEffectPreset)
{
	int32 EffectIndex = FindEffectPreset(InEffectPreset);
	if (EffectIndex != INDEX_NONE)
	{
		return SetDefaultEffectPreset(EffectIndex);
	}

	return false;
}

bool UAtomBus::SetDefaultEffectPreset(int32 InEffectIndex)
{
	if (BusEffectChain.IsValidIndex(InEffectIndex))
	{
		if (auto EffectPreset = BusEffectChain[InEffectIndex])
		{
			EffectPreset->RemoveTargetBus(this);
		}

		// create default effect preset
		UClass* EffectPresetClass = BusEffectChainClasses[InEffectIndex];

		if (auto EffectPreset = UAtomBusEffectPreset::CreateBusEffectPreset(this, EffectPresetClass))
		{
			BusEffectChain[InEffectIndex] = EffectPreset;
		}
		else
		{
			BusEffectChain[InEffectIndex] = nullptr;
		}

		return true;
	}

	return false;
}

int32 UAtomBus::FindEffectPreset(UAtomBusEffectPreset* InEffectPreset) const
{
	return BusEffectChain.Find(InEffectPreset);
}

void UAtomBus::PostInitProperties()
{
	Super::PostInitProperties();
}

#if WITH_EDITOR
/*bool UAtomBus::CanEditChange(const FProperty* InProperty) const
{
	return true;
}*/

void UAtomBus::PreEditChange(FProperty* PropertyAboutToChange)
{
	if (PropertyAboutToChange && PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomBus, BusEffectChain))
	{
		// Take a copy of the current state of BusEffectChain
		BackupBusEffectChain = BusEffectChain;
	}
}

void UAtomBus::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
	{
		const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, SpatializationType))
		{
			if (SpatializationType == EAtomSpatializationType::SendToChannel)
			{
				ApplyChannelLevelMatrix();
			}
			else
			{
				ApplyPanning(); // default
			}
		}
		else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, Panning))
		{
			ApplyPanning();
		}
		else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, OutputVolumeModulation)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, WetLevelModulation)
			|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, DryLevelModulation))
		{
			UAtomBus* AtomBus = this;

			PushModulationChanges();

			float NewVolumeModBase = OutputVolumeModulation.Value;
			float NewWetModBase = WetLevelModulation.Value;
			float NewDryModBase = DryLevelModulation.Value;

			AtomRuntimeManager->IterateOverAllRuntimes([AtomBus, NewVolumeModBase, NewWetModBase, NewDryModBase](FAtomRuntimeId ID, FAtomRuntime* Runtime)
			{
				Runtime->SetBusModulationBaseLevels(AtomBus, NewVolumeModBase, NewWetModBase, NewDryModBase);
			});
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, BusEffectChain))
		{
			// Find preset that was changed/added
			for (int32 EffectIndex = 0; EffectIndex < BusEffectChain.Num(); EffectIndex++)
			{
				check(BusEffectChainClasses.IsValidIndex(EffectIndex));

				auto BackupEffectPreset = BackupBusEffectChain.IsValidIndex(EffectIndex) ? BackupBusEffectChain[EffectIndex] : nullptr;

				// if preset has changed
				if (BusEffectChain[EffectIndex] != BackupEffectPreset)
				{
					if (auto& AddedEffectPreset = BusEffectChain[EffectIndex])
					{
						// check if valid preset class for this slot
						if (AddedEffectPreset->IsA(BusEffectChainClasses[EffectIndex]))
						{
							// setup new preset
							AddedEffectPreset->TargetBuses.Add(this);
							ApplyEffectPreset(AddedEffectPreset);
						}
						else
						{
							// invalid preset set to this index - launch notification to inform user
							FNotificationInfo Info(NSLOCTEXT("Engine", "UnableToChangeAtomBusEffectPresetDueToInvalidEffectType", "Could not change AtomBusEffectPreset with this effect preset type at this index."));
							Info.ExpireDuration = 5.0f;
							Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
							FSlateNotificationManager::Get().AddNotification(Info);

							// Revert to the previous preset
							BusEffectChain[EffectIndex] = BackupEffectPreset;
							continue;
						}

						// remove old preset if new value was set
						if (BackupEffectPreset)
						{
							BackupEffectPreset->TargetBuses.Remove(this);
						}
					}
					else
					{
						// reset to default in case AddedEffectPreset is nullptr
						SetDefaultEffectPreset(EffectIndex);

						auto DefaultEffectPreset = BusEffectChain[EffectIndex];
						check(DefaultEffectPreset);

						SetEditorEffectSettings(DefaultEffectPreset);

						if (DefaultEffectPreset)
						{
							// setup new preset
							DefaultEffectPreset->TargetBuses.Add(this);
							ApplyEffectPreset(DefaultEffectPreset);
						}
					}
				}
			}
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, bOverrideEndpoint)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, Endpoint))
		{
			if (auto AtomRuntime = AtomRuntimeManager->GetActiveAtomRuntime())
			{
				if (AtomRuntime->IsAtomMixerPluginEnabled())
				{
					if (!bOverrideEndpoint)
					{
						ApplyEditorEndpoint();
					}
					else
					{
						ApplyEndpoint(Endpoint);
					}
				}
			}
		}

		BackupBusEffectChain.Reset();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

TArray<TObjectPtr<UAtomBusEffectPreset>> UAtomBus::BackupBusEffectChain;

void UAtomBus::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	FProperty* ModifiedProperty = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName ModifiedPropertyName = ModifiedProperty ? ModifiedProperty->GetFName() : NAME_None;

	if (ModifiedPropertyName == GET_MEMBER_NAME_CHECKED(UAtomBus, Sends)
			 && PropertyName == GET_MEMBER_NAME_CHECKED(FAtomBusSend, Level))
	{
		const int32 ChangedIndex = PropertyChangedEvent.GetArrayIndex(ModifiedPropertyName.ToString());
		if (Sends.IsValidIndex(ChangedIndex))
		{
			auto& Send = Sends[ChangedIndex];
			ApplySendLevel(Send);
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UAtomBus::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UAtomBus* This = CastChecked<UAtomBus>(InThis);

	for (auto& Backup : This->BackupBusEffectChain)
	{
		Collector.AddReferencedObject(Backup);
	}

	Super::AddReferencedObjects(InThis, Collector);
}
#endif

void UAtomBus::StartEnvelopeFollowing(const UObject* WorldContextObject)
{
	// Find runtime for this specific EnvelopeFollowing thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StartEnvelopeFollowing(AtomRuntime);
	}
}

void UAtomBus::StartEnvelopeFollowing(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StartEnvelopeFollowing(this);
	}
}

void UAtomBus::StopEnvelopeFollowing(const UObject* WorldContextObject)
{
	// Find runtime for this specific EnvelopeFollowing thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StopEnvelopeFollowing(AtomRuntime);
	}
}

void UAtomBus::StopEnvelopeFollowing(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StopEnvelopeFollowing(this);
	}
}

void UAtomBus::AddEnvelopeFollowerDelegate(const UObject* WorldContextObject, const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP)
{
	// Find runtime for this specific audio recording thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddEnvelopeFollowerDelegate(this, OnAtomBusEnvelopeBP);
	}
}

void UAtomBus::AddSpectralAnalysisDelegate(const UObject* WorldContextObject, const TArray<FAtomBusSpectralAnalysisBandSettings>& InBandSettings, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP, float UpdateRate, float DecibelNoiseFloor, bool bDoNormalize, bool bDoAutoRange, float AutoRangeAttackTime, float AutoRangeReleaseTime)
{
	// Find runtime for this specific audio recording thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		FAtomSoundSpectrumAnalyzerDelegateSettings DelegateSettings = UAtomBus::GetSpectrumAnalysisDelegateSettings(InBandSettings, UpdateRate, DecibelNoiseFloor, bDoNormalize, bDoAutoRange, AutoRangeAttackTime, AutoRangeReleaseTime);
		AtomRuntime->AddSpectralAnalysisDelegate(this, DelegateSettings, OnAtomBusSpectralAnalysisBP);
	}
}

void UAtomBus::RemoveSpectralAnalysisDelegate(const UObject* WorldContextObject, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP)
{
	// Find runtime for this specific audio recording thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveSpectralAnalysisDelegate(this, OnAtomBusSpectralAnalysisBP);
	}
}

void UAtomBus::StartSpectralAnalysis(const UObject* WorldContextObject, EAtomFFTSize FFTSize, EAtomFFTPeakInterpolationMethod InterpolationMethod, EAtomFFTWindowType WindowType, float HopSize, EAtomSpectrumType SpectrumType)
{
	// Find runtime for this specific audio recording thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StartSpectralAnalysis(AtomRuntime, FFTSize, InterpolationMethod, WindowType, HopSize, SpectrumType);
	}
}

void UAtomBus::StartSpectralAnalysis(FAtomRuntime* InAtomRuntime, EAtomFFTSize FFTSize, EAtomFFTPeakInterpolationMethod InterpolationMethod, EAtomFFTWindowType WindowType, float HopSize, EAtomSpectrumType SpectrumType)
{
	// Find runtime for this specific audio recording thing.
	if (InAtomRuntime)
	{
		FAtomSoundSpectrumAnalyzerSettings Settings = UAtomBus::GetSpectrumAnalyzerSettings(FFTSize, InterpolationMethod, WindowType, HopSize, SpectrumType);
		InAtomRuntime->StartSpectrumAnalysis(this, Settings);
	}
}

void UAtomBus::StopSpectralAnalysis(const UObject* WorldContextObject)
{
	// Find runtime for this specific audio recording thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StopSpectralAnalysis(AtomRuntime);
	}
}

void UAtomBus::StopSpectralAnalysis(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StopSpectrumAnalysis(this);
	}
}

void UAtomBus::SetBusOutputVolume(const UObject* WorldContextObject, float InOutputVolume)
{
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->SetSubmixOutputVolume(this, InOutputVolume);
	}
}

float UAtomBus::GetBusOutputVolume(const UObject* WorldContextObject) const
{
	// get from asr
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		return AtomRuntime->GetBusVolume(this);
	}

	return 0.0f;
}

void UAtomBus::SetBusWetLevel(const UObject* WorldContextObject, float InWetLevel)
{
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->SetSubmixWetLevel(this, InWetLevel);
	}
}

float UAtomBus::GetBusWetLevel() const
{
	return 0.0f;
}

void UAtomBus::SetBusDryLevel(const UObject* WorldContextObject, float InDryLevel)
{
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->SetSubmixDryLevel(this, InDryLevel);
	}
}

float UAtomBus::GetBusDryLevel() const
{
	return 0.0f;
}

void UAtomBus::SetOutputVolumeModulation(const FAtomSoundModulationSettings& InVolMod)
{
	OutputVolumeModulation = InVolMod;
	PushModulationChanges();
}

void UAtomBus::SetWetVolumeModulation(const FAtomSoundModulationSettings& InVolMod)
{
	WetLevelModulation = InVolMod;
	PushModulationChanges();
}

void UAtomBus::SetDryVolumeModulation(const FAtomSoundModulationSettings& InVolMod)
{
	DryLevelModulation = InVolMod;
	PushModulationChanges();
}

void UAtomBus::PushModulationChanges()
{
	if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
	{
		// Send the changes to the Modulation System
		TSet<TObjectPtr<UAtomModulatorBase>> NewVolumeMod = OutputVolumeModulation.Modulators;
		TSet<TObjectPtr<UAtomModulatorBase>> NewWetLevelMod = WetLevelModulation.Modulators;
		TSet<TObjectPtr<UAtomModulatorBase>> NewDryLevelMod = DryLevelModulation.Modulators;
		AtomRuntimeManager->IterateOverAllRuntimes([AtomBus = this, VolMod = MoveTemp(NewVolumeMod), WetMod = MoveTemp(NewWetLevelMod), DryMod = MoveTemp(NewDryLevelMod)](FAtomRuntimeId ID, FAtomRuntime* Runtime) mutable
		{
			if (Runtime)
			{
				Runtime->UpdateBusModulationSettings(AtomBus, VolMod, WetMod, DryMod);
			}
		});
	}
}

void UAtomBus::ApplyVolume(const UObject* WorldContextObject, float InOutputVolume) const
{
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->SetBusVolume(this, InOutputVolume);
	}
}

void UAtomBus::SetPanning(const FAtomPanning& PanningSetting)
{
	Panning = PanningSetting;
	ApplyPanning();
}

FAtomPanning UAtomBus::GetPanning() const
{
	// get from asr
	/*if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		FAtomPanning AsrPanning;
		Atom->GetBusPanning(this, AsrPanning);
		return AsrPanning;
	}
	*/

	return Panning;
}

void UAtomBus::ApplyPanning() const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusPanning(this);
		}
	}
}

void UAtomBus::SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level)
{
	ChannelLevelMatrix.SetChannelLevel(InChannel, OutChannel, Level);
	ApplyChannelLevelMatrix();
}

float UAtomBus::GetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel)
{
	return ChannelLevelMatrix.GetChannelLevel(InChannel, OutChannel);
}

void UAtomBus::ApplyChannelLevelMatrix() const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusChannelLevelMatrix(this);
		}
	}
}

void UAtomBus::ResetChannelLevelMatrix()
{
	ChannelLevelMatrix.SetIdentity();
	ApplyChannelLevelMatrix();
}

void UAtomBus::SetSendLevel(int SendIndex, float Level)
{
	if (Sends.IsValidIndex(SendIndex))
	{
		Sends[SendIndex].Level = Level;
		ApplySendLevel(Sends[SendIndex]);
	}
}

float UAtomBus::GetSendLevel(int SendIndex)
{
	if (Sends.IsValidIndex(SendIndex))
	{
		return Sends[SendIndex].Level;
	}

	return 0.0f;
}

void UAtomBus::ApplySendLevel(const FAtomBusSend& InSend) const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusSendLevel(this, InSend);
		}
	}
}

void UAtomBus::ApplyEffectParameter(const UAtomBusEffectPreset* EffectPreset, int ParameterIndex) const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusEffectParameter(this, EffectPreset, ParameterIndex);
		}
	}
}

void UAtomBus::ApplyEffectBypass(const UAtomBusEffectPreset* EffectPreset) const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusEffectBypass(this, EffectPreset);
		}
	}
}

void UAtomBus::ApplyEffectPreset(const UAtomBusEffectPreset* EffectPreset) const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetBusEffectPreset(this, EffectPreset);
		}
	}
}

void UAtomBus::ApplyEndpoint(const UAtomEndpointBase* InEndpoint) const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->SetSubmixEndpoint(this, InEndpoint);
		}
	}
}

void UAtomBus::ApplyAll() const
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			//AtomRuntime->SetBusVolume(this);
			if (SpatializationType == EAtomSpatializationType::SendToChannel)
			{
				AtomRuntime->SetBusChannelLevelMatrix(this);
			}
			else
			{
				AtomRuntime->SetBusPanning(this);
			}
			for (auto& Send : Sends)
			{
				AtomRuntime->SetBusSendLevel(this, Send);
			}
			for (auto EffectPreset : BusEffectChain)
			{
				AtomRuntime->SetBusEffectPreset(this, EffectPreset);
			}

			if (AtomRuntime->IsAtomMixerPluginEnabled())
			{
				if (bOverrideEndpoint)
				{
					ApplyEndpoint(Endpoint);
				}
#if WITH_EDITORONLY_DATA
				else // reset to default while editing
				{
					ApplyEditorEndpoint();
				}
#endif
			}
		}
	}
}

void UAtomBus::SyncAll()
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			//Volume = AtomRuntime->GetBusVolume(this);
			if (SpatializationType == EAtomSpatializationType::SendToChannel)
			{
				// nothing to get
			}
			else
			{
				AtomRuntime->GetBusPanning(this, Panning);
			}
			for (auto& Send : Sends)
			{
				// nothing to get
			}
			for (auto EffectPreset : BusEffectChain)
			{
				AtomRuntime->GetBusEffectPreset(this, EffectPreset);
			}
		}
	}
}

void UAtomBus::ResetSends()
{
	Sends.Empty();

	// Do not create sends for master out bus
	if (IsMainBus())
	{
		return;
	}

	UAtomRack* Rack = Cast<UAtomRack>(GetRack());
	if (Rack)
	{
		for (auto Bus : Rack->Buses)
		{
			// Avoid send to itself
			if (Bus->BusIndex != BusIndex)
			{
				// By default do not output to another bus
				Sends.Add(FAtomBusSend(Bus));

				// Output to master bus
				//const float Level = BusName == UAtomRack::MasterBusName ? 1.0f : 0.0f;
			}
			/*else
			{
				Sends.Add(FAtomBusSend(this), 1.0f);
			}*/
		}
	}
}

void UAtomBus::ResetEffectChain(bool bUseRuntimeDefaults /* = false */)
{
	// Clear effects
	for (auto EffectPreset : BusEffectChain)
	{
		if (EffectPreset)
		{
			EffectPreset->TargetBuses.Remove(this);
		}
	}

	BusEffectChainClasses.Empty();
	BusEffectChain.Empty();

	// Recreate effect slots
	UAtomRack* Rack = Cast<UAtomRack>(GetRack());
	if (Rack && Rack->DspBusSetting)
	{
		// BusIndex should not have changed
		check(Rack->DspBusSetting->GetBusName(BusIndex) == BusName);

		// Setup effects
		for (int EffectIndex = 0; EffectIndex < Rack->DspBusSetting->GetNumBusEffects(BusIndex); EffectIndex++)
		{
			const FName EffectName = Rack->DspBusSetting->GetBusEffectName(BusIndex, EffectIndex);
			const bool bEffectBypassed = false;
			AddEffect(EffectName, bEffectBypassed, bUseRuntimeDefaults);
		}
	}
}

int UAtomBus::AddEffect(const FName& InEffectName, bool bIsBypassed, bool bUseRuntimeValues)
{
	// create default effect preset
	UAtomBusEffectPreset* EffectPreset = UAtomBusEffectPreset::CreateBusEffectPreset(this, InEffectName);

	if (EffectPreset)
	{
		// add slot
		BusEffectChainClasses.Add(EffectPreset->GetClass());
		int32 Index = BusEffectChain.Add(nullptr);

		// setup the default effect preset
		EffectPreset->Init();
		SetEffectPreset(EffectPreset);

		if (bUseRuntimeValues)
		{
			// setup with atom current values
			if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
			{
				AtomRuntime->GetBusEffectPreset(this, EffectPreset);
				EffectPreset->SetBypass(bIsBypassed);
			}
		}

		return Index;
	}

	return INDEX_NONE;
}

UAtomRackBase* UAtomBus::GetRack() const
{
	return Cast<UAtomRackBase>(GetOuter());
}

FAtomSoundSpectrumAnalyzerSettings UAtomBus::GetSpectrumAnalyzerSettings(EAtomFFTSize FFTSize, EAtomFFTPeakInterpolationMethod InterpolationMethod, EAtomFFTWindowType WindowType, float HopSize, EAtomSpectrumType SpectrumType)
{
	FAtomSoundSpectrumAnalyzerSettings OutSettings;

	OutSettings.FFTSize = FFTSize;
	OutSettings.WindowType = WindowType;
	OutSettings.InterpolationMethod = InterpolationMethod;
	OutSettings.SpectrumType = SpectrumType;

	const float MinHopSize = 0.001f;
	const float MaxHopSize = 10.0f;
	OutSettings.HopSize = FMath::Clamp(HopSize, MinHopSize, MaxHopSize);

	return OutSettings;
}

FAtomSoundSpectrumAnalyzerDelegateSettings UAtomBus::GetSpectrumAnalysisDelegateSettings(const TArray<FAtomBusSpectralAnalysisBandSettings>& InBandSettings, float UpdateRate, float DecibelNoiseFloor, bool bDoNormalize, bool bDoAutoRange, float AutoRangeAttackTime, float AutoRangeReleaseTime)
{
	FAtomSoundSpectrumAnalyzerDelegateSettings DelegateSettings;

	DelegateSettings.BandSettings = InBandSettings;
	DelegateSettings.UpdateRate = UpdateRate;
	DelegateSettings.DecibelNoiseFloor = DecibelNoiseFloor;
	DelegateSettings.bDoNormalize = bDoNormalize;
	DelegateSettings.bDoAutoRange = bDoAutoRange;
	DelegateSettings.AutoRangeAttackTime = AutoRangeAttackTime;
	DelegateSettings.AutoRangeReleaseTime = AutoRangeReleaseTime;

	return DelegateSettings;
}

void UAtomBus::Reset(bool bApplyToAtom /* = true */)
{
	OutputVolumeModulation.Value = 0.0f;
	SpatializationType = EAtomSpatializationType::Panning;
	Panning = FAtomPanning();
	ChannelLevelMatrix.SetIdentity();
	ResetSends();
	ResetEffectChain(true);
	// preset are generated using the value actually in the runtime so values from acf data.

#if WITH_EDITORONLY_DATA
	// Setup default data cached from acf data to reflect values in editor UI
	UAtomRack* Rack = Cast<UAtomRack>(GetRack());
	if (Rack && Rack->DspBusSetting)
	{
		const auto& DspSetting = Rack->DspBusSetting->GetEditorDspSetting();

		// Look up for bus info then setup to AtomBus
		const auto BusInfoPtr = DspSetting.Buses.FindByPredicate([this](const FAtomBusInfo& Info)
		{
			return GetBusName() == Info.Name;
		});

		if (BusInfoPtr)
		{
			const FAtomBusInfo& BusInfo = *BusInfoPtr;
			OutputVolumeModulation.Value = Atom::ConvertToDecibels(BusInfo.Volume);
			Panning = BusInfo.Panning;

			for (auto& Send : Sends)
			{
				// Look up for bus send info then setup to AtomBus's sends 
				const auto BusSendInfoPtr = BusInfo.Sends.FindByPredicate([Send](const FAtomBusSendInfo& Info)
				{
					return Send.DestinationBus ? Send.DestinationBus->GetBusName() == Info.SendToBusName : false;
				});

				if (BusSendInfoPtr)
				{
					const FAtomBusSendInfo& BusSendInfo = *BusSendInfoPtr;
					Send.Level = BusSendInfo.Level;
					Send.SendType = BusSendInfo.SendType;
				}
			}

			for (auto& EffectPreset : BusEffectChain)
			{
				if (EffectPreset)
				{
					// Look up for bus effect then add them to AtomBus's effects.
					const auto BusEffectInfoPtr = BusInfo.Effects.FindByPredicate([EffectPreset](const FAtomBusEffectInfo& Info)
					{
						return EffectPreset->GetEffectName().ToString() == Info.BusEffectName;
					});

					if (BusEffectInfoPtr)
					{
						// Setup dynamic settings.
						const FAtomBusEffectInfo& BusEffectInfo = *BusEffectInfoPtr;
						for (int ParamIndex = 0; ParamIndex < BusEffectInfo.Parameters.Num(); ++ParamIndex)
						{
							EffectPreset->SetParameterValue(ParamIndex, BusEffectInfo.Parameters[ParamIndex]);
						}
						EffectPreset->SetBypass(BusEffectInfo.bBypass);

						// Apply dynamic change to defaults settings of the preset.
						EffectPreset->ApplyToDefautlsSettings();
					}
				}
			}

			// Mixer2 endpoint settings
			if (GCriWare)
			{
				if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
				{
					if (AtomRuntime->IsAtomMixerPluginEnabled())
					{
						if (!bOverrideEndpoint)
						{
							if (BusInfo.SoundfieldRendererType != EAtomSoundfieldRendererType::Default)
							{
								if (UAtomSoundfieldEndpoint* NewEndpoint = NewObject<UAtomSoundfieldEndpoint>(this))
								{
									NewEndpoint->SoundfieldRendererType = BusInfo.SoundfieldRendererType;
									Endpoint = NewEndpoint;
								}
							}
							else
							{
								if (UAtomEndpoint* NewEndpoint = NewObject<UAtomEndpoint>(this))
								{
									NewEndpoint->SoundRendererType = BusInfo.SoundRendererType;
									NewEndpoint->SpeakerChannelMap = BusInfo.SpeakerChannelMap;
									Endpoint = NewEndpoint;
								}
							}
						}
					}
				}
			}
		}
	}
#endif

	if (bApplyToAtom)
	{
		ApplyAll();
	}
}

#if WITH_EDITORONLY_DATA
void UAtomBus::SetEditorEffectSettings(UAtomBusEffectPreset* EffectPreset)
{
	check(EffectPreset);

	UAtomRack* Rack = Cast<UAtomRack>(GetRack());
	if (Rack && Rack->DspBusSetting)
	{
		const auto& DspSetting = Rack->DspBusSetting->GetEditorDspSetting();

		// Look up for bus info then setup to AtomBus
		const auto BusInfoPtr = DspSetting.Buses.FindByPredicate([this](const FAtomBusInfo& Info)
		{
			return GetBusName() == Info.Name;
		});

		// Look up for bus effect then add them to AtomBus's effects.
		const auto BusEffectInfoPtr = BusInfoPtr->Effects.FindByPredicate([EffectPreset](const FAtomBusEffectInfo& Info)
		{
			return EffectPreset->GetEffectName().ToString() == Info.BusEffectName;
		});

		if (BusEffectInfoPtr)
		{
			// Setup dynamic settings.
			const FAtomBusEffectInfo& BusEffectInfo = *BusEffectInfoPtr;
			for (int ParamIndex = 0; ParamIndex < BusEffectInfo.Parameters.Num(); ++ParamIndex)
			{
				EffectPreset->SetParameterValue(ParamIndex, BusEffectInfo.Parameters[ParamIndex]);
			}
			EffectPreset->SetBypass(BusEffectInfo.bBypass);

			// Apply dynamic changes to default settings of the preset.
			EffectPreset->ApplyToDefautlsSettings();
		}
	}
}

void UAtomBus::ApplyEditorEndpoint() const
{
	UAtomRack* Rack = Cast<UAtomRack>(GetRack());
	if (Rack && Rack->DspBusSetting)
	{
		const auto& DspSetting = Rack->DspBusSetting->GetEditorDspSetting();

		// Look up for bus info then setup to AtomBus
		const auto BusInfoPtr = DspSetting.Buses.FindByPredicate([this](const FAtomBusInfo& Info) { return GetBusName() == Info.Name; });
		if (BusInfoPtr)
		{
			const FAtomBusInfo& BusInfo = *BusInfoPtr;
			if (BusInfo.SoundfieldRendererType != EAtomSoundfieldRendererType::Default)
			{
				if (UAtomSoundfieldEndpoint* NewEndpoint = NewObject<UAtomSoundfieldEndpoint>(const_cast<UAtomBus*>(this)))
				{
					NewEndpoint->SoundfieldRendererType = BusInfo.SoundfieldRendererType;
					ApplyEndpoint(NewEndpoint);
				}
			}
			else
			{
				if (UAtomEndpoint* NewEndpoint = NewObject<UAtomEndpoint>(const_cast<UAtomBus*>(this)))
				{
					NewEndpoint->SoundRendererType = BusInfo.SoundRendererType;
					NewEndpoint->SpeakerChannelMap = BusInfo.SpeakerChannelMap;
					ApplyEndpoint(NewEndpoint);
				}
			}
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
