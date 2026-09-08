/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Module Settings for CriWare
* File     : CriWareCoreSettings.cpp
*
****************************************************************************/

#include "CriWareCoreSettings.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Runtime/Launch/Resources/Version.h"
#endif

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomCustomVersion.h"
#include "Atom/Modulation/AtomModulationParameter.h"

#define LOCTEXT_NAMESPACE "CriWareCoreSettings"

/* VoicesSettings functions
 *****************************************************************************/

void FAtomVoicesSettings::OverridesWith(const FAtomVoicesSettings& Settings)
{
	if (Settings.NumVoices > 0)
		NumVoices = Settings.NumVoices;
	if (Settings.NumChannels > 0)
		NumChannels = Settings.NumChannels;
	if (Settings.SamplingRate > 0)
		SamplingRate = Settings.SamplingRate;
}

void FCriWareStandardVoicesSettings::OverridesWith(const FCriWareStandardVoicesSettings& Settings)
{
	if (Settings.NumStandardMemoryVoices > 0)
		NumStandardMemoryVoices = Settings.NumStandardMemoryVoices;
	if (Settings.StandardMemoryVoiceNumChannels > 0)
		StandardMemoryVoiceNumChannels = Settings.StandardMemoryVoiceNumChannels;
	if (Settings.StandardMemoryVoiceSamplingRate > 0)
		StandardMemoryVoiceSamplingRate = Settings.StandardMemoryVoiceSamplingRate;
	if (Settings.NumStandardStreamingVoices > 0)
		NumStandardMemoryVoices = Settings.NumStandardStreamingVoices;
	if (Settings.StandardStreamingVoiceNumChannels > 0)
		StandardStreamingVoiceNumChannels = Settings.StandardStreamingVoiceNumChannels;
	if (Settings.StandardStreamingVoiceSamplingRate > 0)
		StandardStreamingVoiceSamplingRate = Settings.StandardStreamingVoiceSamplingRate;
}

void FCriWareHcaMxVoicesSettings::OverridesWith(const FCriWareHcaMxVoicesSettings& Settings)
{
	if (Settings.NumHcaMxMemoryVoices > 0)
		NumHcaMxMemoryVoices = Settings.NumHcaMxMemoryVoices;
	if (Settings.HcaMxMemoryVoiceNumChannels > 0)
		HcaMxMemoryVoiceNumChannels = Settings.HcaMxMemoryVoiceNumChannels;
	if (Settings.NumHcaMxStreamingVoices > 0)
		NumHcaMxStreamingVoices = Settings.NumHcaMxStreamingVoices;
	if (Settings.HcaMxStreamingVoiceNumChannels > 0)
		HcaMxStreamingVoiceNumChannels = Settings.HcaMxStreamingVoiceNumChannels;
	if (Settings.HcaMxVoiceSamplingRate > 0)
		HcaMxVoiceSamplingRate = Settings.HcaMxVoiceSamplingRate;
}

/* UCriWareCoreSettings structors
 *****************************************************************************/

UCriWareCoreSettings::UCriWareCoreSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("ADX Atom");

	// File System
	bUseAutomaticFileSystemManagement = true;
	NumBinders = 64;
	MaxBindings = 64;
	NumLoaders = 64;
	bEnableLoggingFileSystem = false;

	// Atom
	AtomConfig = nullptr;
	bUseInGamePreview = false;
	MonitorCommunicationBufferSize = 2 * 1024 * 1024; // 2MB
	MaxPitch = 2400.0f;
	DistanceFactor = 0.01f; // meter
	MaxSoundSources = 64;
	NumReservedSoundSources = 8;
	bEnableLoggingAtom = false;

	// Atom - Sound Renderer
	bEnableBinauralSpatialization = false;
	bUseAudioLink = false;
	bUseUnrealSoundRenderer = false;
	// Atom - Sound Renderer - Mixer
	bAllowCenterChannel3DPanning = false;

	// Atom - Voices
	bUseAutomaticVoiceManagement = false;
	VoicesSettings.NumVoices = 64;
	VoicesSettings.NumChannels = CRIATOM_DEFAULT_INPUT_MAX_CHANNELS;
	VoicesSettings.SamplingRate = CRIATOM_DEFAULT_INPUT_MAX_SAMPLING_RATE;

	// Atom - Decoding
	bEnableHcaMxDecoding = false;
	HcaMxSettings.MaxVoices = 64;
	HcaMxSettings.MaxSamplingRate = CRIATOM_DEFAULT_INPUT_MAX_SAMPLING_RATE;
	HcaMxSettings.OutputSamplingRate = CRIATOM_DEFAULT_OUTPUT_SAMPLING_RATE;

	// Hides Mana class settings for LE
