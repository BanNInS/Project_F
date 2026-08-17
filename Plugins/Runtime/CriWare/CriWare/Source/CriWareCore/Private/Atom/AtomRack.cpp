/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomRack.cpp
 *
 ****************************************************************************/

#include "Atom/AtomRack.h"

#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Sound/SoundSubmix.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Async/Async.h"
#include "Runtime/Launch/Resources/Version.h"
#endif // WITH_EDITOR

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/AtomCustomVersion.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomRackSubmixListener.h"

#define LOCTEXT_NAMESPACE "AtomRack"

static int32 ClearBrokenRackAssetsCVar = 0;
FAutoConsoleVariableRef CVarFixUpBrokenRackAssets(
	TEXT("atom.rack.clearbrokenrackassets"),
	ClearBrokenRackAssetsCVar,
	TEXT("If fixed, will verify that we don't have a rack list a child rack that doesn't have it as it's parent, or vice versa.\n")
	TEXT("0: Disable, >0: Enable"),
	ECVF_Default);

const FString UAtomRackBase::MainBusName = TEXT("MasterOut");

// Custom serialization version for AtomRack asset
struct FAtomRackCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
	};

	// The GUID for this custom version number
	static const FGuid GUID;
};

const FGuid FAtomRackCustomVersion::GUID(0xA3C9605D, 0xE05D4565, 0x91A47426, 0x4E03DF34);
// Register the custom version with core
FCustomVersionRegistration GRegisterAtomRackCustomVersion(FAtomRackCustomVersion::GUID, FAtomRackCustomVersion::LatestVersion, TEXT("AtomRackVer"));

UAtomRackBase::UAtomRackBase(const FObjectInitializer& ObjectInitializer)
#if WITH_EDITORONLY_DATA
	: AtomRackGraph(nullptr)
#endif // WITH_EDITORONLY_DATA
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomRackBase::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomCustomVersion::GUID);
}

UAtomRackWithParentBase::UAtomRackWithParentBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ParentRack(nullptr)
	, MasterBus(nullptr)
{}

UAtomRack::UAtomRack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bMuteWhenBackgrounded(0)
	, NumChannels(8)
	, SpeakerChannelMap(EAtomSpeakerChannelMap::SevenPointOne)
	, SamplingRate(48000)
	, ServerFrequency(60.0f)
	, AtomConfig(nullptr)
	, DspBusSetting(nullptr)
	//, AmbisonicsPluginSettings(nullptr)
#if WITH_EDITOR
	, CachedAtomConfig(nullptr)
#endif
{
}

void UAtomRackWithParentBase::PostLoad()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
#if WITH_EDITORONLY_DATA
		if (GetLinkerCustomVersion(FAtomCustomVersion::GUID) < FAtomCustomVersion::AddMixerNameSupport)
		{
			MasterBus->BusName = MainBusName;

			if (auto Package = GetOutermost())
			{
				Package->SetDirtyFlag(true);
				Package->PackageMarkedDirtyEvent.Broadcast(Package, true);
			}
		}
#endif

		MasterBus->BusIndex = 0;
	}

	Super::PostLoad();
}

void UAtomRack::PostLoad()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
#if WITH_EDITORONLY_DATA
		if (GetLinkerCustomVersion(FAtomCustomVersion::GUID) < FAtomCustomVersion::AddMixerNameSupport)
		{
			for (auto& Bus : Buses)
			{
				if (Bus)
				{
					Bus->GetName(Bus->BusName);
				}
			}

			if (DspBusSetting)
			{
				DspBusSettingName = DspBusSetting->GetSettingName();
			}

			if (!Snapshot_DEPRECATED.IsNone())
			{
				SnapshotName = Snapshot_DEPRECATED.ToString();
			}
		}
#endif
		// Setup current index to bus for convenience
		for (int Index = 0; Index < Buses.Num(); ++Index)
		{
			if (auto& Bus = Buses[Index])
			{
				Bus->BusIndex = Index;
			}
		}

		// Register rack and apply mixer settings
		Super::PostLoad();

		// Update bus list and apply local mixer bus settings
		UpdateBuses();
	}
	else
	{
		Super::PostLoad();
	}
}

void UAtomRackWithParentBase::PostInitProperties()
{
	Super::PostInitProperties();

	if (!MasterBus && !HasAnyFlags(RF_ClassDefaultObject))
	{
		MasterBus = NewObject<UAtomBus>(this, UAtomBus::StaticClass(), FName(MainBusName), RF_Public);
		MasterBus->Init(MainBusName, 0);
	}
}

void UAtomRack::PostInitProperties()
{
	bool bNeedResetBuses = !MasterBus;

	Super::PostInitProperties();

	if (bNeedResetBuses && !HasAnyFlags(RF_ClassDefaultObject))
	{
		ResetBuses(false); // don't apply to Atom since it is not yet registered with runtime.
	}
}

