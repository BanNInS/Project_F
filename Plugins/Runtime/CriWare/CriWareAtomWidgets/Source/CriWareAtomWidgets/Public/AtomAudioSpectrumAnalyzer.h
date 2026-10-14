
#pragma once

#include "UObject/StrongObjectPtr.h"
#include "SAudioSpectrumPlot.h"

#include "Atom/AtomAudioBus.h"
#include "AtomAudioAnalyzerRack.h"
#include "Analyzers/AtomSpectrumAnalysis.h"
#include "Analyzers/AtomConstantQ.h"

#include "AtomAudioSpectrumAnalyzer.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class UWorld;

UENUM(BlueprintType)
enum class EAtomAudioSpectrumAnalyzerBallistics : uint8
{
	Analog,
	Digital,
};

UENUM(BlueprintType)
enum class EAtomAudioSpectrumAnalyzerType : uint8
{
	FFT UMETA(ToolTip = "Fast Fourier Transform"),
	CQT UMETA(ToolTip = "Constant-Q Transform"),
};

DECLARE_DELEGATE_OneParam(FOnAtomBallisticsMenuEntryClicked, EAtomAudioSpectrumAnalyzerBallistics);
DECLARE_DELEGATE_OneParam(FOnAtomAnalyzerTypeMenuEntryClicked, EAtomAudioSpectrumAnalyzerType);
DECLARE_DELEGATE_OneParam(FOnAtomFFTAnalyzerFFTSizeMenuEntryClicked, EAtomFFTSize);
DECLARE_DELEGATE_OneParam(FOnAtomCQTAnalyzerFFTSizeMenuEntryClicked, EAtomConstantQFFTSizeEnum);

namespace AtomWidgets
{
	/**
	 * Constructor parameters for the analyzer.
	 */
	struct FAudioSpectrumAnalyzerParams
	{
		int32 NumChannels = 1;
		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;
		TObjectPtr<UAtomAudioBus> ExternalAudioBus = nullptr;

		TAttribute<EAtomAudioSpectrumAnalyzerBallistics> Ballistics = EAtomAudioSpectrumAnalyzerBallistics::Digital;
		TAttribute<EAtomAudioSpectrumAnalyzerType> AnalyzerType = EAtomAudioSpectrumAnalyzerType::CQT;
		TAttribute<EAtomFFTSize> FFTAnalyzerFFTSize = EAtomFFTSize::Max;
		TAttribute<EAtomConstantQFFTSizeEnum> CQTAnalyzerFFTSize = EAtomConstantQFFTSizeEnum::XXLarge;
		TAttribute<float> TiltExponent = 0.0f;
		TAttribute<EAudioSpectrumPlotFrequencyAxisPixelBucketMode> FrequencyAxisPixelBucketMode = EAudioSpectrumPlotFrequencyAxisPixelBucketMode::Average;
		TAttribute<EAudioSpectrumPlotFrequencyAxisScale> FrequencyAxisScale = EAudioSpectrumPlotFrequencyAxisScale::Logarithmic;
		TAttribute<bool> bDisplayFrequencyAxisLabels = false;
		TAttribute<bool> bDisplaySoundLevelAxisLabels = false;
		
		FOnAtomBallisticsMenuEntryClicked OnBallisticsMenuEntryClicked;
		FOnAtomAnalyzerTypeMenuEntryClicked OnAnalyzerTypeMenuEntryClicked;
		FOnAtomFFTAnalyzerFFTSizeMenuEntryClicked OnFFTAnalyzerFFTSizeMenuEntryClicked;
		FOnAtomCQTAnalyzerFFTSizeMenuEntryClicked OnCQTAnalyzerFFTSizeMenuEntryClicked;
		FOnTiltSpectrumMenuEntryClicked OnTiltSpectrumMenuEntryClicked;
		FOnFrequencyAxisPixelBucketModeMenuEntryClicked OnFrequencyAxisPixelBucketModeMenuEntryClicked;
		FOnFrequencyAxisScaleMenuEntryClicked OnFrequencyAxisScaleMenuEntryClicked;
		FOnDisplayAxisLabelsButtonToggled OnDisplayFrequencyAxisLabelsButtonToggled;
		FOnDisplayAxisLabelsButtonToggled OnDisplaySoundLevelAxisLabelsButtonToggled;

		const FAudioSpectrumPlotStyle* PlotStyle = nullptr;
	};