#ifndef CRIWARE_UE_LE
	bCanEditManaClassName = true;
#else
	bCanEditManaClassName = false;
#endif

#if WITH_EDITOR
	if (FModuleManager::Get().IsModuleLoaded("CriWareAtomMixer"))
	{
		if (UClass* Class = GetClass())
		{
			if (FProperty* Property = Class->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, SpatializationRack)))
			{
				Property->SetMetaData(TEXT("AllowedClasses"), TEXT("/Script/CriWareCore.AtomBus"));
				Property->SetMetaData(TEXT("DisplayName"), TEXT("Spatialization Bus"));
				Property->SetMetaData(TEXT("Tooltip"), TEXT("Specifies the Atom Bus to use for sound routed to binaural spatialization."));
			}
		}
	}
#endif
}

//~ UDeveloperSettings interface
#if WITH_EDITOR
FText UCriWareCoreSettings::GetSectionText() const
{ 
	return LOCTEXT("CriWareCoreSettingsSectionText", "ADX Atom");
}

FText UCriWareCoreSettings::GetSectionDescription() const
{
	return LOCTEXT("CriWareCoreSettingsSectionDescription", "Settings for CriWare FileSystem and ADX Atom.");
}
#endif

//~ UObject interface
void UCriWareCoreSettings::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomCustomVersion::GUID);

#if WITH_EDITORONLY_DATA
	if (GetLinkerCustomVersion(FAtomCustomVersion::GUID) < FAtomCustomVersion::LatestVersion)
	{
		// Disable dynamic FileSystem if updated from previous versions.
		bUseAutomaticFileSystemManagement = false;

		// Use deprecated standard voice settings values to setup voices settings.
		VoicesSettings.NumVoices = StandardVoicesSettings_DEPRECATED.NumStandardMemoryVoices + StandardVoicesSettings_DEPRECATED.NumStandardStreamingVoices;
		VoicesSettings.NumChannels = FMath::Max(StandardVoicesSettings_DEPRECATED.StandardStreamingVoiceNumChannels, StandardVoicesSettings_DEPRECATED.StandardMemoryVoiceNumChannels);
		VoicesSettings.SamplingRate = FMath::Max(StandardVoicesSettings_DEPRECATED.StandardStreamingVoiceSamplingRate, StandardVoicesSettings_DEPRECATED.StandardMemoryVoiceSamplingRate);
	
		// Setup the HCA-MX mixer if it was used previously.
		if (HcaMxVoicesSettings_DEPRECATED.NumHcaMxMemoryVoices > 0 || HcaMxVoicesSettings_DEPRECATED.NumHcaMxStreamingVoices > 0)
		{
			bEnableHcaMxDecoding = true;
			HcaMxSettings.MaxVoices = HcaMxVoicesSettings_DEPRECATED.NumHcaMxMemoryVoices + HcaMxVoicesSettings_DEPRECATED.NumHcaMxStreamingVoices;
			HcaMxSettings.MaxSamplingRate = HcaMxVoicesSettings_DEPRECATED.HcaMxVoiceSamplingRate;
			HcaMxSettings.OutputSamplingRate = HcaMxVoicesSettings_DEPRECATED.HcaMxVoiceSamplingRate;
		}
	}
#endif
}

void UCriWareCoreSettings::PostInitProperties()
{
	Super::PostInitProperties();

	if (MaxSoundPlaybackHandles_DEPRECATED > 0)
	{
		MaxSoundSources = MaxSoundPlaybackHandles_DEPRECATED;
		NumReservedSoundSources = 0;

		MaxSoundPlaybackHandles_DEPRECATED = 0;
		TryUpdateDefaultConfigFile();
	}
}