void UAtomSoundfieldRack::PostInitProperties()
{
	bool bNeedResetBuses = !MasterBus;

	Super::PostInitProperties();

	if (bNeedResetBuses && !HasAnyFlags(RF_ClassDefaultObject) && MasterBus)
	{
		// setup default values
		MasterBus->Reset(false); // don't apply to Atom since it is not yet registered with runtime.
	}
}

void UAtomRack::SetAtomConfig(const UAtomConfig* InAtomConfig)
{
	if (AtomConfig)
	{
		if (!DspBusSetting || Cast<UAtomConfig>(DspBusSetting->GetOuter()) != AtomConfig)
		{
			SetDspBusSetting(AtomConfig->GetDefaultDspBusSetting(), true);
		}
	}
	else
	{
		SetDspBusSetting(nullptr, true);
	}
}

void UAtomRack::SetDspBusSettingName(const FString& InDspBusSettingName)
{
	if (AtomConfig)
	{
		if (auto Setting = AtomConfig->GetDspBusSetting(InDspBusSettingName))
		{
			SetDspBusSetting(Setting);
			return;
		}
	}

	UE_LOG(LogCriWareAtom, Warning, TEXT("Cannot set DspBusSetting to rack '%s'. '%s' to is not available."), *GetFullName(), *InDspBusSettingName);
}

const FString& UAtomRack::GetDspBusSettingName() const
{
	if (DspBusSetting)
	{
		return DspBusSetting->GetSettingName();
	}

	return DspBusSettingName;
}

void UAtomRack::SetDspBusSetting(const UAtomDspBusSetting* InDspBusSetting, bool bResetBuses /* = false*/)
{
	DspBusSetting = InDspBusSetting;

	if (DspBusSetting)
	{
		AtomConfig = Cast<UAtomConfig>(DspBusSetting->GetOuter());
		DspBusSettingName = DspBusSetting->GetSettingName();
	}
	else
	{
		DspBusSettingName.Empty();
	}

	if (bResetBuses)
	{
		ResetBuses();
		ApplyDspBusSetting();
	}
	else
	{
		UpdateBuses();
		if (ApplyDspBusSetting())
		{
			SyncBusesToRuntime();
		}
	}	
}

void UAtomRack::SetSnapshot(const FString& InSnapshotName, float FadeSeconds)
{
	SnapshotName = InSnapshotName;

	ApplySnapshot(FadeSeconds);
}

const FString& UAtomRack::GetSnapshot() const
{
#if WITH_EDITOR
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			// check if snapshot was be changed outside
			if (const FString AppliedSnapshotName = AtomRuntime->GetRackSnapshotName(this); SnapshotName != AppliedSnapshotName)
			{
				UE_LOG(LogCriWareAtom, Warning, TEXT("Snapshot '%s' modified outside AtomRack '%s'."), *SnapshotName, *GetFullName());
			}
		}
	}
#endif
	
	return SnapshotName;
}

bool UAtomRack::ApplySnapshot(float FadeSeconds)
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			return AtomRuntime->SetRackSnapshot(this, FadeSeconds);
		}
	}

	return false;
}

bool UAtomRack::ApplyDspBusSetting()
{
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			return AtomRuntime->SetRackDspBusSetting(this);
		}
	}

	return false;
}

void UAtomRack::ResetBuses(bool bApplyToAtom /* = true */)
{
	// clean Mixer Submix
	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			// Reflect reset of the extra bus except the main bus that is keep
			for (auto& Bus : Buses)
			{
				if (Bus != MasterBus)
				{
					AtomRuntime->UnloadSubmixInstance(Bus);
				}
			}
		}
	}

	// Keep master bus since it is always present, all other buses are wiped out
	Buses.Empty();
	Buses.Add(MasterBus);

	if (DspBusSetting)
	{
		for (int BusIndex = 1; BusIndex < DspBusSetting->GetNumBuses(); BusIndex++)
		{
			UAtomBus* NewBus = nullptr;
			const FString& BusName = DspBusSetting->GetBusName(BusIndex);
			if (!BusName.IsEmpty())
			{
				// Create new bus
				NewBus = NewObject<UAtomBus>(this, UAtomBus::StaticClass(), FName(BusName), RF_Public);
				NewBus->Init(BusName, BusIndex);
				Buses.Add(NewBus);
			}
		}

		// Setup buses to default settings (get and show value from DspBusSettings in editor) but do not re-apply to Atom since setting is already loaded. 
		for (auto& Bus : Buses)
		{
			Bus->Reset(false);
		}
	}
	else
	{
		// In case of there is no DspBusSetting, we ensure values for master bus are reflected between UE4 and Atom.
		MasterBus->Reset(bApplyToAtom);
	}
}

