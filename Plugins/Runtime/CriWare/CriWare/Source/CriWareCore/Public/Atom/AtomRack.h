
#pragma once

#include "CoreMinimal.h"

#include "Atom.h"
#include "Modulation/AtomModulationDestination.h"
#include "Extensions/IAtomEndpoint.h"

#include "AtomRack.generated.h"

// Forward Declarations
class UEdGraph;
class UAtomDspBusSetting;
class UAtomBus;
class FAtomRuntime;
class FAtomRackSubmixListener;

/**
 * Called when a new AtomRack level meter measured value is generated on the given Atom runtime id (different for multiple PIE). Array is an level value for each channel.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomRackLevelMeterMeasure, const TArray<FAtomLevelMeterMeasure>&, Measures);

struct FAtomLevelMeterSettings
{
	/** Number of seconds between meter measurements */
	float AnalysisPeriod = 0.02f;

	/** Peak hold time, in milliseconds */
	int32 PeakHoldTime = 1000;
};

/**
 * Called when a new AtomRack loudness meter measured value is generated on the given Atom runtime id (different for multiple PIE).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomRackLoudnessMeterMeasure, const FAtomLoudnessMeterMeasure&, Measure);

struct FAtomLoudnessMeterSettings
{
	/** Time in seconds for short-term average loudness */
	int32 ShortTermTime = 3;

	/** Time in seconds for long-term average loudness, in seconds */
	int32 IntegratedTime = 600;
};

/**
 * Called when a new AtomRack loudness meter measured value is generated on the given Atom runtime id (different for multiple PIE). Array is an level value for each channel.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomRackTruePeakMeterMeasure, const TArray<FAtomTruePeakMeterMeasure>& , Measure);

struct FAtomTruePeakMeterSettings
{
	/** Number of seconds between meter measurements */
	float AnalysisPeriod = 0.02f;

	/** Peak hold time, in milliseconds */
	int32 PeakHoldTime = 1000;

	/** Whether to clip samples. */
	bool bSampleClipping = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAtomRackRenderedSamples, int32, RackID, int64, NumSamples, int32, SampleRate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomRackPerformanceMonitorResult, const FAtomPerformanceMonitorResult&, Result);

#if WITH_EDITOR

/** Interface for Atom rack submix graph interaction with the CriWareCoreEditor module. */
class IAtomRackEditor
{
public:
	virtual ~IAtomRackEditor() {}

	/** Refreshes the sound class graph links. */
	virtual void RefreshGraphLinks(UEdGraph* SoundClassGraph) = 0;
};
#endif