#if WITH_EDITOR
void UCriWareCoreSettings::PreEditChange(FProperty* PropertyAboutToChange)
{
	// Cache master rack in case user tries to set to rack that isn't a top-level rack
	CachedMasterRack = MasterRack;

	// Cache master rack in case user tries to set an invalid rack that cannot be used to spatialize sounds.
	CachedSpatializationRack = SpatializationRack;

	// Cache Atom configuration in case user cancels the operation from the default DSP bus setting message box
	CachedAtomConfig = AtomConfig;
}

void UCriWareCoreSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (GEditor && GEditor->IsPlayingSessionInEditor())
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Parameter changing is not applied in PIE Mode."));
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MasterRack))
	{
		if (UAtomRack* NewAtomRack = Cast<UAtomRack>(MasterRack.TryLoad()))
		{
			if (NewAtomRack->ParentRack)
			{
				FNotificationInfo Info(LOCTEXT("CriWareCore_Settings_InvalidMasterRack", "'Master Rack' cannot be set to submix with parent."));
				Info.bFireAndForget = true;
				Info.ExpireDuration = 2.0f;
				Info.bUseThrobber = true;
				FSlateNotificationManager::Get().AddNotification(Info);

				// revert
				MasterRack = CachedMasterRack;
			}
			else
			{
				// reactivate the active Atom runtime
				ReactivateActiveRuntime();
			}
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, SpatializationRack))
	{
		if (UAtomRackBase* NewAtomRack = Cast<UAtomRackBase>(SpatializationRack.TryLoad()))
		{
			// reactivate the active Atom runtime
			ReactivateActiveRuntime();
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultAudioBuses)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(FDefaultAtomAudioBusSettings, AudioBus))
	{
		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			RuntimeManager->IterateOverAllRuntimes([this](FAtomRuntimeId, FAtomRuntime* InRuntime)
			{
				UAtomAudioBusSubsystem* AudioBusSubsystem = InRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
				check(AudioBusSubsystem);
				AudioBusSubsystem->InitDefaultAudioBuses();
			});
		}
	}

	// setup atom configuration on change
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, AtomConfig))
	{
		auto AtomConfigAsset = Cast<UAtomConfig>(AtomConfig.TryLoad());
		auto MasterRackAsset = Cast<UAtomRack>(MasterRack.TryLoad());

		bool bWasCanceled = false;
		bool bConfigToMaster = ShowApplyAtomConfigToMasterRackDialog(AtomConfigAsset, bWasCanceled);
		if (bWasCanceled)
		{
			// revert
			AtomConfig = CachedAtomConfig;
		}
		else
		{
			// Apply to Atom runtime
			if (GCriWare)
			{
				FAtomRuntime* ActiveAtomRuntime = nullptr;
				if (FAtomRuntimeHandle RuntimeHandle = GCriWare->GetActiveAtomRuntime())
				{
					ActiveAtomRuntime = RuntimeHandle.GetAtomRuntime();
					if (ActiveAtomRuntime)
					{
						ActiveAtomRuntime->Deactivate();
					}
				}

				if (!GCriWare->SetAtomConfiguration(AtomConfigAsset))
				{
					FNotificationInfo Info(LOCTEXT("CriWareCore_Settings_InvalidAtomConfig", "Failed to register AtomConfig to CriWare libary."));
					Info.bFireAndForget = true;
					Info.ExpireDuration = 2.0f;
					Info.bUseThrobber = true;
					FSlateNotificationManager::Get().AddNotification(Info);
				}
				else if (bConfigToMaster)
				{
					// Apply Dsp bus setting to master rack
					if (MasterRackAsset)
					{
						auto DefaultDspBusSetting = AtomConfigAsset ? AtomConfigAsset->GetDefaultDspBusSetting() : nullptr;
						MasterRackAsset->SetDspBusSetting(DefaultDspBusSetting, true);
						MasterRackAsset->MarkPackageDirty();
					}
				}

				if (ActiveAtomRuntime)
				{
					// reactivate the active Atom runtime
					ActiveAtomRuntime->Activate();
				}
			}
		}
	}

	// constants if changed that need to restart Atom runtime to apply
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet
		&& (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseAutomaticFileSystemManagement)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, NumBinders)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MaxBindings)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, NumLoaders)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableLoggingFileSystem)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MaxPitch)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MaxSoundSources)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, NumReservedSoundSources)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableLoggingAtom)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableBinauralSpatialization)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultOutputSubmix)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseAutomaticVoiceManagement)
		|| MemberPropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, VoicesSettings)))
	{
		// reactivate the active Atom runtime
		ReactivateActiveRuntime();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	CriWareCoreSettingsChanged.Broadcast();
}

void UCriWareCoreSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();

		LoadDefaultObjects();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, ModulationParameters))
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Reloading Parameters from CriWare Atom Settings..."));
			RegisterModulationParameters();
		}
	}

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UCriWareCoreSettings::ReactivateActiveRuntime() const
{
	// reactivate the active Atom runtime
	if (GCriWare)
	{
		if (FAtomRuntimeHandle RuntimeHandle = GCriWare->GetActiveAtomRuntime())
		{
			if (FAtomRuntime* AtomRuntime = RuntimeHandle.GetAtomRuntime())
			{
				AtomRuntime->Deactivate();
				AtomRuntime->Activate();
			}
		}
	}
}

bool UCriWareCoreSettings::ShowApplyAtomConfigToMasterRackDialog(const UAtomConfig* InAtomConfig, bool& bOutWasCanceled)
{
	auto MasterRackAsset = Cast<UAtomRack>(MasterRack.TryLoad());
	if (!MasterRackAsset)
	{
		return false;
	}

	if (InAtomConfig)
	{
		// Ask user if master rack need to use the default DSP bus settings from ACF data.
		const UAtomDspBusSetting* DefaultSetting = InAtomConfig->GetDefaultDspBusSetting();
		if (DefaultSetting)
		{
			FText MessageBoxTitle = LOCTEXT("AtomConfigDspSettingTitle", "Setup Atom Configuration File");
			FText MessageBoxContent = FText::Format(LOCTEXT("AtomConfigDspSettingContent",
				"Atom configuration file contains DSP bus setting '{0}' marked as default.\n\n"
				"Do you want to update Master Rack with default DSP bus setting?\n\n"
				"Warning: All buses and their current settings will be reset."), FText::FromString(DefaultSetting->GetSettingName()));

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, MessageBoxTitle);
#else
			auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, &MessageBoxTitle);
#endif
			bool bCancelByUser = Result == EAppReturnType::Type::Cancel;
			bool bSetupToMaster = Result == EAppReturnType::Type::Yes;

			if (bCancelByUser)
			{
				bOutWasCanceled = true;
				return false;
			}

			if (!bSetupToMaster)
			{
				return false;
			}
		}

		// ACF should always have a default settings
	}
	else
	{
		if (!MasterRackAsset->DspBusSetting)
		{
			return false;
		}

		FText MessageBoxTitle = LOCTEXT("AtomConfigDspSettingTitle", "Setup Atom Configuration File");
		FText MessageBoxContent = LOCTEXT("RemoveAtomConfigContent",
			"Atom Configuration file will be removed.\n\n"
			"Do you want to remove Master Rack Dsp bus setting?\n\n"
			"Warning: All buses and their current settings will be reset.");

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, MessageBoxTitle);
#else
		auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, &MessageBoxTitle);
#endif
		bool bCancelByUser = Result == EAppReturnType::Type::Cancel;
		bool bSetupToMaster = Result == EAppReturnType::Type::Yes;

		if (bCancelByUser)
		{
			bOutWasCanceled = true;
			return false;
		}

		if (!bSetupToMaster)
		{
			return false;
		}
	}

	return true;
}
#endif