void UAtomRack::UpdateBuses()
{
	if (DspBusSetting)
	{
		TArray<UAtomBus*> UpdatedBuses;

		for (int Index = 0; Index < DspBusSetting->GetNumBuses(); Index++)
		{
			const FString& BusName = DspBusSetting->GetBusName(Index);
			if (!BusName.IsEmpty())
			{
				if (auto BusPtr = Buses.FindByPredicate([&](const UAtomBus* Bus) { return Bus->GetBusName() == BusName; }); BusPtr != nullptr)
				{
					(*BusPtr)->BusIndex = Index;

					UpdatedBuses.Add(*BusPtr);
					//(*BusPtr)->UpdateEffectsFromDspSetting();

					// setup current values to runtime
					(*BusPtr)->ApplyAll();
				}
				else
				{
					UAtomBus* NewBus = NewObject<UAtomBus>(this, UAtomBus::StaticClass(), FName(BusName), RF_Public);
					NewBus->Init(BusName, Index);

					UpdatedBuses.Add(NewBus);
				}
			}
		}

		Buses = UpdatedBuses;
	}
}

void UAtomRack::SyncBusesFromRuntime()
{
	/*for (auto Bus : Buses)
	{
		Bus->UpdateAll();
	}*/
}

void UAtomRack::SyncBusesToRuntime() const
{
	for (auto& Bus : Buses)
	{
		Bus->ApplyAll();
	}
}

void UAtomRack::ProcessAudio(FAtomRuntime* AtomRuntime)
{
	// Register/unregister rack with Unreal submix listener if not yet
	if (AtomRuntime->IsUsingUnrealSoundRenderer() && !SubmixListener.IsValid())
	{
		StartSendToUnrealSubmix(AtomRuntime);
	}
	else if (!AtomRuntime->IsUsingUnrealSoundRenderer() && SubmixListener.IsValid())
	{
		StopSendToUnrealSubmix();
	}
}

void UAtomRack::StartSendToUnrealSubmix(FAtomRuntime* AtomRuntime)
{
	// setup listener
	auto Settings = GetDefault<UCriWareCoreSettings>();
	check(Settings);
	USoundSubmix* OutputSubmix = Cast<USoundSubmix>(Settings->DefaultOutputSubmix.TryLoad());
	int32 RackID = AtomRuntime->GetAsrRackId(this);
	SubmixListener = MakeShared<FAtomRackSubmixListener>(RackID, NumChannels, SamplingRate, OutputSubmix);
	SubmixListener->RegisterSubmixListener();
}

void UAtomRack::StopSendToUnrealSubmix()
{
	SubmixListener.Reset();
}

FAtomLevelMeterSettings UAtomRackBase::GetLevelMeterSettings(float AnalysisPeriod, int32 PeakHoldTime)
{
	FAtomLevelMeterSettings OutSettings;

	OutSettings.AnalysisPeriod = AnalysisPeriod;
	OutSettings.PeakHoldTime = PeakHoldTime;

	return OutSettings;
}

void UAtomRackBase::StartLevelMeterMeasuring(const UObject* WorldContextObject, float AnalysisPeriod, int32 PeakHoldTime)
{
	// Find runtime for this specific level meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StartLevelMeterMeasuring(AtomRuntime, AnalysisPeriod, PeakHoldTime);
	}
}

void UAtomRackBase::StartLevelMeterMeasuring(FAtomRuntime* InAtomRuntime, float AnalysisPeriod, int32 PeakHoldTime)
{
	if (InAtomRuntime)
	{
		FAtomLevelMeterSettings Settings = UAtomRackBase::GetLevelMeterSettings(AnalysisPeriod, PeakHoldTime);
		InAtomRuntime->StartLevelMeterMeasuring(this, Settings);
	}
}

void UAtomRackBase::StopLevelMeterMeasuring(const UObject* WorldContextObject)
{
	// Find runtime for this specific level meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StopLevelMeterMeasuring(AtomRuntime);
	}
}

void UAtomRackBase::StopLevelMeterMeasuring(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StopLevelMeterMeasuring(this);
	}
}

void UAtomRackBase::AddLevelMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP)
{
	// Find runtime for this specific level meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddLevelMeterDelegate(this, OnAtomRackLevelMeterMeasureBP);
	}
}

void UAtomRackBase::RemoveLevelMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP)
{
	// Find runtime for this specific level meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveLevelMeterDelegate(this, OnAtomRackLevelMeterMeasureBP);
	}
}

void UAtomRackBase::StartLoudnessMeterMeasuring(const UObject* WorldContextObject, int32 ShortTermTime, int32 IntegratedTime)
{
	// Find runtime for this specific loudness meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StartLoudnessMeterMeasuring(AtomRuntime, ShortTermTime, IntegratedTime);
	}
}

