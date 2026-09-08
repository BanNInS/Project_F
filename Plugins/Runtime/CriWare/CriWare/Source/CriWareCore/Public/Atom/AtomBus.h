/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBus.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"

#include "Atom.h"
#include "AtomBusEffectPreset.h" 
#include "Modulation/AtomModulationDestination.h"

#include "Extensions/IAtomEndpoint.h"

#include "AtomBus.generated.h"

// Forward Defenitions
class FAtomRuntime;
namespace Atom
{
	class FMixerSubmix;
}

UENUM()
enum class EAtomSpatializationType : uint8
{
	Panning,
	SendToChannel
};

UENUM()
enum class EAtomBusSendType : uint8
{
	PreVolume, // post effect
	PostVolume, // post fader
	PostPanning // pre effect
};

/*
 * FAtomChannelLevelMatrix structure
 *****************************************************************************/

USTRUCT(Blueprintable)
struct CRIWARECORE_API FAtomChannelLevelMatrix
{
	GENERATED_BODY()

public:

	FAtomChannelLevelMatrix();

	/** */
	FAtomChannelLevelMatrix(int NumInputChannels, int NumOutputChannels);

	// currently speaker format seams fixed in Atom
	/*FAtomChannelLevelMatrix(const TArray<EAtomSpeaker>& InInputChannelArray, int NumOutputChannels)
	{
		InputChannelArray = InInputChannelArray;
		SetupChannelArrayFromNumChannels(OutputChannelArray, NumOutputChannels);
	}

	FAtomChannelLevelMatrix(const TArray<EAtomSpeaker>& InInputChannelArray, const TArray<EAtomSpeaker>& InOutputChannelArray)
	{
		InputChannelArray = InInputChannelArray;
		OutputChannelArray = InOutputChannelArray;
	}*/

	/** */
	bool SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level);

	/** */
	float GetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel) const;

	/** */
	FORCEINLINE const float* GetLevelMatixData() const { return LevelMatrix.GetData(); }

	/** */
	FORCEINLINE int32 GetNumInputChannels() const { return InputChannelArray.Num(); }

	/** */
	FORCEINLINE int32 GetNumOutputChannels() const { return OutputChannelArray.Num(); }

	/** */
	void SetIdentity();
	
	void UpMix();
	void DownMix();

private:

	void SetupChannelArrayFromNumChannels(TArray<EAtomSpeaker>& ChannelArray, int NumChannels);

	// Layout
	//UPROPERTY()
	TArray<EAtomSpeaker> InputChannelArray;
	//UPROPERTY()
	TArray<EAtomSpeaker> OutputChannelArray;

	// Values
	UPROPERTY(EditInstanceOnly, Category = "Level Mixer")
	TArray<float> LevelMatrix;
};

/*
 * FAtomPan structure
 *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomPanning
{
	GENERATED_BODY()

public:

	FAtomPanning();
	FAtomPanning(float Volume, float Angle, float Distance, float Wideness, float Spread);

	/** */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", SliderExponent = "5"))
	float Volume;

	/** */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ClampMin = -180.f, ClampMax = 180.f, UIMin = -180.f, UIMax = 180.f, SliderExponent = "1"))
	float Angle;

	/** */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, SliderExponent = 5))
	float Distance;

	/** */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, SliderExponent = 5))
	float Wideness;

	/** */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, SliderExponent = 5))
	float Spread;
};

/*
 * FAtomBusSend structure
 *****************************************************************************/

USTRUCT()
struct FAtomBusSend
{
	GENERATED_BODY()

public:
	FAtomBusSend();
	FAtomBusSend(const UAtomBus* InDestinationBus, float InLevel = 0.0f, EAtomBusSendType SendType = EAtomBusSendType::PostPanning);
	
	/** */
	UPROPERTY(VisibleAnyWhere, Category = "Send Level")
	TObjectPtr<const UAtomBus> DestinationBus;

	/** */
	UPROPERTY(EditAnywhere, Category = "Send Level", meta = (ClampMin = 0.f, ClampMax = 1.f, UIMin = 0.f, UIMax = 1.f, SliderExponent = 5))
	float Level;

#if WITH_EDITORONLY_DATA
	/** */
	UPROPERTY(VisibleAnyWhere, Category = "Send Level")
	EAtomBusSendType SendType;
#endif
};

/**
* Called when a recorded file has finished writing to disk.
*
*/
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomBusRecordedFileDone, const UAtomSoundWave*, ResultingSoundWave);