UCLASS(config = CriWare, abstract, HideCategories = Object, EditInlineNew, BlueprintType)
class CRIWARECORE_API UAtomRackBase 
	: public UObject
{
	GENERATED_BODY()

public:

	UAtomRackBase(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITORONLY_DATA
	/** EdGraph based representation of the AtomRack */
	TObjectPtr<UEdGraph> AtomRackGraph;
#endif

	// Auto-manage enabling and disabling the submix as a CPU optimization. It will be disabled if the submix and all child submixes are silent. It will re-enable if a sound is sent to the submix or a child submix is audible.
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AutoDisablement)
	//bool bAutoDisable = true;

	// The minimum amount of time to wait before automatically disabling a submix if it is silent. Will immediately re-enable if source audio is sent to it. 
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AutoDisablement, meta = (EditCondition = "bAutoDisable"))
	//float AutoDisableTime = 0.01f;

	// Child racks to this rack
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Rack")
	TArray<TObjectPtr<UAtomRackBase>> ChildRacks;

	// Start level meter measuring the levels of Atom rack output. Register with OnSubmixEnvelope to receive envelope follower data in BP.
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Meter", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 1))
	void StartLevelMeterMeasuring(const UObject* WorldContextObject, float AnalysisPeriod = 0.02f, int32 PeakHoldTime = 100);

	void StartLevelMeterMeasuring(FAtomRuntime* InAtomRuntime, float AnalysisPeriod = 0.02f, int32 PeakHoldTime = 100);

	// Stop level meter measuring the levels the Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Meter", meta = (WorldContext = "WorldContextObject"))
	void StopLevelMeterMeasuring(const UObject* WorldContextObject);

	void StopLevelMeterMeasuring(FAtomRuntime* InAtomRuntime);

	/**
	 *	Adds an level meter delegate to the Atom rack when level meter measure is enabled on this rack.
	 *	@param	OnAtomRackLevelMeterMeasureBP	Event to fire when new envelope data is available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Meter", meta = (WorldContext = "WorldContextObject"))
	void AddLevelMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP);

	UFUNCTION(BlueprintCallable, Category = "Atom|Level Meter", meta = (WorldContext = "WorldContextObject"))
	void RemoveLevelMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP);

	static FAtomLevelMeterSettings GetLevelMeterSettings(float AnalysisPeriod, int32 PeakHoldTime);

	// Start loudness meter measuring the loudness of Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Loudness Meter", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 1))
	void StartLoudnessMeterMeasuring(const UObject* WorldContextObject, int32 ShortTermTime = 3, int32 IntegratedTime = 600);

	void StartLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime, int32 ShortTermTime = 3, int32 IntegratedTime = 600);

	// Stop level meter measuring the loudness the Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Loudness Meter", meta = (WorldContext = "WorldContextObject"))
	void StopLoudnessMeterMeasuring(const UObject* WorldContextObject);

	void StopLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime);

	// Reset level meter measuring the loudness the Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Loudness Meter", meta = (WorldContext = "WorldContextObject"))
	void ResetLoudnessMeterMeasuring(const UObject* WorldContextObject);

	void ResetLoudnessMeterMeasuring(FAtomRuntime* InAtomRuntime);

	/**
	 *	Adds a loudness meter delegate to the Atom rack when loudness meter measure is enabled on this rack.
	 *	@param	OnAtomRackLevelMeterMeasureBP	Event to fire when new envelope data is available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Loudness Meter", meta = (WorldContext = "WorldContextObject"))
	void AddLoudnessMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP);

	UFUNCTION(BlueprintCallable, Category = "Atom|Loudness Meter", meta = (WorldContext = "WorldContextObject"))
	void RemoveLoudnessMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP);

	static FAtomLoudnessMeterSettings GetLoudnessMeterSettings(int32 ShortTermTime, int32 IntegratedTime);

	// Start true peak meter measuring the true peak of Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|True Peak Meter", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 1))
	void StartTruePeakMeterMeasuring(const UObject* WorldContextObject, float AnalysisPeriod = 0.02f, int32 PeakHoldTime = 100, bool bSampleClipping = false);

	void StartTruePeakMeterMeasuring(FAtomRuntime* InAtomRuntime, float AnalysisPeriod = 0.02f, int32 PeakHoldTime = 100, bool bSampleClipping = false);

	// Stop level meter measuring the true peak the Atom rack output.
	UFUNCTION(BlueprintCallable, Category = "Atom|True Peak Meter", meta = (WorldContext = "WorldContextObject"))
	void StopTruePeakMeterMeasuring(const UObject* WorldContextObject);

	void StopTruePeakMeterMeasuring(FAtomRuntime* InAtomRuntime);

	/**
	 *	Adds a true peak meter delegate to the Atom rack when level meter measure is enabled on this rack.
	 *	@param	OnAtomRackTruePeakMeterMeasureBP	Event to fire when new envelope data is available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|True Peak Meter", meta = (WorldContext = "WorldContextObject"))
	void AddTruePeakMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP);

	UFUNCTION(BlueprintCallable, Category = "Atom|True Peak Meter", meta = (WorldContext = "WorldContextObject"))
	void RemoveTruePeakMeterDelegate(const UObject* WorldContextObject, const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP);

	static FAtomTruePeakMeterSettings GetTruePeakMeterSettings(float AnalysisPeriod, int32 PeakHoldTime, bool bSampleClipping);

	/**
	 *	Adds a rendered samples count delegate to the Atom rack.
	 *	@param	OnAtomRackRenderedSamplesBP	Event to fire when counter was increased.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Rendered Samples", meta = (WorldContext = "WorldContextObject"))
	void AddRenderedSamplesDelegate(const UObject* WorldContextObject, const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSamplesBP);

	UFUNCTION(BlueprintCallable, Category = "Atom|Rendered Samples", meta = (WorldContext = "WorldContextObject"))
	void RemoveRenderedSamplesDelegate(const UObject* WorldContextObject, const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSamplesBP);

	/**
	 *	Adds a Performace Monitor delegate to the Atom rack.
	 *	@param	OnAtomPerformanceMonitorResultBP	Event to fire when new results are available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Performance Monitor", meta = (WorldContext = "WorldContextObject"))
	void AddPerformanceMonitorDelegate(const UObject* WorldContextObject, const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP);

	UFUNCTION(BlueprintCallable, Category = "Atom|Performance Monitor", meta = (WorldContext = "WorldContextObject"))
	void RemovePerformanceMonitorDelegate(const UObject* WorldContextObject, const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP);

	// Reset counters in the Performance Monitor results.
	UFUNCTION(BlueprintCallable, Category = "Atom|Performance Monitor", meta = (WorldContext = "WorldContextObject"))
	void ResetPerformanceMonitor(const UObject* WorldContextObject);

	void ResetPerformanceMonitor(FAtomRuntime* InAtomRuntime);

protected:

	//~ Begin UObject Interface.
	virtual FString GetDesc() override;
	virtual void BeginDestroy() override;
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;

public:

	/** The main out bus name. */
	static const FString MainBusName;

	// Atom Rack Editor functionality