void UAtomRackBase::StartLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime, int32 ShortTermTime, int32 IntegratedTime)
{
	if (InAtomRuntime)
	{
		FAtomLoudnessMeterSettings Settings = UAtomRackBase::GetLoudnessMeterSettings(ShortTermTime, IntegratedTime);
		InAtomRuntime->StartLoudnessMeterMeasuring(this, Settings);
	}
}

void UAtomRackBase::StopLoudnessMeterMeasuring(const UObject* WorldContextObject)
{
	// Find runtime for this specific loudness meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StopLoudnessMeterMeasuring(AtomRuntime);
	}
}

void UAtomRackBase::StopLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StopLoudnessMeterMeasuring(this);
	}
}

void UAtomRackBase::ResetLoudnessMeterMeasuring(const UObject* WorldContextObject)
{
	// Find runtime for this specific loudness meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		ResetLoudnessMeterMeasuring(AtomRuntime);
	}
}

void UAtomRackBase::ResetLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->ResetLoudnessMeterMeasuring(this);
	}
}

void UAtomRackBase::AddLoudnessMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP)
{
	// Find runtime for this specific loudness meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddLoudnessMeterDelegate(this, OnAtomRackLoudnessMeterMeasureBP);
	}
}

void UAtomRackBase::RemoveLoudnessMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP)
{
	// Find runtime for this specific loudness meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveLoudnessMeterDelegate(this, OnAtomRackLoudnessMeterMeasureBP);
	}
}

FAtomLoudnessMeterSettings UAtomRackBase::GetLoudnessMeterSettings(int32 ShortTermTime, int32 IntegratedTime)
{
	FAtomLoudnessMeterSettings OutSettings;

	OutSettings.ShortTermTime = ShortTermTime;
	OutSettings.IntegratedTime = IntegratedTime;

	return OutSettings;
}

void UAtomRackBase::StartTruePeakMeterMeasuring(const UObject* WorldContextObject, float AnalysisPeriod, int32 PeakHoldTime, bool bSampleClipping)
{
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StartTruePeakMeterMeasuring(AtomRuntime, AnalysisPeriod, PeakHoldTime, bSampleClipping);
	}
}

void UAtomRackBase::StartTruePeakMeterMeasuring(FAtomRuntime* InAtomRuntime, float AnalysisPeriod, int32 PeakHoldTime, bool bSampleClipping)
{
	if (InAtomRuntime)
	{
		FAtomTruePeakMeterSettings Settings = UAtomRackBase::GetTruePeakMeterSettings(AnalysisPeriod, PeakHoldTime, bSampleClipping);
		InAtomRuntime->StartTruePeakMeterMeasuring(this, Settings);
	}
}

void UAtomRackBase::StopTruePeakMeterMeasuring(const UObject* WorldContextObject)
{
	// Find runtime for this specific true peak meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		StopTruePeakMeterMeasuring(AtomRuntime);
	}
}

void UAtomRackBase::StopTruePeakMeterMeasuring(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->StopTruePeakMeterMeasuring(this);
	}
}

void UAtomRackBase::AddTruePeakMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP)
{
	// Find runtime for this specific true peak meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddTruePeakMeterDelegate(this, OnAtomRackTruePeakMeterMeasureBP);
	}
}

void UAtomRackBase::RemoveTruePeakMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP)
{
	// Find runtime for this specific true peak meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveTruePeakMeterDelegate(this, OnAtomRackTruePeakMeterMeasureBP);
	}
}

FAtomTruePeakMeterSettings UAtomRackBase::GetTruePeakMeterSettings(float AnalysisPeriod, int32 PeakHoldTime, bool bSampleClipping)
{
	FAtomTruePeakMeterSettings OutSettings;

	OutSettings.AnalysisPeriod = AnalysisPeriod;
	OutSettings.PeakHoldTime = PeakHoldTime;
	OutSettings.bSampleClipping = bSampleClipping;

	return OutSettings;
}

void UAtomRackBase::AddRenderedSamplesDelegate(const UObject* WorldContextObject, const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSamplesBP)
{
	// Find runtime for this specific level meter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddRenderedSamplesDelegate(this, OnAtomRackRenderedSamplesBP);
	}
}

void UAtomRackBase::RemoveRenderedSamplesDelegate(const UObject* WorldContextObject, const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSamplesBP)
{
	// Find runtime for this specific sample counter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveRenderedSamplesDelegate(this, OnAtomRackRenderedSamplesBP);
	}
}

void UAtomRackBase::AddPerformanceMonitorDelegate(const UObject* WorldContextObject, const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP)
{
	// Find runtime for this specific sample counter thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddPerformanceMonitorDelegate(this, OnAtomRackPerformanceMonitorResultBP);
	}
}

void UAtomRackBase::RemovePerformanceMonitorDelegate(const UObject* WorldContextObject, const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP)
{
	// Find runtime for this specific Performance Monitor thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemovePerformanceMonitorDelegate(this, OnAtomRackPerformanceMonitorResultBP);
	}
}