/**
* Called when a new AtomBus envelope value is generated on the given Atom runtime id (different for multiple PIE). Array is an envelope value for each channel.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomBusEnvelope, const TArray<float>&, Envelope);

/**
* Called when a new AtomBus spectral analysis value is generated on the given Atom runtime id (different for multiple PIE). Array is an mangitude value for each spectra.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomBusSpectralAnalysis, const TArray<float>&, Magnitudes);


UENUM(BlueprintType)
enum class EAtomFFTSize : uint8
{
	// 512
	DefaultSize,

	// 64
	Min,

	// 256
	Small,

	// 512
	Medium,

	// 1024
	Large,

	// 2048
	VeryLarge,

	// 4096
	Max
};

UENUM()
enum class EAtomFFTPeakInterpolationMethod : uint8
{
	NearestNeighbor,
	Linear,
	Quadratic,
	ConstantQ,
};

UENUM()
enum class EAtomFFTWindowType : uint8
{
	// No window is applied. Technically a boxcar window.
	None,

	// Mainlobe width of -3 dB and sidelobe attenuation of ~-40 dB. Good for COLA.
	Hamming,

	// Mainlobe width of -3 dB and sidelobe attenuation of ~-30dB. Good for COLA.
	Hann,

	// Mainlobe width of -3 dB and sidelobe attenuation of ~-60db. Tricky for COLA.
	Blackman
};

UENUM(BlueprintType)
enum class EAtomSpectrumType : uint8
{
	// Spectrum frequency values are equal to magnitude of frequency.
	MagnitudeSpectrum,

	// Spectrum frequency values are equal to magnitude squared.
	PowerSpectrum,

	// Returns decibels (0.0 dB is 1.0)
	Decibel,
};

struct FAtomSoundSpectrumAnalyzerSettings
{
	// FFTSize used in spectrum analyzer.
	EAtomFFTSize FFTSize;

	// Type of window to apply to audio.
	EAtomFFTWindowType WindowType;

	// Metric used when analyzing spectrum. 
	EAtomSpectrumType SpectrumType;

	// Interpolation method used when getting frequencies.
	EAtomFFTPeakInterpolationMethod  InterpolationMethod;

	// Hopsize between audio windows as a ratio of the FFTSize.
	float HopSize;
};

struct FAtomSoundSpectrumAnalyzerDelegateSettings
{
	// Settings for individual bands.
	TArray<FAtomBusSpectralAnalysisBandSettings> BandSettings;

	// Number of times a second the delegate is triggered.
	float UpdateRate;

	// The decibel level considered silence.
	float DecibelNoiseFloor;

	// If true, returned values are scaled between 0 and 1.
	bool bDoNormalize;

	// If true, the band values are tracked to always have values between 0 and 1. 
	bool bDoAutoRange;

	// The time in seconds for the range to expand to a new observed range.
	float AutoRangeAttackTime;

	// The time in seconds for the range to shrink to a new observed range.
	float AutoRangeReleaseTime;
};

/*
 * Mixer 2 UAtomEndpoint class
 *****************************************************************************/
UCLASS(Config = CriWare, abstract, HideCategories = Object, EditInlineNew, BlueprintType, MinimalAPI)
class UAtomEndpointBase
	: public UObject
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomEndpointBase(const FObjectInitializer& ObjectInitializer);
};

UCLASS(Config = CriWare, HideCategories = Object, MinimalApi, EditInlineNew, BlueprintType, Meta = (DisplayName = "Atom Endpoint"))
class UAtomEndpoint
	: public UAtomEndpointBase
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomEndpoint(const FObjectInitializer& ObjectInitializer);

	CRIWARECORE_API const UAtomEndpointSettingsBase* GetEndpointSettings(const TSubclassOf<UAtomEndpointSettingsBase>& EndpointSettingsClass) const;

#if WITH_EDITORONLY_DATA
	CRIWARECORE_API void Serialize(FArchive& Ar);
#endif

	/** Number and configuration of channels to output to buses. (Currently redefining mixer output type is not available in Atom.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSpeakerChannelMap SpeakerChannelMap;

	/** Type of the sound renderer endpoint to apply. (Currently redefining mixer output type is not available in Atom.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSoundRendererType SoundRendererType;

	/** Settings for the endpoint. First endpoint settings for the current platform used but you can set multiple settings for each platforms. */
	UPROPERTY(EditAnywhere, Category = "Sound Renderer")
	TArray<TObjectPtr<UAtomEndpointSettingsBase>> EndpointSettings;
};