	/**
	 * Owns an analyzer and a corresponding Slate widget for displaying the resulting spectrum.
	 * Exponential time-smoothing is applied to the spectrum.
	 * Can either create an Audio Bus to analyze, or analyze the given Bus.
	 */
	class FAudioSpectrumAnalyzer : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;

		CRI_API FAudioSpectrumAnalyzer(const FAudioSpectrumAnalyzerParams& Params);
		CRI_API FAudioSpectrumAnalyzer(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr);
		CRI_API ~FAudioSpectrumAnalyzer();

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
		CRI_API void UpdateARSmoothing(const float TimeStamp, TConstArrayView<float> SquaredMagnitudes);

		CRI_API FAudioPowerSpectrumData GetAudioSpectrumData();

		CRI_API void ExtendSpectrumPlotContextMenu(FMenuBuilder& MenuBuilder);
		CRI_API void BuildBallisticsSubMenu(FMenuBuilder& SubMenu);
		CRI_API void BuildAnalyzerTypeSubMenu(FMenuBuilder& SubMenu);
		CRI_API void BuildFFTSizeSubMenu(FMenuBuilder& SubMenu);

		CRI_API void UpdateAnalyzerSettings();

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

		/** Meaning of spectrum data. */
		TArray<float> CenterFrequencies;

		/** Cached spectrum data, with AR smoothing applied. */
		TArray<float> ARSmoothedSquaredMagnitudes;

		/** Handles for results delegate for analyzers. */
		FDelegateHandle SpectrumResultsDelegateHandle;
		FDelegateHandle ConstantQResultsDelegateHandle;

		/** Analyzer settings. */
		TStrongObjectPtr<UAtomSpectrumAnalysisSettings> SpectrumAnalysisSettings;
		TStrongObjectPtr<UAtomConstantQSettings> ConstantQSettings;

		/** Slate widget for spectrum display */
		TSharedPtr<SAudioSpectrumPlot> Widget;
		TSharedPtr<const FExtensionBase> ContextMenuExtension;

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;
		bool bUseExternalAudioBus = false;

		TOptional<EAtomAudioSpectrumAnalyzerType> ActiveAnalyzerType;
		TOptional<float> PrevTimeStamp;
		float WindowCompensationPowerGain = 1.0f;
		float AttackTimeMsec = 300.0f;
		float ReleaseTimeMsec = 300.0f;
		TAttribute<EAtomAudioSpectrumAnalyzerBallistics> Ballistics;
		TAttribute<EAtomAudioSpectrumAnalyzerType> AnalyzerType;
		TAttribute<EAtomFFTSize> FFTAnalyzerFFTSize;
		TAttribute<EAtomConstantQFFTSizeEnum> CQTAnalyzerFFTSize;

		FOnAtomBallisticsMenuEntryClicked OnBallisticsMenuEntryClicked;
		FOnAtomAnalyzerTypeMenuEntryClicked OnAnalyzerTypeMenuEntryClicked;
		FOnAtomFFTAnalyzerFFTSizeMenuEntryClicked OnFFTAnalyzerFFTSizeMenuEntryClicked;
		FOnAtomCQTAnalyzerFFTSizeMenuEntryClicked OnCQTAnalyzerFFTSizeMenuEntryClicked;
	};
} // namespace

USTRUCT()
struct FAtomSpectrumAnalyzerRackUnitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	EAtomAudioSpectrumAnalyzerBallistics Ballistics = EAtomAudioSpectrumAnalyzerBallistics::Digital;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	EAtomAudioSpectrumAnalyzerType AnalyzerType = EAtomAudioSpectrumAnalyzerType::CQT;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer, meta = (DisplayName = "FFT Size (FFT Analyzer)"))
	EAtomFFTSize FFTAnalyzerFFTSize = EAtomFFTSize::Max;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer, meta = (DisplayName = "FFT Size (CQT Analyzer)"))
	EAtomConstantQFFTSizeEnum CQTAnalyzerFFTSize = EAtomConstantQFFTSizeEnum::XXLarge;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	EAudioSpectrumPlotTilt TiltSpectrum = EAudioSpectrumPlotTilt::NoTilt;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	EAudioSpectrumPlotFrequencyAxisPixelBucketMode PixelPlotMode = EAudioSpectrumPlotFrequencyAxisPixelBucketMode::Average;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	EAudioSpectrumPlotFrequencyAxisScale FrequencyScale = EAudioSpectrumPlotFrequencyAxisScale::Logarithmic;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	bool bDisplayFrequencyAxisLabels = false;

	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer)
	bool bDisplaySoundLevelAxisLabels = false;
};

#undef CRI_API