void UAtomRackBase::ResetPerformanceMonitor(const UObject* WorldContextObject)
{
	// Find runtime for this specific Performance Monitor thing.
	if (auto AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		ResetPerformanceMonitor(AtomRuntime);
	}
}

void UAtomRackBase::ResetPerformanceMonitor(FAtomRuntime* InAtomRuntime)
{
	if (InAtomRuntime)
	{
		InAtomRuntime->ResetPerformanceMonitor(this);
	}
}

UAtomEndpointRack::UAtomEndpointRack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SoundRendererType(EAtomSoundRendererType::Default)
{

}

const UAtomEndpointSettingsBase* UAtomEndpointRack::GetEndpointSettings(const TSubclassOf<UAtomEndpointSettingsBase>& EndpointSettingsClass) const
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
void UAtomEndpointRack::Serialize(FArchive& Ar)
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

#if WITH_EDITOR
void UAtomRack::PreEditChange(FProperty* PropertyAboutToChange)
{
	CachedAtomConfig = AtomConfig;
	CachedDspBusSettingName = DspBusSettingName;
}

void UAtomRack::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property)
	{
		FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRack, AtomConfig))
		{
			if (AtomConfig != CachedAtomConfig)
			{
				SetAtomConfig(AtomConfig);
			}

			if (ApplyDspBusSetting())
			{
				ResetBuses();
			}
		}

		/*if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRack, DspBusSettingName))
		{
			if (DspBusSettingName != CachedDspBusSettingName)
			{
				SetDspBusSettingName(DspBusSettingName);
			}

			if (ApplyDspBusSetting())
			{
				ResetBuses();
			}
		}*/

		if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRack, DspBusSetting))
		{
			SetDspBusSetting(DspBusSetting);
		}

		if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRack, SnapshotName))
		{
			ApplySnapshot(0.0f);
		}

		//bool bUpdateRackGain = false;
	
		//if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRack, OutputVolume))
		//{
		//	bUpdateRackGain = true;
		//}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FString UAtomRackBase::GetDesc()
{
	return FString(TEXT("Atom Rack Submix"));
}

void UAtomRackBase::BeginDestroy()
{
	Super::BeginDestroy();

	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->UnregisterAtomRack(this);
		}
	}
}

void UAtomRackBase::PostLoad()
{
	Super::PostLoad();

	if (ClearBrokenRackAssetsCVar)
	{
		for (int32 ChildIndex = ChildRacks.Num() - 1; ChildIndex >= 0; ChildIndex--)
		{
			UAtomRackBase* ChildRack = ChildRacks[ChildIndex];

			if (!ChildRack)
			{
				continue;
			}

			if (UAtomRackWithParentBase* CastedChildRack = Cast<UAtomRackWithParentBase>(ChildRack))
			{
				if (!ensure(CastedChildRack->ParentRack == this))
				{
					UE_LOG(LogCriWareCore, Warning, TEXT("Atom rack had a child rack that didn't explicitly mark this rack as a parent!"));
					ChildRacks.RemoveAtSwap(ChildIndex);
				}
			}
			else
			{
				ensureMsgf(false, TEXT("Atom rack had a child rack that doesn't have an output!"));
				ChildRacks.RemoveAtSwap(ChildIndex);
			}
		}
	}

	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
		{
			AtomRuntime->RegisterAtomRack(this);
		}
	}
}

#if WITH_EDITOR

void UAtomRackBase::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	if (DuplicateMode == EDuplicateMode::Normal)
	{
		ChildRacks.Reset();
	}
}

void UAtomRackBase::PreEditChange(FProperty* PropertyAboutToChange)
{
	if (PropertyAboutToChange && PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomRackBase, ChildRacks))
	{
		// Take a copy of the current state of child classes
		BackupChildRacks = ChildRacks;
	}
}

void UAtomRackBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!GCriWare || !GCriWare->IsInitialized())
	{
		return;
	}

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomRackBase, ChildRacks))
		{
			// Find child that was changed/added
			for (int32 ChildIndex = 0; ChildIndex < ChildRacks.Num(); ChildIndex++)
			{
				if (ChildRacks[ChildIndex] != nullptr && !BackupChildRacks.Contains(ChildRacks[ChildIndex]))
				{
					if (ChildRacks[ChildIndex]->RecurseCheckChild(this))
					{
						// Contains cycle so revert to old layout - launch notification to inform user
						FNotificationInfo Info(NSLOCTEXT("Engine", "UnableToChangeAtomRackChildDueToInfiniteLoopNotification", "Could not change AtomRack child as it would create a loop"));
						Info.ExpireDuration = 5.0f;
						Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
						FSlateNotificationManager::Get().AddNotification(Info);

						// Revert to the child racks
						ChildRacks = BackupChildRacks;
					}
					else if (UAtomRackWithParentBase* RackWithParent = CastChecked<UAtomRackWithParentBase>(ChildRacks[ChildIndex]))
					{
						// Update parentage
						RackWithParent->SetParentRack(this);
					}
					break;
				}
			}

			// Update old child's parent if it has been removed
			for (int32 ChildIndex = 0; ChildIndex < BackupChildRacks.Num(); ChildIndex++)
			{
				if (BackupChildRacks[ChildIndex] != nullptr && !ChildRacks.Contains(BackupChildRacks[ChildIndex]))
				{
					BackupChildRacks[ChildIndex]->Modify();
					if (UAtomRackWithParentBase* RackWithParent = Cast<UAtomRackWithParentBase>(BackupChildRacks[ChildIndex]))
					{
						RackWithParent->ParentRack = nullptr;
					}
				}
			}

			if (GCriWare)
			{
				if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
				{
					AtomRuntime->RegisterAtomRack(this);
				}
			}

			// Force the properties to be initialized for this SoundSubmix on all active audio devices
			//if (FAudioDeviceManager* AudioDeviceManager = GEngine->GetAudioDeviceManager())
			//{
			//	AudioDeviceManager->RegisterSoundSubmix(this);
			//}
		}
	}

	BackupChildRacks.Reset();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

TArray<TObjectPtr<UAtomRackBase>> UAtomRackBase::BackupChildRacks;

bool UAtomRackBase::RecurseCheckChild(const UAtomRackBase* ChildAtomRack) const
{
	for (int32 Index = 0; Index < ChildRacks.Num(); Index++)
	{
		if (ChildRacks[Index])
		{
			if (ChildRacks[Index] == ChildAtomRack)
			{
				return true;
			}

			if (ChildRacks[Index]->RecurseCheckChild(ChildAtomRack))
			{
				return true;
			}
		}
	}

	return false;
}

void UAtomRackWithParentBase::SetParentRack(UAtomRackBase* InParentRack)
{
	if (ParentRack != InParentRack)
	{
		if (ParentRack)
		{
			ParentRack->Modify();
			ParentRack->ChildRacks.Remove(this);
		}

		Modify();
		ParentRack = InParentRack;
		if (ParentRack)
		{
			ParentRack->ChildRacks.AddUnique(this);
		}
	}
}

void UAtomRackWithParentBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!GCriWare || !GCriWare->IsInitialized())
	{
		return;
	}

	if (PropertyChangedEvent.Property != nullptr)
	{
		FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		if (ChangedPropName == GET_MEMBER_NAME_CHECKED(UAtomRackWithParentBase, ParentRack))
		{
			// Add this sound class to the parent class if it's not already added
			if (ParentRack)
			{
				bool bIsChildRack = false;
				for (int32 i = 0; i < ParentRack->ChildRacks.Num(); ++i)
				{
					UAtomRackBase* ChildRack = ParentRack->ChildRacks[i];
					if (ChildRack && ChildRack == this)
					{
						bIsChildRack = true;
						break;
					}
				}

				if (!bIsChildRack)
				{
					ParentRack->Modify();
					ParentRack->ChildRacks.AddUnique(this);
				}
			}

			Modify();

			if (GCriWare)
			{
				if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
				{
					AtomRuntime->RegisterAtomRack(this);
				}
			}

			// Force the properties to be initialized for this SoundSubmix on all active audio devices
			//if (FAudioDeviceManager* AudioDeviceManager = GEngine->GetAudioDeviceManager())
			//{
			//	AudioDeviceManager->RegisterSoundSubmix(this);
			//}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UAtomRackWithParentBase::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	if (DuplicateMode == EDuplicateMode::Normal)
	{
		SetParentRack(nullptr);
	}

	Super::PostDuplicate(DuplicateMode);
}

void UAtomRackBase::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UAtomRackBase* This = CastChecked<UAtomRackBase>(InThis);

	Collector.AddReferencedObject(This->AtomRackGraph, This);

	for (auto& Backup : This->BackupChildRacks)
	{
		Collector.AddReferencedObject(Backup);
	}

	Super::AddReferencedObjects(InThis, Collector);
}
#endif // WITH_EDITOR

/*ISoundfieldFactory* UAtomSoundfieldRack::GetSoundfieldFactoryForRack() const
{
	// If this isn't called in the game thread, a ParentSubmix could get destroyed while we are recursing through the submix graph.
	// ensure(IsInGameThread());

	FName SoundfieldFormat = GetSubmixFormat();
	check(SoundfieldFormat != ISoundfieldFactory::GetFormatNameForInheritedEncoding());

	return ISoundfieldFactory::Get(SoundfieldFormat);
}*/

/*const UAtomSoundfieldEncodingSettingsBase* UAtomSoundfieldRack::GetSoundfieldEncodingSettings() const
{
	return GetEncodingSettings();
}*/