#if WITH_EDITOR

	/**
	* @return true if the child sound class exists in the tree
	*/
	bool RecurseCheckChild(const UAtomRackBase* ChildAtomRack) const;

	/**
	* Add Referenced objects
	*
	* @param	InThis		AtomRack we are adding references from.
	* @param	Collector	Reference Collector
	*/
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:

	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface.

private:

	static TArray<TObjectPtr<UAtomRackBase>> BackupChildRacks;
#endif // WITH_EDITOR
};

/**
 * This rack class can be derived from for racks that output to a parent rack.
 */
UCLASS(config = CriWare, abstract, HideCategories = Object, EditInlineNew, BlueprintType)
class CRIWARECORE_API UAtomRackWithParentBase
	: public UAtomRackBase
{
	GENERATED_BODY()

public:

	UAtomRackWithParentBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Rack")
	TObjectPtr<UAtomRackBase> ParentRack;

	// Master Bus - always one defined per rack
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buses", Instanced, meta = (ShowOnlyInnerProperties))
	TObjectPtr<UAtomBus> MasterBus;

	/**
	* Set the parent rack of this AtomRack, removing it as a child from its previous owner
	*
	* @param	InParentRack	The new parent rack of this
	*/
	void SetParentRack(UAtomRackBase* InParentRack);

protected:
	//~ Begin UObject Interface.
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
#endif 
};

// Whether to use linear or decibel values for audio gains
UENUM(BlueprintType)
enum class EAtomGainParamMode : uint8
{
	Linear = 0,
	Decibels,
};

/**
 * Atom Rack class meant for applying an effect to the downmixed sum of multiple audio sources.
 */
UCLASS(config = CriWare, HideCategories = Object, EditInlineNew, BlueprintType)
class CRIWARECORE_API UAtomRack
	: public UAtomRackWithParentBase
{
	GENERATED_BODY()

public:

	UAtomRack(const FObjectInitializer& ObjectInitializer);

	/** Mute this rack when the application is muted or in the background. Used to prevent rack effect tails from continuing when tabbing out of application or if application is muted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atom Rack")
	uint8 bMuteWhenBackgrounded : 1;

	// Number of channels to input.
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", meta = (ClampMin = "0", ClampMax = "16"))
	int32 NumChannels;

	// Number and configuration of channels to output to buses.
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer")
	EAtomSpeakerChannelMap SpeakerChannelMap;

	// Sample Rate to output.
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", meta = (ClampMin = "0", Units = Hz))
	int32 SamplingRate;

	// Atom server frequency to use.
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", meta = (ClampMin = "0.0", ClampMax = "120.0", Units = Hz))
	float ServerFrequency;

	// AtomConfig to use with this rack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, BlueprintSetter = SetAtomConfig, Category = "Buses")
	TObjectPtr<const UAtomConfig> AtomConfig;

	// The name of the DSP bus setting in use
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, BlueprintGetter = GetDspBusSettingName, BlueprintSetter = SetDspBusSettingName, Category = "Buses")
	FString DspBusSettingName;

	// Attached Dsp Bus Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDspBusSetting, AssetRegistrySearchable, Category = "Buses")
	TObjectPtr<const UAtomDspBusSetting> DspBusSetting;

	// Initial snapshot. Will use this snapshot at initialisation of the rack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter = GetSnapshot, BlueprintSetter = SetSnapshot, Category = "Snapshot", meta = (DisplayName = "Snapshot"))
	FString SnapshotName;

	// Buses used in this rack.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buses")
	TArray<TObjectPtr<UAtomBus>> Buses;

	/** Change the Atom config to use with this rack. */
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Buses")
	void SetAtomConfig(const UAtomConfig* InAtomConfig);
	/** Change the currently applied DspBusSetting by the name. */
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Buses")
	void SetDspBusSettingName(const FString& InDspBusSettingName);
	/** Get the name of currently applied DspBusSetting. */
	UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "Buses")
	const FString& GetDspBusSettingName() const;

	/** Setup a DSP bus settings and udpate buses. Buses with name that does not exist anymore in settings are wiped out. */
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Buses")
	void SetDspBusSetting(const UAtomDspBusSetting* InDspBusSetting) { SetDspBusSetting(InDspBusSetting, false); }
	
	/** Setup a DSP bus settings. If bResetBuses is true, buse are wiped out and reset otherwise buses with name that does not exist anymore in settings are wiped out. */
	void SetDspBusSetting(const UAtomDspBusSetting* InDspBusSetting, bool bResetBuses);

	/** Setup a snapshot to this rack and apply it without fading. */
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Snapshot")
	void SetSnapshot(const FString& InSnapshot) { SetSnapshot(InSnapshot, 0.0f); }
	
	/** Setup a snapshot to this rack and apply it with fade time in seconds. */
	void SetSnapshot(const FString& InSnapshot, float FadeTime);

	/** Setup a snapshot to this rack and apply it with fade time in seconds. */
	UFUNCTION(BlueprintCallable, Category = "Snapshot", meta = (DisplayName = "SetSnapshot"))
	void BP_SetSnapshot(const FString& InSnapshot, float FadeTime) { SetSnapshot(InSnapshot, FadeTime); }
	
	/** Get the currently applied snapshot to this rack. */
	UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "Snapshot")
	const FString& GetSnapshot() const;

	//static FSoundSpectrumAnalyzerSettings GetSpectrumAnalyzerSettings(EFFTSize FFTSize, EFFTPeakInterpolationMethod InterpolationMethod, EFFTWindowType WindowType, float HopSize, EAudioSpectrumType SpectrumType);

	//static FSoundSpectrumAnalyzerDelegateSettings GetSpectrumAnalysisDelegateSettings(const TArray<FSoundSubmixSpectralAnalysisBandSettings>& InBandSettings, float UpdateRate, float DecibelNoiseFloor, bool bDoNormalize, bool bDoAutoRange, float AutoRangeAttackTime, float AutoRangeReleaseTime);