UCLASS(Config = CriWare, HideCategories = Object, MinimalApi, EditInlineNew, BlueprintType, Meta = (DisplayName = "Atom Soundfield Endpoint"))
class UAtomSoundfieldEndpoint
	: public UAtomEndpointBase
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomSoundfieldEndpoint(const FObjectInitializer& ObjectInitializer);

	/** Type of the soundfield endpoint to apply. (Currently redefining mixer output type is not available in Atom.) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Sound Renderer", Meta = (Tooltip = ""))
	EAtomSoundfieldRendererType SoundfieldRendererType;
};

/*
 * UAtomBus class
 *****************************************************************************/

 // Forward Declarations
class UAtomRack;

UCLASS(hidecategories = Object, BlueprintType, MinimalApi)
class UAtomBus
	: public UObject
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomBus(const FObjectInitializer& ObjectInitializer);
	
	void Init(const FString& InBusName, int InBusIndex);

	//~ Begin UObject Interface
	CRIWARECORE_API virtual void PostInitProperties() override;
#if WITH_EDITOR

	/**
	* Add Referenced objects
	*
	* @param	InThis		AtomBus we are adding references from.
	* @param	Collector	Reference Collector
	*/
	CRIWARECORE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

protected:

	//CRIWARECORE_API virtual bool CanEditChange(const FProperty* InProperty) const override;
	CRIWARECORE_API virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	CRIWARECORE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	CRIWARECORE_API virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

	static TArray<TObjectPtr<UAtomBusEffectPreset>> BackupBusEffectChain;
#endif
	//~ End Uobject Interface