/*TArray<UAtomSoundfieldEffectBase*> UAtomSoundfieldRack::GetSoundfieldProcessors() const
{
	return SoundfieldEffectChain;
}*/

EAtomSoundfieldRendererType UAtomSoundfieldRack::GetRackType() const
{
	UAtomSoundfieldRack* ParentSoundfieldRack = Cast<UAtomSoundfieldRack>(ParentRack);

	if (!ParentSoundfieldRack || SoundfieldRendererType != EAtomSoundfieldRendererType::Default)// || SoundfieldEncodingFormat != ISoundfieldFactory::GetFormatNameForInheritedEncoding())
	{
		/*if (SoundfieldEncodingFormat == ISoundfieldFactory::GetFormatNameForInheritedEncoding())
		{
			return ISoundfieldFactory::GetFormatNameForNoEncoding();
		}
		else
		{
			return SoundfieldEncodingFormat;
		}*/

		return SoundfieldRendererType;

	}
	else if (ParentSoundfieldRack)
	{
		// If this submix matches the format of whatever submix it's plugged into, 
		// Recurse into the submix graph to find it.
		return ParentSoundfieldRack->GetRackType();
	}
	else
	{
		return EAtomSoundfieldRendererType::Default;
		//return ISoundfieldFactory::GetFormatNameForNoEncoding();
	}
}

/*const UAtomSoundfieldEncodingSettingsBase* UAtomSoundfieldRack::GetEncodingSettings() const
{
	FName SubmixFormatName = GetSubmixFormat();

	USoundfieldSubmix* ParentSoundfieldSubmix = Cast<USoundfieldSubmix>(ParentSubmix);

	if (EncodingSettings)
	{
		return EncodingSettings;
	}
	else if (ParentSoundfieldSubmix && SoundfieldEncodingFormat == ISoundfieldFactory::GetFormatNameForInheritedEncoding())
	{
		// If this submix matches the format of whatever it's plugged into,
		// Recurse into the submix graph to match it's settings.
		return ParentSoundfieldSubmix->GetEncodingSettings();
	}
	else if (ISoundfieldFactory* Factory = ISoundfieldFactory::Get(SubmixFormatName))
	{
		// If we don't have any encoding settings, use the default.
		return Factory->GetDefaultEncodingSettings();
	}
	else
	{
		// If we don't have anything, exit.
		return nullptr;
	}
}*/

void UAtomSoundfieldRack::SanitizeLinks()
{
	bool bShouldRefreshGraph = false;

	// Iterate through children and check encoding formats.
	for (int32 Index = ChildRacks.Num() - 1; Index >= 0; Index--)
	{
		if (!AtomRackUtils::AreRackSoundTypeCompatible(ChildRacks[Index], this))
		{
			CastChecked<UAtomRackWithParentBase>(ChildRacks[Index])->ParentRack = nullptr;
			ChildRacks[Index]->Modify();
			ChildRacks.RemoveAtSwap(Index);
			bShouldRefreshGraph = true;
		}
	}

	// If this submix is now incompatible with the parent submix, disconnect it.
	if (ParentRack && !AtomRackUtils::AreRackSoundTypeCompatible(this, ParentRack))
	{
		ParentRack->ChildRacks.RemoveSwap(this);
		ParentRack->Modify();
		ParentRack = nullptr;
		bShouldRefreshGraph = true;
	}

	if (bShouldRefreshGraph)
	{
#if WITH_EDITOR
		AtomRackUtils::RefreshEditorForRack(this);
#endif
	}
}

#if WITH_EDITOR

void UAtomSoundfieldRack::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Whether to clean up now invalid links between submix and refresh the submix graph editor.
	bool bShouldSanitizeLinks = false;

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomSoundfieldRack, SoundfieldRendererType))
		{
			bShouldSanitizeLinks = true;
			//AudioDeviceManager->InitSoundSubmixes(); //all devices!
				
			MasterBus->Reset(false);

			const TWeakObjectPtr<UAtomRackBase> WeakThis = this;
			FAtomThread::RunCommandOnAtomThread([WeakThis]()
			{
				if (const UAtomRackBase* ThisRack = WeakThis.Get())
				{
					FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
					if (AtomRuntimeManager)
					{
						if (FAtomRuntimeHandle Runtime = AtomRuntimeManager->GetActiveAtomRuntime())
						{
							Runtime->UnregisterAtomRack(ThisRack);
							Runtime->RegisterAtomRack(ThisRack);
						}
					}
				}
			});	
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bShouldSanitizeLinks)
	{
		SanitizeLinks();
	}
}

#endif // WITH_EDITOR