protected:

	// Apply DspSetting to runtime
	bool ApplyDspBusSetting();
	// Sync buses from DspSetting
	void ResetBuses(bool bApplyToAtom = true);
	// Updates buses from DspSetting (don't change values of existing buses)
	void UpdateBuses();
	// Sync buses values from runtime values
	void SyncBusesFromRuntime();   // -> GetBusSettings()
	// Sync runtime buses from rack buses values
	void SyncBusesToRuntime() const;  // -> ApplyBusesSettings()
	// Apply Snapshot to runtime
	bool ApplySnapshot(float FadeSeconds);

	void ProcessAudio(FAtomRuntime* AtomRuntime);
	void StartSendToUnrealSubmix(FAtomRuntime* AtomRuntime);
	void StopSendToUnrealSubmix();
	TSharedPtr<FAtomRackSubmixListener, ESPMode::ThreadSafe> SubmixListener;

	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_EDITOR
	TWeakObjectPtr<const UAtomConfig> CachedAtomConfig;
	FString CachedDspBusSettingName;
#endif // WITH_EDITOR

	friend class FAtomRuntime;

private:
#if WITH_EDITORONLY_DATA
	// Replaced by FString SnapshotName.
	UPROPERTY()
	FName Snapshot_DEPRECATED;
#endif // WITH_EDITORONLY_DATA
};

/**
 * Sound Rack class meant for use with soundfield formats, such as Ambisonics.
 */
UCLASS(Config = CriWare, HideCategories = Object, EditInlineNew, BlueprintType, Meta = (DisplayName = "Sound Rack Soundfield"))
class CRIWARECORE_API UAtomSoundfieldRack
	: public UAtomRackWithParentBase
{
	GENERATED_BODY()

public:

	//IAtomSoundfieldFactory* GetSoundfieldFactoryForRack() const;
	//const UAtomSoundfieldEncodingSettingsBase* GetSoundfieldEncodingSettings() const;
	//TArray<UAtomSoundfieldEffectBase*> GetSoundfieldProcessors() const;

public:

	/** Currently used format. */
	//UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = Soundfield)
	//FName SoundfieldEncodingFormat;

	/** Type of the soundrenderer endpoint to apply to child racks. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSoundfieldRendererType SoundfieldRendererType;

	/** Which encoding settings to use the sound field. */
	//UPROPERTY(EditAnywhere, Category = Soundfield)
	//TObjectPtr<UAtomSoundfieldEncodingSettingsBase> EncodingSettings;

	/** Soundfield effect chain to use for the sound field. */
	//UPROPERTY(EditAnywhere, Category = Soundfield)
	//TArray<TObjectPtr<UAtomSoundfieldEffectBase>> SoundfieldEffectChain;

	// Traverses parent submixes until we find a submix that doesn't inherit its soundfield format.
	EAtomSoundfieldRendererType GetRackType() const;

	//UPROPERTY()
	//TSubclassOf<UAtomSoundfieldEncodingSettingsBase> EncodingSettingsClass;

	// Traverses parent submixes until we find a submix that specifies encoding settings.
	//const UAtomSoundfieldEncodingSettingsBase* GetEncodingSettings() const;

	// This function goes through every child submix and the parent submix to ensure that they have a compatible format with this submix's format.
	void SanitizeLinks();

protected:

	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};