void UCriWareCoreSettings::LoadDefaultObjects()
{
	UE_LOG(LogCriWareAtom, Display, TEXT("Loading Default Audio Settings Objects..."));

	// TODO: Move all soft object paths to load here (SoundMixes, Submixes, etc.)
	//static const FString EngineSoundsDir = TEXT("/Engine/EngineSounds");

	if (DefaultSoundClass)
	{
		DefaultSoundClass->RemoveFromRoot();
		DefaultSoundClass = nullptr;
	}

	if (UObject* SoundClassObject = DefaultSoundClassName.TryLoad())
	{
		DefaultSoundClass = CastChecked<UAtomSoundClass>(SoundClassObject);
		DefaultSoundClass->AddToRoot();
	}

#if WITH_EDITOR
	if (!DefaultSoundClass)
	{
		DefaultSoundClassName = CachedSoundClass;
		if (UObject* SoundClassObject = DefaultSoundClassName.TryLoad())
		{
			DefaultSoundClass = CastChecked<UAtomSoundClass>(SoundClassObject);
			DefaultSoundClass->AddToRoot();
		}
	}
#endif // WITH_EDITOR

	/*if (!DefaultSoundClass)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Failed to load Default SoundClassObject from path '%s'.  Attempting to fall back to engine default."), *DefaultSoundClassName.GetAssetPathString());
		DefaultSoundClassName.SetPath(EngineSoundsDir / TEXT("Master"));
		if (UObject* SoundClassObject = DefaultSoundClassName.TryLoad())
		{
			DefaultSoundClass = CastChecked<UAtomSoundClass>(SoundClassObject);
			DefaultSoundClass->AddToRoot();
		}
	}

	if (!DefaultSoundClass)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("Failed to load Default SoundClassObject from path '%s'."), *DefaultSoundClassName.GetAssetPathString());
	}*/

	if (DefaultManaSoundClass)
	{
		DefaultManaSoundClass->RemoveFromRoot();
		DefaultManaSoundClass = nullptr;
	}

	if (UObject* ManaSoundClassObject = DefaultManaSoundClassName.TryLoad())
	{
		DefaultManaSoundClass = CastChecked<UAtomSoundClass>(ManaSoundClassObject);
		DefaultManaSoundClass->AddToRoot();
	}
	else
	{
		UE_LOG(LogAudio, Display, TEXT("No default ManaSoundClassObject specified (or failed to load)."));
	}

	if (DefaultSoundConcurrency)
	{
		DefaultSoundConcurrency->RemoveFromRoot();
		DefaultSoundConcurrency = nullptr;
	}

	if (UObject* SoundConcurrencyObject = DefaultSoundConcurrencyName.TryLoad())
	{
		DefaultSoundConcurrency = CastChecked<UAtomConcurrency>(SoundConcurrencyObject);
		DefaultSoundConcurrency->AddToRoot();
	}
	else
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("No default SoundConcurrencyObject specified (or failed to load)."));
	}
}

void UCriWareCoreSettings::RegisterModulationParameters() const
{
	Atom::UnregisterAllModulationParameters();

	for (const FSoftObjectPath& Path : ModulationParameters)
	{
		if (UAtomModulationParameter* Param = Cast<UAtomModulationParameter>(Path.TryLoad()))
		{
			Atom::FModulationParameter NewParam = Param->CreateParameter();
			Atom::RegisterModulationParameter(NewParam.ParameterName, MoveTemp(NewParam));
			UE_LOG(LogCriWareAtom, Display, TEXT("Initialized Atom Modulation Parameter '%s'"), *NewParam.ParameterName.ToString());
		}
		else
		{
			UE_LOG(LogCriWareAtom, Error, TEXT("Failed to load parameter at '%s': Missing asset or invalid type."), *Path.GetAssetName());
		}
	}
}

UAtomSoundClass* UCriWareCoreSettings::GetDefaultSoundClass() const
{
	return DefaultSoundClass;
}

UAtomSoundClass* UCriWareCoreSettings::GetDefaultManaSoundClass() const
{
	return DefaultManaSoundClass;
}

UAtomConcurrency* UCriWareCoreSettings::GetDefaultSoundConcurrency() const
{
	return DefaultSoundConcurrency;
}

#undef LOCTEXT_NAMESPACE