void UAtomSoundfieldEndpointRack::SanitizeLinks()
{
	bool bShouldRefreshEditor = false;

	// Iterate through children and check encoding formats.
	for (int32 Index = ChildRacks.Num() - 1; Index >= 0; Index--)
	{
		if (!AtomRackUtils::AreRackSoundTypeCompatible(ChildRacks[Index], this))
		{
			CastChecked<UAtomRackWithParentBase>(ChildRacks[Index])->ParentRack = nullptr;
			ChildRacks[Index]->Modify();
			ChildRacks.RemoveAtSwap(Index);

			bShouldRefreshEditor = true;
		}
	}

	if (bShouldRefreshEditor)
	{
#if WITH_EDITOR
		AtomRackUtils::RefreshEditorForRack(this);
#endif
	}
}

#if WITH_EDITOR

void UAtomSoundfieldEndpointRack::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		static const FName NAME_AtomSoundfieldType(TEXT("SoundfieldRendererType"));

		if (PropertyChangedEvent.Property->GetFName() == NAME_AtomSoundfieldType)
		{
			// Add this sound class to the parent class if it's not already added
			SanitizeLinks();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR


CRIWARECORE_API bool AtomRackUtils::AreRackSoundTypeCompatible(const UAtomRackBase* ChildRack, const UAtomRackBase* ParentRack)
{
	const UAtomSoundfieldRack* ChildSoundfieldRack = Cast<const UAtomSoundfieldRack>(ChildRack);

	// If both the child and parent are soundfield racks, ensure that their formats are compatible.
	{
		const UAtomSoundfieldRack* ParentSoundfieldRack = Cast<const UAtomSoundfieldRack>(ParentRack);

		if (ChildSoundfieldRack && ParentSoundfieldRack)
		{
			/*ISoundfieldFactory* ChildSoundfieldFactory = ChildSoundfieldRack->GetSoundfieldFactoryForRack();
			ISoundfieldFactory* ParentSoundfieldFactory = ParentSoundfieldRack->GetSoundfieldFactoryForRack();

			if (ChildSoundfieldFactory && ParentSoundfieldFactory)
			{
				return ChildSoundfieldFactory->CanTranscodeToSoundfieldFormat(ParentSoundfieldFactory->GetSoundfieldFormatName(), *(ParentSoundfieldRack->GetSoundfieldEncodingSettings()->GetProxy()))
					|| ParentSoundfieldFactory->CanTranscodeFromSoundfieldFormat(ChildSoundfieldFactory->GetSoundfieldFormatName(), *(ChildSoundfieldRack->GetSoundfieldEncodingSettings()->GetProxy()));
			}
			else
			{
				return true;
			}*/

			return true;
		}
	}

	// If the child is a soundfield rack and the parent is a soundfield endpoint rack, ensure that they have compatible formats.
	{
		const UAtomSoundfieldEndpointRack* ParentSoundfieldEndpointRack = Cast<const UAtomSoundfieldEndpointRack>(ParentRack);

		if (ChildSoundfieldRack && ParentSoundfieldEndpointRack)
		{
			/*ISoundfieldFactory* ChildSoundfieldFactory = ChildSoundfieldRack->GetSoundfieldFactoryForRack();
			ISoundfieldFactory* ParentSoundfieldFactory = ParentSoundfieldEndpointRack->GetSoundfieldEndpointForRack();

			if (ChildSoundfieldFactory && ParentSoundfieldFactory)
			{
				return ChildSoundfieldFactory->CanTranscodeToSoundfieldFormat(ParentSoundfieldFactory->GetSoundfieldFormatName(), *(ParentSoundfieldEndpointRack->GetEncodingSettings()->GetProxy()))
					|| ParentSoundfieldFactory->CanTranscodeFromSoundfieldFormat(ChildSoundfieldFactory->GetSoundfieldFormatName(), *(ChildSoundfieldRack->GetSoundfieldEncodingSettings()->GetProxy()));
			}
			else
			{
				return true;
			}*/

			return true;
		}
	}

	// Otherwise, these racks are compatible.
	return true;
}

#if WITH_EDITOR

CRIWARECORE_API void AtomRackUtils::RefreshEditorForRack(const UAtomRackBase* InRack)
{
	if (!GEditor || !InRack)
	{
		return;
	}

	TWeakObjectPtr<UAtomRackBase> WeakRack = TWeakObjectPtr<UAtomRackBase>(const_cast<UAtomRackBase*>(InRack));

	// Since we may be in the middle of a PostEditProperty call,
	// Dispatch a command to close and reopen the editor window next tick.
	AsyncTask(ENamedThreads::GameThread, [WeakRack]
	{
		if (WeakRack.IsValid())
		{
			UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			TArray<IAssetEditorInstance*> RackEditors = EditorSubsystem->FindEditorsForAsset(WeakRack.Get());
			for (IAssetEditorInstance* Editor : RackEditors)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				Editor->CloseWindow(EAssetEditorCloseReason::EditorRefreshRequested);
#else
				Editor->CloseWindow();
#endif
			}

			EditorSubsystem->OpenEditorForAsset(WeakRack.Get());
		}
	});
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