/**
 * AtomEndpointRack class meant for sending audio to an external endpoint, such as controller haptics or an additional audio device.
 */
UCLASS(Config = CriWare, HideCategories = Object, EditInlineNew, BlueprintType, Meta = (DisplayName = "Atom Rack Endpoint"))
class CRIWARECORE_API UAtomEndpointRack
	: public UAtomRackBase
{
	GENERATED_BODY()

public:

	UAtomEndpointRack(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITORONLY_DATA
	void Serialize(FArchive& Ar);
#endif

	const UAtomEndpointSettingsBase* GetEndpointSettings(const TSubclassOf<UAtomEndpointSettingsBase>& EndpointSettingsClass) const;

	/** Type of the soundrenderer endpoint to apply to child racks. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSoundRendererType SoundRendererType;

#if WITH_EDITORONLY_DATA
	/** This will select the port number of the sound renderer type to use. */
	UPROPERTY()
	int32 OutputPort_DEPRECATED = 0;
#endif // WITH_EDITORONLY_DATA

	/** Settings for the endpoint. Only one endpoint is used at time but you can set multiple settings for each platforms. */
	UPROPERTY(EditAnywhere, Category = "Sound Renderer")
	TArray<TObjectPtr<UAtomEndpointSettingsBase>> EndpointSettings;
};

/**    
 * Sound Rack class meant for sending soundfield-encoded audio to an                            external endpoint, such as a hardware binaural renderer that supports ambisonics.
 */
UCLASS(Config = CriWare, HideCategories = Object, EditInlineNew, BlueprintType, Meta = (DisplayName = "Sound Rack Soundfield Endpoint"))
class CRIWARECORE_API UAtomSoundfieldEndpointRack
	: public UAtomRackBase
{
	GENERATED_BODY()

public:
	//IAtomSoundfieldEndpointFactory* GetSoundfieldEndpointForSubmix() const;
	//const UAtomSoundfieldEndpointSettingsBase* GetEndpointSettings() const;
	//const UAtomSoundfieldEncodingSettingsBase* GetEncodingSettings() const;
	//TArray<USAtomoundfieldEffectBase*> GetSoundfieldProcessors() const;
public:
	/** Currently used format. */
	//UPROPERTY(EditAnywhere, Category = Endpoint, AssetRegistrySearchable)
	//FName SoundfieldEndpointType;

	/** Type of the soundrenderer endpoint to apply to child racks. */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSoundfieldRendererType SoundfieldRendererType;

	//UPROPERTY()
	//TSubclassOf<UAudioEndpointSettingsBase> EndpointSettingsClass;

	// This function goes through every child submix and the parent submix to ensure that they have a compatible format.
	void SanitizeLinks();

	//UPROPERTY(EditAnywhere, Category = Endpoint)
	//TObjectPtr<UAtomSoundfieldEndpointSettingsBase> EndpointSettings;

	//UPROPERTY()
	//TSubclassOf<USoundfieldEncodingSettingsBase> EncodingSettingsClass;

	//UPROPERTY(EditAnywhere, Category = Soundfield)
	//TObjectPtr<USoundfieldEncodingSettingsBase> EncodingSettings;

	//UPROPERTY(EditAnywhere, Category = Soundfield)
	//TArray<TObjectPtr<USoundfieldEffectBase>> SoundfieldEffectChain;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

namespace AtomRackUtils
{
	CRIWARECORE_API bool AreRackSoundTypeCompatible(const UAtomRackBase* ChildRack, const UAtomRackBase* ParentRack);

#if WITH_EDITOR
	CRIWARECORE_API void RefreshEditorForRack(const UAtomRackBase* InRack);
#endif
}