public:

	/** The attack time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the envelope value of sounds played with this DSP bus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EnvelopeFollower, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerAttackTime;

	/** The release time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the envelope value of sounds played with this DSP bus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EnvelopeFollower, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerReleaseTime;

	/** The output volume of the bus in Decibels. Applied after bus effects and analysis are performed. */
	UPROPERTY(EditAnywhere, BlueprintSetter = SetOutputVolumeModulation, Category = "Level Mixer", meta = (DisplayName = "Output Volume (dB)", AtomParam = "Volume", AtomParamClass = "AtomModulationParameterVolume"))
	FAtomSoundModulationSettings OutputVolumeModulation;

	/** The wet level of the bus in Decibels. Applied after bus effects and analysis are performed. */
	UPROPERTY(EditAnywhere, BlueprintSetter = SetWetVolumeModulation, Category = "Level Mixer", meta = (DisplayName = "Wet Level (dB)", AtomParam = "Volume", AtomParamClass = "AtomModulationParameterVolume"))
	FAtomSoundModulationSettings WetLevelModulation;

	/** The dry level of the bus in Decibels. Applied before bus effects and analysis are performed. */
	UPROPERTY(EditAnywhere, BlueprintSetter = SetDryVolumeModulation, Category = "Level Mixer", meta = (DisplayName = "Dry Level (dB)", AtomParam = "Volume", AtomParamClass = "AtomModulationParameterVolume"))
	FAtomSoundModulationSettings DryLevelModulation;

	/** The type of spatialization of the multi-channels for this bus. */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (DisplayAfter = OutputVolumeModulation))
	EAtomSpatializationType SpatializationType;

	/** The level matrix for each channels for this bus. */
	UPROPERTY(EditAnywhere, Category = "Level Mixer", meta = (ShowOnlyInnerProperties, DisplayAfter=Panning, EditCondition="SpatializationType == EAtomSpatializationType::SendToChannel"))
	FAtomChannelLevelMatrix ChannelLevelMatrix;

	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Level Mixer", meta = (DisplayAfter = ChannelLevelMatrix))
	TArray<FAtomBusSend> Sends;

	/** Whether to override settings of the current endpoint for this bus. */
	UPROPERTY(BlueprintReadOnly, Category = "Mixer Output", meta = (InlineEditConditionToggle))
	bool bOverrideEndpoint = false;

	/** The current mixer output endpoint for this bus. Overrides to setup specfic output settings. */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Mixer Output", meta=(EditCondition = "bOverrideEndpoint"))
	TObjectPtr<UAtomEndpointBase> Endpoint = nullptr;

	// Blueprint delegate for when a recorded file is finished exporting.
	//UPROPERTY(BlueprintAssignable)
	//FOnSubmixRecordedFileDone OnSubmixRecordedFileDone;

	// Start recording the audio from this Atom bus.
	//UFUNCTION(BlueprintCallable, Category = "Atom|Bounce", meta = (WorldContext = "WorldContextObject", DisplayName = "Start Recording Submix Output"))
	//CRIWARECORE_API void StartRecordingOutput(const UObject* WorldContextObject, float ExpectedDuration);

	//CRIWARECORE_API void StartRecordingOutput(FAtomRuntime* InAtomRuntime, float ExpectedDuration);

	// Finish recording the audio from this Atom bus and export it as a wav file or a UAtomSoundWave.
	//UFUNCTION(BlueprintCallable, Category = "Atom|Bounce", meta = (WorldContext = "WorldContextObject", DisplayName = "Finish Recording Submix Output"))
	//CRIWARECORE_API void StopRecordingOutput(const UObject* WorldContextObject, EAtomRecordingExportType ExportType, const FString& Name, FString Path, UAtomSoundWave* ExistingSoundWaveToOverwrite = nullptr);

	//CRIWARECORE_API void StopRecordingOutput(FAtomRuntime* InAtomRuntime, EAtomRecordingExportType ExportType, const FString& Name, FString Path, UAtomSoundWave* ExistingSoundWaveToOverwrite = nullptr);

	// Start envelope following the Atom bus output. Register with OnAtomBusEnvelope to receive envelope follower data in BP.
	UFUNCTION(BlueprintCallable, Category = "Atom|EnvelopeFollowing", meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void StartEnvelopeFollowing(const UObject* WorldContextObject);

	CRIWARECORE_API void StartEnvelopeFollowing(FAtomRuntime* InAtomRuntime);

	// Stop envelope following the Atom bus output.
	UFUNCTION(BlueprintCallable, Category = "Atom|EnvelopeFollowing", meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void StopEnvelopeFollowing(const UObject* WorldContextObject);

	CRIWARECORE_API void StopEnvelopeFollowing(FAtomRuntime* InAtomRuntime);

	/**
	 *	Adds an envelope follower delegate to the Atom bus when envelope following is enabled on this DSP bus.
	 *	@param	OnAtomBusEnvelopeBP	Event to fire when new envelope data is available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|EnvelopeFollowing", meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void AddEnvelopeFollowerDelegate(const UObject* WorldContextObject, const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP);

	/**
	 *	Adds a spectral analysis delegate to receive notifications when this Atom bus has spectrum analysis enabled.
	 *	@param	InBandsettings					The frequency bands to analyze and their envelope-following settings.
	 *  @param  OnAtomBusSpectralAnalysisBP		Event to fire when new spectral data is available.
	 *	@param	UpdateRate						How often to retrieve the data from the spectral analyzer and broadcast the event. Max is 30 times per second.
	 *	@param  InterpMethod                    Method to used for band peak calculation.
	 *	@param  SpectrumType                    Metric to use when returning spectrum values.
	 *	@param  DecibelNoiseFloor               Decibel Noise Floor to consider as silence when using a Decibel Spectrum Type.
	 *	@param  bDoNormalize                    If true, output band values will be normalized between zero and one.
	 *	@param  bDoAutoRange                    If true, output band values will have their ranges automatically adjusted to the minimum and maximum values in the audio. Output band values will be normalized between zero and one.
	 *	@param  AutoRangeAttackTime             The time (in seconds) it takes for the range to expand to 90% of a larger range.
	 *	@param  AutoRangeReleaseTime            The time (in seconds) it takes for the range to shrink to 90% of a smaller range.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Spectrum", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 3))
	CRIWARECORE_API void AddSpectralAnalysisDelegate(const UObject* WorldContextObject, const TArray<FAtomBusSpectralAnalysisBandSettings>& InBandSettings, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP, float UpdateRate = 10.f, float DecibelNoiseFloor = -40.f, bool bDoNormalize = true, bool bDoAutoRange = false, float AutoRangeAttackTime = 0.1f, float AutoRangeReleaseTime = 60.f);

	/**
	 *	Remove a spectral analysis delegate.
	 *  @param  OnAtomBusSpectralAnalysisBP		The event delegate to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Spectrum", meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void RemoveSpectralAnalysisDelegate(const UObject* WorldContextObject, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP);

	/** Start spectrum analysis of the audio output. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Analysis", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 1))
	CRIWARECORE_API void StartSpectralAnalysis(const UObject* WorldContextObject, EAtomFFTSize FFTSize = EAtomFFTSize::DefaultSize, EAtomFFTPeakInterpolationMethod InterpolationMethod = EAtomFFTPeakInterpolationMethod::Linear, EAtomFFTWindowType WindowType = EAtomFFTWindowType::Hann, float HopSize = 0, EAtomSpectrumType SpectrumType = EAtomSpectrumType::MagnitudeSpectrum);

	CRIWARECORE_API void StartSpectralAnalysis(FAtomRuntime* InAtomRuntime, EAtomFFTSize FFTSize = EAtomFFTSize::DefaultSize, EAtomFFTPeakInterpolationMethod InterpolationMethod = EAtomFFTPeakInterpolationMethod::Linear, EAtomFFTWindowType WindowType = EAtomFFTWindowType::Hann, float HopSize = 0, EAtomSpectrumType SpectrumType = EAtomSpectrumType::MagnitudeSpectrum);

	/** Stop spectrum analysis of the audio output. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Analysis", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 1))
	CRIWARECORE_API void StopSpectralAnalysis(const UObject* WorldContextObject);

	CRIWARECORE_API void StopSpectralAnalysis(FAtomRuntime* InAtomRuntime);

	/** Sets the output volume of the bus in linear gain. This dynamic volume acts as a multiplier on the OutputVolume property of this bus.  */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer", meta = (WorldContext = "WorldContextObject", DisplayName = "SetBusOutputVolume (linear gain)"))
	CRIWARECORE_API void SetBusOutputVolume(const UObject* WorldContextObject, float InOutputVolume);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer", meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API float GetBusOutputVolume(const UObject* WorldContextObject) const;

	/** Sets the output volume of the bus in linear gain. This dynamic level acts as a multiplier on the WetLevel property of this bus.  */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer", meta = (WorldContext = "WorldContextObject", DisplayName = "SetBusWetLevel (linear gain)"))
	CRIWARECORE_API void SetBusWetLevel(const UObject* WorldContextObject, float InWetLevel);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API float GetBusWetLevel() const;

	/** Sets the output volume of the bus in linear gain. This dynamic level acts as a multiplier on the DryLevel property of this bus.  */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer", meta = (WorldContext = "WorldContextObject", DisplayName = "SetBusDryLevel (linear gain)"))
	CRIWARECORE_API void SetBusDryLevel(const UObject* WorldContextObject, float InDryLevel);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API float GetBusDryLevel() const;

	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Level Mixer")
	CRIWARECORE_API void SetPanning(const FAtomPanning& PanningSetting);

	UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "Atom|Level Mixer")
	CRIWARECORE_API FAtomPanning GetPanning() const;

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API void SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API float GetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API void ResetChannelLevelMatrix();

	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API void SetSendLevel(int SendIndex, float Level);

	UFUNCTION(BlueprintCallable, Category = "Atom|Level Mixer")
	CRIWARECORE_API float GetSendLevel(int SendIndex);

	/** Sets preset to corresponding bus effect, overriding previous preset if present and retrun false when effect type is not valid with this bus. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API bool SetEffectPreset(UAtomBusEffectPreset* InEffectPreset);

	/** Removes preset to corresponding bus effect, default bus effect settings will be restored. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API bool RemoveEffectPreset(UAtomBusEffectPreset* InEffectPreset);

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API bool SetDefaultEffectPreset(int32 InEffectIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API UAtomBusEffectPreset* GetEffectPreset(int32 InEffectIndex)
	{
		return BusEffectChain.IsValidIndex(InEffectIndex) ? BusEffectChain[InEffectIndex] : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API int32 GetNumEffectPresets() { return BusEffectChain.Num(); }

	/** Finds the given preset if it is actualy used in this bus. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects")
	CRIWARECORE_API int32 FindEffectPreset(UAtomBusEffectPreset* InEffectPreset) const;

	/** Gets the Parent holder rack of this bus. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Bus")
	CRIWARECORE_API UAtomRackBase* GetRack() const;

	/** Whether this bus is a main bus. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Bus")
	CRIWARECORE_API bool IsMainBus() const { return BusIndex == 0; }

	/** Gets the bus name identifier. */
	FORCEINLINE const FString& GetBusName() const { return BusName; }

	/** Reset this bus to default values, uses DspSetting if available (Editor). */
	CRIWARECORE_API void Reset(bool bApplyToAtom = true);

	static CRIWARECORE_API FAtomSoundSpectrumAnalyzerSettings GetSpectrumAnalyzerSettings(EAtomFFTSize FFTSize, EAtomFFTPeakInterpolationMethod InterpolationMethod, EAtomFFTWindowType WindowType, float HopSize, EAtomSpectrumType SpectrumType);

	static CRIWARECORE_API FAtomSoundSpectrumAnalyzerDelegateSettings GetSpectrumAnalysisDelegateSettings(const TArray<FAtomBusSpectralAnalysisBandSettings>& InBandSettings, float UpdateRate, float DecibelNoiseFloor, bool bDoNormalize, bool bDoAutoRange, float AutoRangeAttackTime, float AutoRangeReleaseTime);

protected:

	// Custom settors for Modulation Destinations
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetOutputVolumeModulation(const FAtomSoundModulationSettings& InVolMod);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetWetVolumeModulation(const FAtomSoundModulationSettings& InVolMod);

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetDryVolumeModulation(const FAtomSoundModulationSettings& InVolMod);

	// Send bus modulation changes to runtime
	void PushModulationChanges();

	/** Name of this bus.*/
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Bus")
	FString BusName;

	/** The panning settings for this bus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetPanning, BlueprintGetter = GetPanning, DisplayName = "Spatial Panning", Category = "Level Mixer", meta = (DisplayAfter=SpatializationType, EditCondition="SpatializationType == EAtomSpatializationType::Panning"))
	FAtomPanning Panning;

	/** The presets classes used by this bus. (Hidden by Customization.) */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Effects");
	TArray<TSubclassOf<UAtomBusEffectPreset>> BusEffectChainClasses;

	/** The presets to apply to bus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, meta = (EditFixedOrder), Category = "Effects")
	TArray<TObjectPtr<UAtomBusEffectPreset>> BusEffectChain;

protected:

	/** Sync the native bus with current volume. */
	void ApplyVolume(const UObject* WorldContextObject, float InOutputVolume) const;

	/** Sync the native bus with current spatialization panning settings. */
	void ApplyPanning() const;

	/** Sync the native bus with current level matrix. */
	void ApplyChannelLevelMatrix() const;

	/** Sync the native bus with given bus send level. */
	void ApplySendLevel(const FAtomBusSend& InSend) const;

	/** Sync the native bus effect parameter with given value. */
	void ApplyEffectParameter(const UAtomBusEffectPreset* EffectPreset, int ParameterIndex) const; // todo: lookup by effect name

	/** Sync the native bus effect bypass with given value. */
	void ApplyEffectBypass(const UAtomBusEffectPreset* EffectPreset) const; // todo: lookup by effect name

	/** Sync the native bus effect with all preset settings (parameters and bypass). */
	void ApplyEffectPreset(const UAtomBusEffectPreset* EffectPreset) const; // todo: lookup by effect name

	/** Sync the native bus output type with given endpoint settings. */
	void ApplyEndpoint(const UAtomEndpointBase* Endpoint) const;

	/** Sync the native bus with all current settings from this AtomBus. */
	void ApplyAll() const;

	/** Sync this AtomBus with all possible settings from the native bus. */
	void SyncAll();

	/** Reset sends to default values. */
	void ResetSends();

	/** Reset Effect chain to defaults. */
	void ResetEffectChain(bool bUseRuntimeDefaults = false);

	/** Add an effect slot based on its name. */
	int AddEffect(const FName& InEffectName, bool bIsBypassed, bool bUseRuntimeValues);

#if WITH_EDITORONLY_DATA
	/** Setup effect preset settings from AtomConfig bus effect values. */
	void SetEditorEffectSettings(UAtomBusEffectPreset* EffectPreset);

	/** Set editor mixer2 endpoint. */
	void ApplyEditorEndpoint() const;
#endif

	friend class UAtomRackWithParentBase;
	friend class UAtomRack;
	friend class UAtomBusEffectPreset;

private:

	int BusIndex = INDEX_NONE;

	/*
	float CurrentOutputVolume;
	float TargetOutputVolume;
	Atom::FModulationDestination VolumeMod;
	float VolumeModBaseDb = 0.f;
	// modifiers set from BP code
	float VolumeModifier = 1.f;
	*/

	//void ProcessAudio(FAtomRuntime* AtomRuntime);
	FORCEINLINE int GetBusIndex() const { return BusIndex; }

	friend class FAtomRuntime;
	friend class Atom::FMixerSubmix;
	friend class Atom::FAtomExPlayback;
};
