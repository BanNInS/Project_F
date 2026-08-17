
#pragma once

#include "SAudioSpectrogram.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomAudioSpectrumAnalyzer.h"
#include "AtomAudioAnalyzerRack.h"

#include "AtomAudioSpectrogram.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

namespace AtomWidgets
{
	/**
	 * Constructor parameters for the analyzer.
	 */
	struct FAudioSpectrogramParams
	{
		int32 NumChannels = 1;
		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;
		TObjectPtr<UAtomAudioBus> ExternalAudioBus = nullptr;

		TAttribute<EAtomAudioSpectrumAnalyzerType> AnalyzerType = EAtomAudioSpectrumAnalyzerType::FFT;
		TAttribute<EAtomFFTSize> FFTAnalyzerFFTSize = EAtomFFTSize::Max;
		TAttribute<EAtomConstantQFFTSizeEnum> CQTAnalyzerFFTSize = EAtomConstantQFFTSizeEnum::XXLarge;
		TAttribute<EAudioSpectrogramFrequencyAxisPixelBucketMode> FrequencyAxisPixelBucketMode = EAudioSpectrogramFrequencyAxisPixelBucketMode::Average;
		TAttribute<EAudioSpectrogramFrequencyAxisScale> FrequencyAxisScale = EAudioSpectrogramFrequencyAxisScale::Logarithmic;
		TAttribute<EAudioColorGradient> ColorMap = EAudioColorGradient::BlackToWhite;
		TAttribute<EOrientation> Orientation = EOrientation::Orient_Horizontal;

		FOnAtomAnalyzerTypeMenuEntryClicked OnAnalyzerTypeMenuEntryClicked;
		FOnAtomFFTAnalyzerFFTSizeMenuEntryClicked OnFFTAnalyzerFFTSizeMenuEntryClicked;
		FOnAtomCQTAnalyzerFFTSizeMenuEntryClicked OnCQTAnalyzerFFTSizeMenuEntryClicked;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		FOnSpectrogramFrequencyAxisPixelBucketModeMenuEntryClicked OnFrequencyAxisPixelBucketModeMenuEntryClicked;
		FOnSpectrogramFrequencyAxisScaleMenuEntryClicked OnFrequencyAxisScaleMenuEntryClicked;
		FOnSpectrogramColorMapMenuEntryClicked OnColorMapMenuEntryClicked;
		FOnSpectrogramOrientationMenuEntryClicked OnOrientationMenuEntryClicked;
#endif
	};

	/**
	 * Owns an analyzer and a corresponding Slate widget for displaying the resulting spectra.
	 * Can either create an Audio Bus to analyze, or analyze the given Bus.
	 */
	class FAudioSpectrogram : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;
		
		CRI_API FAudioSpectrogram(const FAudioSpectrogramParams& Params);
		CRI_API ~FAudioSpectrogram();

		CRI_API UAtomAudioBus* GetAudioBus() const;

		CRI_API TSharedRef<SWidget> GetWidget() const;

		CRI_API void Init(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr);

		// Begin IAudioAnalyzerRackUnit overrides.
		virtual void SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo) override
		{
			Init(AudioBusInfo.AudioBus->GetNumChannels(), AudioBusInfo.AtomRuntimeID, AudioBusInfo.AudioBus);
		}

		CRI_API virtual TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args) const override;
		// End IAudioAnalyzerRackUnit overrides.

	protected:
		CRI_API void StartAnalyzing(const EAtomAudioSpectrumAnalyzerType InAnalyzerType);
		CRI_API void StopAnalyzing();

		CRI_API void OnSpectrumResults(UAtomSpectrumAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomSpectrumResults>& InSpectrumResultsArray);
		CRI_API void OnConstantQResults(UAtomConstantQAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomConstantQResults>& InSpectrumResultsArray);

		CRI_API void ExtendSpectrumPlotContextMenu(FMenuBuilder& MenuBuilder);
		CRI_API void BuildAnalyzerTypeSubMenu(FMenuBuilder& SubMenu);
		CRI_API void BuildFFTSizeSubMenu(FMenuBuilder& SubMenu);

		CRI_API EActiveTimerReturnType Update(double InCurrentTime, float InDeltaTime);

	private:
		static CRI_API TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params);

		CRI_API void CreateAtomSpectrumAnalyzer();
		CRI_API void ReleaseAtomSpectrumAnalyzer();

		CRI_API void CreateConstantQAnalyzer();
		CRI_API void ReleaseConstantQAnalyzer();

		CRI_API void Teardown();

		/** Audio analyzer objects. */
		TStrongObjectPtr<UAtomSpectrumAnalyzer> SpectrumAnalyzer;
		TStrongObjectPtr<UAtomConstantQAnalyzer> ConstantQAnalyzer;

		/** The audio bus used for analysis. */
		TStrongObjectPtr<UAtomAudioBus> AudioBus;

		/** Handles for results delegate for analyzers. */
		FDelegateHandle SpectrumResultsDelegateHandle;
		FDelegateHandle ConstantQResultsDelegateHandle;

		/** Analyzer settings. */
		TStrongObjectPtr<UAtomSpectrumAnalysisSettings> SpectrumAnalysisSettings;
		TStrongObjectPtr<UAtomConstantQSettings> ConstantQSettings;

		/** Slate widget for spectrum display */
		TSharedPtr<SAudioSpectrogram> Widget;
		TSharedPtr<const FExtensionBase> ContextMenuExtension;
		TSharedPtr<FActiveTimerHandle> ActiveTimer;

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;
		bool bUseExternalAudioBus = false;

		TOptional<EAtomAudioSpectrumAnalyzerType> ActiveAnalyzerType;
		TAttribute<EAtomAudioSpectrumAnalyzerType> AnalyzerType;
		TAttribute<EAtomFFTSize> FFTAnalyzerFFTSize;
		TAttribute<EAtomConstantQFFTSizeEnum> CQTAnalyzerFFTSize;

		FOnAtomAnalyzerTypeMenuEntryClicked OnAnalyzerTypeMenuEntryClicked;
		FOnAtomFFTAnalyzerFFTSizeMenuEntryClicked OnFFTAnalyzerFFTSizeMenuEntryClicked;
		FOnAtomCQTAnalyzerFFTSizeMenuEntryClicked OnCQTAnalyzerFFTSizeMenuEntryClicked;
	};
} // namespace

USTRUCT()
struct FAtomSpectrogramRackUnitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category = Spectrogram)
	EAtomAudioSpectrumAnalyzerType AnalyzerType = EAtomAudioSpectrumAnalyzerType::FFT;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram, meta = (DisplayName = "FFT Size (FFT Analyzer)"))
	EAtomFFTSize FFTAnalyzerFFTSize = EAtomFFTSize::Max;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram, meta = (DisplayName = "FFT Size (CQT Analyzer)"))
	EAtomConstantQFFTSizeEnum CQTAnalyzerFFTSize = EAtomConstantQFFTSizeEnum::XXLarge;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram)
	EAudioSpectrogramFrequencyAxisPixelBucketMode PixelPlotMode = EAudioSpectrogramFrequencyAxisPixelBucketMode::Average;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram)
	EAudioSpectrogramFrequencyAxisScale FrequencyScale = EAudioSpectrogramFrequencyAxisScale::Logarithmic;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram)
	EAudioColorGradient ColorMap = EAudioColorGradient::BlackToWhite;

	UPROPERTY(EditAnywhere, config, Category = Spectrogram)
	TEnumAsByte<EOrientation> Orientation = EOrientation::Orient_Horizontal;
};

#undef CRI_API
