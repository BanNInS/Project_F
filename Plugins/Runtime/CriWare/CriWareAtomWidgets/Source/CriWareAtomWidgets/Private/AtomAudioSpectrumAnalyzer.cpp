
#include "AtomAudioSpectrumAnalyzer.h"

#include "ConstantQFactory.h"
#include "DSP/EnvelopeFollower.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"
#include "AudioWidgetsStyle.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#include "Analyzers/AtomSpectrumAnalysisFactory.h"
#include "Analyzers/AtomConstantQFactory.h"

#define LOCTEXT_NAMESPACE "AtomWidgets::FAudioSpectrumAnalyzer"

namespace AtomWidgets
{
	namespace AudioSpectrumAnalyzerPrivate
	{
		inline float GetWindowCompensationPowerGain(Audio::EWindowType WindowType, int32 FFTSize)
		{
			ensure(FFTSize <= 16384);
			ensure(FMath::IsPowerOfTwo(FFTSize));

			using namespace Audio;

			// Create a temporary buffer and initialize it to +1 DC:
			FAlignedFloatBuffer Samples;
			Samples.SetNumUninitialized(FFTSize);
			Audio::ArraySetToConstantInplace(Samples, 1.0f);

			// Initialize the window in the same manner as FAtomSpectrumAnalyzer and FConstantQAnalyzer:
			FWindow Window(WindowType, FFTSize, /*NumChannels=*/1, /*bIsPeriodic=*/false);

			// Apply window to DC signal:
			Window.ApplyToBuffer(Samples.GetData());

			// Calculate the mean square of the windowed signal:
			float WindowedDCMeanSquare = 1.0f;
			Audio::ArrayMeanSquared(Samples, WindowedDCMeanSquare);

			// Return the power gain required to reverse the effect of the windowing process on the RMS of DC:
			constexpr float DCMeanSquare = 1.0f;
			return DCMeanSquare / WindowedDCMeanSquare;
		}

		/**
		 * Light wrapper for accessing settings for the analyzer rack unit. Can be passed by value.
		 */
		class FRackUnitSettingsHelper
		{
		public:
			FRackUnitSettingsHelper(const FProperty& InSettingsProperty)
				: SettingsProperty(InSettingsProperty)
			{
				//
			}

			FAtomSpectrumAnalyzerRackUnitSettings* GetRackUnitSettings() const
			{
				UObject* EditorSettingsObject = GetEditorSettingsObject();
				return SettingsProperty.ContainerPtrToValuePtr<FAtomSpectrumAnalyzerRackUnitSettings>(EditorSettingsObject);
			}

			void SaveConfig() const
			{
				GetEditorSettingsObject()->SaveConfig();
			}

		private:
			UObject* GetEditorSettingsObject() const
			{
				return SettingsProperty.GetOwnerClass()->GetDefaultObject();
			}

			const FProperty& SettingsProperty;
		};
	}

	const FAudioAnalyzerRackUnitTypeInfo FAudioSpectrumAnalyzer::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAudioSpectrumAnalyzer"),
		.DisplayName = LOCTEXT("AudioSpectrumAnalyzerDisplayName", "Spectrum Analyzer"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.25f,
	};

	FAudioSpectrumAnalyzer::FAudioSpectrumAnalyzer(const FAudioSpectrumAnalyzerParams& Params)
		: SpectrumAnalysisSettings(NewObject<UAtomSpectrumAnalysisSettings>())
		, ConstantQSettings(NewObject<UAtomConstantQSettings>())
		, Widget(SNew(SAudioSpectrumPlot)
			.Style(Params.PlotStyle ? Params.PlotStyle : &FAudioWidgetsStyle::Get().GetWidgetStyle<FAudioSpectrumPlotStyle>("AudioSpectrumPlot.Style"))
			.Clipping(EWidgetClipping::ClipToBounds)
			.TiltExponent(Params.TiltExponent)
			.DisplayCrosshair(true)
			.DisplayFrequencyAxisLabels(Params.bDisplayFrequencyAxisLabels)
			.DisplaySoundLevelAxisLabels(Params.bDisplaySoundLevelAxisLabels)
			.FrequencyAxisScale(Params.FrequencyAxisScale)
			.FrequencyAxisPixelBucketMode(Params.FrequencyAxisPixelBucketMode)
			.OnTiltSpectrumMenuEntryClicked(Params.OnTiltSpectrumMenuEntryClicked)
			.OnFrequencyAxisPixelBucketModeMenuEntryClicked(Params.OnFrequencyAxisPixelBucketModeMenuEntryClicked)
			.OnFrequencyAxisScaleMenuEntryClicked(Params.OnFrequencyAxisScaleMenuEntryClicked)
			.OnDisplayFrequencyAxisLabelsButtonToggled(Params.OnDisplayFrequencyAxisLabelsButtonToggled)
			.OnDisplaySoundLevelAxisLabelsButtonToggled(Params.OnDisplaySoundLevelAxisLabelsButtonToggled)
			.OnGetAudioSpectrumData_Raw(this, &FAudioSpectrumAnalyzer::GetAudioSpectrumData))
		, Ballistics(Params.Ballistics)
		, AnalyzerType(Params.AnalyzerType)
		, FFTAnalyzerFFTSize(Params.FFTAnalyzerFFTSize)
		, CQTAnalyzerFFTSize(Params.CQTAnalyzerFFTSize)
		, OnBallisticsMenuEntryClicked(Params.OnBallisticsMenuEntryClicked)
		, OnAnalyzerTypeMenuEntryClicked(Params.OnAnalyzerTypeMenuEntryClicked)
		, OnFFTAnalyzerFFTSizeMenuEntryClicked(Params.OnFFTAnalyzerFFTSizeMenuEntryClicked)
		, OnCQTAnalyzerFFTSizeMenuEntryClicked(Params.OnCQTAnalyzerFFTSizeMenuEntryClicked)
	{
		SpectrumAnalysisSettings->SpectrumType = EAtomSpectrumType::PowerSpectrum;
		SpectrumAnalysisSettings->FFTSize = FFTAnalyzerFFTSize.Get();
		SpectrumAnalysisSettings->WindowType = EAtomFFTWindowType::Blackman;
		SpectrumAnalysisSettings->bDownmixToMono = true;

		ConstantQSettings->SpectrumType = EAtomSpectrumType::PowerSpectrum;
		ConstantQSettings->NumBandsPerOctave = 6.0f;
		ConstantQSettings->NumBands = 61;
		ConstantQSettings->StartingFrequencyHz = 20000.0f * FMath::Pow(0.5f, (ConstantQSettings->NumBands - 1) / ConstantQSettings->NumBandsPerOctave);
		ConstantQSettings->FFTSize = CQTAnalyzerFFTSize.Get();
		ConstantQSettings->WindowType = EAtomFFTWindowType::Blackman;
		ConstantQSettings->bDownmixToMono = true;
		ConstantQSettings->BandWidthStretch = 2.0f;

		ContextMenuExtension = Widget->AddContextMenuExtension(EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FAudioSpectrumAnalyzer::ExtendSpectrumPlotContextMenu));

		Init(Params.NumChannels, Params.AtomRuntimeID, Params.ExternalAudioBus);
	}

	FAudioSpectrumAnalyzer::FAudioSpectrumAnalyzer(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus)
		: FAudioSpectrumAnalyzer({ .NumChannels = InNumChannels, .AtomRuntimeID = InAtomRuntimeID, .ExternalAudioBus = InExternalAudioBus })
	{
		// Delegating constructor
	}

	FAudioSpectrumAnalyzer::~FAudioSpectrumAnalyzer()
	{
		Teardown();

		Widget->UnbindOnGetAudioSpectrumData();

		if (ContextMenuExtension.IsValid())
		{
			Widget->RemoveContextMenuExtension(ContextMenuExtension.ToSharedRef());
		}
	}

	UAtomAudioBus* FAudioSpectrumAnalyzer::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	TSharedRef<SWidget> FAudioSpectrumAnalyzer::GetWidget() const
	{
		return Widget->AsShared();
	}

	void FAudioSpectrumAnalyzer::Init(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus)
	{
		Teardown();

		AtomRuntimeID = InAtomRuntimeID;

		// Only create analyzers etc if we have an audio device:
		if (AtomRuntimeID != FAudioBusInfo::InvalidAtomRuntimeID)
		{
			check(InNumChannels > 0);

			bUseExternalAudioBus = InExternalAudioBus != nullptr;
			AudioBus = bUseExternalAudioBus ? TStrongObjectPtr(InExternalAudioBus.Get()) : TStrongObjectPtr(NewObject<UAtomAudioBus>());
			AudioBus->AudioBusChannels = EAtomAudioBusChannels(InNumChannels - 1);

			CreateAtomSpectrumAnalyzer();
			CreateConstantQAnalyzer();

			StartAnalyzing(AnalyzerType.Get());
		}
	}

	void FAudioSpectrumAnalyzer::StartAnalyzing(const EAtomAudioSpectrumAnalyzerType InAnalyzerType)
	{
		ensure(!ActiveAnalyzerType.IsSet());

		switch (InAnalyzerType)
		{
		case EAtomAudioSpectrumAnalyzerType::FFT:
			SpectrumAnalyzer->StartAnalyzing(AtomRuntimeID, AudioBus.Get());
			break;
		case EAtomAudioSpectrumAnalyzerType::CQT:
			ConstantQAnalyzer->StartAnalyzing(AtomRuntimeID, AudioBus.Get());
			break;
		default:
			break;
		}

		ActiveAnalyzerType = InAnalyzerType;
	}

	void FAudioSpectrumAnalyzer::StopAnalyzing()
	{
		ensure(ActiveAnalyzerType.IsSet());

		switch (ActiveAnalyzerType.GetValue())
		{
		case EAtomAudioSpectrumAnalyzerType::FFT:
			SpectrumAnalyzer->StopAnalyzing();
			break;
		case EAtomAudioSpectrumAnalyzerType::CQT:
			ConstantQAnalyzer->StopAnalyzing();
			break;
		default:
			break;
		}

		ActiveAnalyzerType.Reset();
	}

	void FAudioSpectrumAnalyzer::OnSpectrumResults(UAtomSpectrumAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomSpectrumResults>& InSpectrumResultsArray)
	{
		if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::FFT && InSpectrumAnalyzer == SpectrumAnalyzer.Get())
		{
			for (const FAtomSpectrumResults& SpectrumResults : InSpectrumResultsArray)
			{
				if (PrevTimeStamp.IsSet() && SpectrumResults.TimeSeconds > PrevTimeStamp.GetValue())
				{
					UpdateARSmoothing(SpectrumResults.TimeSeconds, SpectrumResults.SpectrumValues);
				}
				else
				{
					// Find samplerate:
					float SampleRate = 48000.0f;
					if (const FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
					{
						if (const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID))
						{
							SampleRate = AtomRuntime->GetRuntimeSampleRate();
						}
					}

					// Init center frequencies:
					CenterFrequencies.SetNumUninitialized(SpectrumAnalyzer->GetNumCenterFrequencies());
					SpectrumAnalyzer->GetCenterFrequencies(SampleRate, CenterFrequencies);

					// Init spectrum data:
					ARSmoothedSquaredMagnitudes = SpectrumResults.SpectrumValues;

					// Update the window compensation power gain:
					TUniquePtr<Atom::IAnalyzerSettings> Settings = SpectrumAnalysisSettings->GetSettings(SampleRate, 1);
					const Atom::FSpectrumAnalysisSettings* ConcreteSettings = static_cast<Atom::FSpectrumAnalysisSettings*>(Settings.Get());
					WindowCompensationPowerGain = AudioSpectrumAnalyzerPrivate::GetWindowCompensationPowerGain(ConcreteSettings->WindowType, ConcreteSettings->FFTSize);

					// Apply window compensation power gain:
					Audio::ArrayMultiplyByConstantInPlace(ARSmoothedSquaredMagnitudes, WindowCompensationPowerGain);
				}

				PrevTimeStamp = SpectrumResults.TimeSeconds;
			}
		}
	}

	void FAudioSpectrumAnalyzer::OnConstantQResults(UAtomConstantQAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomConstantQResults>& InSpectrumResultsArray)
	{
		if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::CQT && InSpectrumAnalyzer == ConstantQAnalyzer.Get())
		{
			for (const FAtomConstantQResults& SpectrumResults : InSpectrumResultsArray)
			{
				if (PrevTimeStamp.IsSet() && SpectrumResults.TimeSeconds > PrevTimeStamp.GetValue())
				{
					UpdateARSmoothing(SpectrumResults.TimeSeconds, SpectrumResults.SpectrumValues);
				}
				else
				{
					// Init center frequencies:
					CenterFrequencies.SetNumUninitialized(ConstantQAnalyzer->GetNumCenterFrequencies());
					ConstantQAnalyzer->GetCenterFrequencies(CenterFrequencies);

					// Init spectrum data:
					ARSmoothedSquaredMagnitudes = SpectrumResults.SpectrumValues;

					// Update the window compensation power gain:
					TUniquePtr<Atom::IAnalyzerSettings> Settings = ConstantQSettings->GetSettings(0.0f, 1);
					const Atom::FConstantQSettings* ConcreteSettings = static_cast<Atom::FConstantQSettings*>(Settings.Get());
					WindowCompensationPowerGain = AudioSpectrumAnalyzerPrivate::GetWindowCompensationPowerGain(ConcreteSettings->WindowType, ConcreteSettings->FFTSize);

					// Apply window compensation power gain:
					Audio::ArrayMultiplyByConstantInPlace(ARSmoothedSquaredMagnitudes, WindowCompensationPowerGain);
				}

				PrevTimeStamp = SpectrumResults.TimeSeconds;
			}
		}
	}

	void FAudioSpectrumAnalyzer::UpdateARSmoothing(const float TimeStamp, TConstArrayView<float> SquaredMagnitudes)
	{
		// Calculate AR smoother coefficients:
		const float DeltaT = (TimeStamp - PrevTimeStamp.GetValue());
		const bool bIsAnalogAttackRelease = (Ballistics.Get() == EAtomAudioSpectrumAnalyzerBallistics::Analog);
		Audio::FAttackRelease AttackRelease(1.0f / DeltaT, AttackTimeMsec, ReleaseTimeMsec, bIsAnalogAttackRelease);

		// Apply AR smoothing for each frequency:
		check(SquaredMagnitudes.Num() == ARSmoothedSquaredMagnitudes.Num());
		for (int Index = 0; Index < SquaredMagnitudes.Num(); Index++)
		{
			const float OldValue = ARSmoothedSquaredMagnitudes[Index];
			const float NewValue = WindowCompensationPowerGain * SquaredMagnitudes[Index];
			const float ARSmootherCoefficient = (NewValue >= OldValue) ? AttackRelease.GetAttackTimeSamples() : AttackRelease.GetReleaseTimeSamples();
			ARSmoothedSquaredMagnitudes[Index] = FMath::Lerp(NewValue, OldValue, ARSmootherCoefficient);
		}
	}

	void FAudioSpectrumAnalyzer::Teardown()
	{
		if (SpectrumAnalyzer.IsValid() && SpectrumAnalyzer->IsValidLowLevel())
		{
			if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::FFT)
			{
				SpectrumAnalyzer->StopAnalyzing();
			}

			ReleaseAtomSpectrumAnalyzer();
		}

		if (ConstantQAnalyzer.IsValid() && ConstantQAnalyzer->IsValidLowLevel())
		{
			if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::CQT)
			{
				ConstantQAnalyzer->StopAnalyzing();
			}

			ReleaseConstantQAnalyzer();
		}

		ActiveAnalyzerType.Reset();
		PrevTimeStamp.Reset();
		CenterFrequencies.Empty();
		ARSmoothedSquaredMagnitudes.Empty();

		AudioBus.Reset();
		bUseExternalAudioBus = false;
	}

	FAudioPowerSpectrumData FAudioSpectrumAnalyzer::GetAudioSpectrumData()
	{
		// The SAudioSpectrumPlot regularly polls us for audio spectrum data.
		// We can update the analyzer settings here:
		UpdateAnalyzerSettings();

		check(CenterFrequencies.Num() == ARSmoothedSquaredMagnitudes.Num());
		return FAudioPowerSpectrumData{ CenterFrequencies, ARSmoothedSquaredMagnitudes };
	}

	void FAudioSpectrumAnalyzer::ExtendSpectrumPlotContextMenu(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("AnalyzerSettings", LOCTEXT("AnalyzerSettings", "Analyzer Settings"));
		MenuBuilder.AddSubMenu(
			LOCTEXT("Ballistics", "Ballistics"),
			FText(),
			FNewMenuDelegate::CreateSP(this, &FAudioSpectrumAnalyzer::BuildBallisticsSubMenu));
		MenuBuilder.AddSubMenu(
			LOCTEXT("AnalyzerType", "Analyzer Type"),
			FText(),
			FNewMenuDelegate::CreateSP(this, &FAudioSpectrumAnalyzer::BuildAnalyzerTypeSubMenu));
		MenuBuilder.AddSubMenu(
			LOCTEXT("FFTSize", "FFT Size"),
			FText(),
			FNewMenuDelegate::CreateSP(this, &FAudioSpectrumAnalyzer::BuildFFTSizeSubMenu));
		MenuBuilder.EndSection();
	}

	void FAudioSpectrumAnalyzer::BuildBallisticsSubMenu(FMenuBuilder& SubMenu)
	{
		const UEnum* EnumClass = StaticEnum<EAtomAudioSpectrumAnalyzerBallistics>();
		const int32 NumEnumValues = EnumClass->NumEnums() - 1; // Exclude 'MAX' enum value.
		for (int32 Index = 0; Index < NumEnumValues; Index++)
		{
			const auto EnumValue = static_cast<EAtomAudioSpectrumAnalyzerBallistics>(EnumClass->GetValueByIndex(Index));

			SubMenu.AddMenuEntry(
				EnumClass->GetDisplayNameTextByIndex(Index),
#if WITH_EDITOR
				EnumClass->GetToolTipTextByIndex(Index),
#else
				FText(),
#endif
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSPLambda(this, [this, EnumValue]()
						{
							if (!Ballistics.IsBound())
							{
								Ballistics = EnumValue;
							}

							OnBallisticsMenuEntryClicked.ExecuteIfBound(EnumValue);
						}),
					FCanExecuteAction(),
					FIsActionChecked::CreateSPLambda(this, [this, EnumValue]() { return (Ballistics.Get() == EnumValue); })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton);
		}
	}

	void FAudioSpectrumAnalyzer::BuildAnalyzerTypeSubMenu(FMenuBuilder& SubMenu)
	{
		const UEnum* EnumClass = StaticEnum<EAtomAudioSpectrumAnalyzerType>();
		const int32 NumEnumValues = EnumClass->NumEnums() - 1; // Exclude 'MAX' enum value.
		for (int32 Index = 0; Index < NumEnumValues; Index++)
		{
			const auto EnumValue = static_cast<EAtomAudioSpectrumAnalyzerType>(EnumClass->GetValueByIndex(Index));

			SubMenu.AddMenuEntry(
				EnumClass->GetDisplayNameTextByIndex(Index),
#if WITH_EDITOR
				EnumClass->GetToolTipTextByIndex(Index),
#else
				FText(),
#endif
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSPLambda(this, [this, EnumValue]()
						{
							if (!AnalyzerType.IsBound())
							{
								AnalyzerType = EnumValue;
							}

							OnAnalyzerTypeMenuEntryClicked.ExecuteIfBound(EnumValue);
						}),
					FCanExecuteAction(),
					FIsActionChecked::CreateSPLambda(this, [this, EnumValue]() { return (AnalyzerType.Get() == EnumValue); })
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton);
		}
	}

	void FAudioSpectrumAnalyzer::BuildFFTSizeSubMenu(FMenuBuilder& SubMenu)
	{
		// There is a different FFTSize enum depending on the analyzer type.

		if (AnalyzerType.Get() == EAtomAudioSpectrumAnalyzerType::FFT)
		{
			const UEnum* EnumClass = StaticEnum<EAtomFFTSize>();
			const int32 NumEnumValues = EnumClass->NumEnums() - 1; // Exclude 'MAX' enum value.
			for (int32 Index = 0; Index < NumEnumValues; Index++)
			{
				const auto EnumValue = static_cast<EAtomFFTSize>(EnumClass->GetValueByIndex(Index));
				if (EnumValue == EAtomFFTSize::DefaultSize)
				{
					// Skip the duplicate 512 enum value 'DefaultSize'.
					continue;
				}

				SubMenu.AddMenuEntry(
					EnumClass->GetDisplayNameTextByIndex(Index),
#if WITH_EDITOR
					EnumClass->GetToolTipTextByIndex(Index),
#else
					FText(),
#endif
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSPLambda(this, [this, EnumValue]()
							{
								if (!FFTAnalyzerFFTSize.IsBound())
								{
									FFTAnalyzerFFTSize = EnumValue;
								}

								OnFFTAnalyzerFFTSizeMenuEntryClicked.ExecuteIfBound(EnumValue);
							}),
						FCanExecuteAction(),
						FIsActionChecked::CreateSPLambda(this, [this, EnumValue]() { return (FFTAnalyzerFFTSize.Get() == EnumValue); })
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);
			}
		}
		else if (AnalyzerType.Get() == EAtomAudioSpectrumAnalyzerType::CQT)
		{
			const UEnum* EnumClass = StaticEnum<EAtomConstantQFFTSizeEnum>();
			const int32 NumEnumValues = EnumClass->NumEnums() - 1; // Exclude 'MAX' enum value.
			for (int32 Index = 0; Index < NumEnumValues; Index++)
			{
				const auto EnumValue = static_cast<EAtomConstantQFFTSizeEnum>(EnumClass->GetValueByIndex(Index));

				SubMenu.AddMenuEntry(
					EnumClass->GetDisplayNameTextByIndex(Index),
#if WITH_EDITOR
					EnumClass->GetToolTipTextByIndex(Index),
#else
					FText(),
#endif
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSPLambda(this, [this, EnumValue]()
							{
								if (!CQTAnalyzerFFTSize.IsBound())
								{
									CQTAnalyzerFFTSize = EnumValue;
								}

								OnCQTAnalyzerFFTSizeMenuEntryClicked.ExecuteIfBound(EnumValue);
							}),
						FCanExecuteAction(),
						FIsActionChecked::CreateSPLambda(this, [this, EnumValue]() { return (CQTAnalyzerFFTSize.Get() == EnumValue); })
					),
					NAME_None,
					EUserInterfaceActionType::ToggleButton);
			}
		}
	}

	void FAudioSpectrumAnalyzer::UpdateAnalyzerSettings()
	{
		if (AtomRuntimeID == FAudioBusInfo::InvalidAtomRuntimeID)
		{
			// No analyzers available if no valid audio device.
			ensure(!ActiveAnalyzerType.IsSet());
			return;
		}

		const EAtomAudioSpectrumAnalyzerType RequiredAnalyzerType = AnalyzerType.Get();
		const EAtomFFTSize FFTAnalyzerRequiredFFTSize = FFTAnalyzerFFTSize.Get();
		const EAtomConstantQFFTSizeEnum CQTAnalyzerRequiredFFTSize = CQTAnalyzerFFTSize.Get();

		const bool bRequiredAnalyzerTypeChanged = (ActiveAnalyzerType != RequiredAnalyzerType);
		const bool bFFTAnalyzerRequiredFFTSizeChanged = (SpectrumAnalysisSettings->FFTSize != FFTAnalyzerRequiredFFTSize);
		const bool bCQTAnalyzerRequiredFFTSizeChanged = (ConstantQSettings->FFTSize != CQTAnalyzerRequiredFFTSize);
		if (bRequiredAnalyzerTypeChanged || bFFTAnalyzerRequiredFFTSizeChanged || bCQTAnalyzerRequiredFFTSizeChanged)
		{
			StopAnalyzing();

			if (bRequiredAnalyzerTypeChanged)
			{
				// There will be different center frequencies when the analyzer type changes:
				PrevTimeStamp.Reset();
				CenterFrequencies.Reset();
				ARSmoothedSquaredMagnitudes.Reset();
			}

			if (bFFTAnalyzerRequiredFFTSizeChanged)
			{
				ReleaseAtomSpectrumAnalyzer();
				SpectrumAnalysisSettings->FFTSize = FFTAnalyzerRequiredFFTSize;
				CreateAtomSpectrumAnalyzer();
			}

			if (bCQTAnalyzerRequiredFFTSizeChanged)
			{
				ReleaseConstantQAnalyzer();
				ConstantQSettings->FFTSize = CQTAnalyzerRequiredFFTSize;
				CreateConstantQAnalyzer();
			}

			StartAnalyzing(RequiredAnalyzerType);
		}
	}

	void FAudioSpectrumAnalyzer::CreateAtomSpectrumAnalyzer()
	{
		ensure(!SpectrumAnalyzer.IsValid());
		ensure(!SpectrumResultsDelegateHandle.IsValid());

		SpectrumAnalyzer = TStrongObjectPtr(NewObject<UAtomSpectrumAnalyzer>());
		SpectrumAnalyzer->Settings = SpectrumAnalysisSettings.Get();
		SpectrumResultsDelegateHandle = SpectrumAnalyzer->OnSpectrumResultsNative.AddRaw(this, &FAudioSpectrumAnalyzer::OnSpectrumResults);
	}

	void FAudioSpectrumAnalyzer::ReleaseAtomSpectrumAnalyzer()
	{
		if (ensure(SpectrumAnalyzer.IsValid() && SpectrumResultsDelegateHandle.IsValid()))
		{
			SpectrumAnalyzer->OnSpectrumResultsNative.Remove(SpectrumResultsDelegateHandle);
		}

		SpectrumResultsDelegateHandle.Reset();
		SpectrumAnalyzer.Reset();
	}

	void FAudioSpectrumAnalyzer::CreateConstantQAnalyzer()
	{
		ConstantQAnalyzer = TStrongObjectPtr(NewObject<UAtomConstantQAnalyzer>());
		ConstantQAnalyzer->Settings = ConstantQSettings.Get();
		ConstantQResultsDelegateHandle = ConstantQAnalyzer->OnConstantQResultsNative.AddRaw(this, &FAudioSpectrumAnalyzer::OnConstantQResults);
	}

	void FAudioSpectrumAnalyzer::ReleaseConstantQAnalyzer()
	{
		if (ensure(ConstantQAnalyzer.IsValid() && ConstantQResultsDelegateHandle.IsValid()))
		{
			ConstantQAnalyzer->OnConstantQResultsNative.Remove(ConstantQResultsDelegateHandle);
		}

		ConstantQResultsDelegateHandle.Reset();
		ConstantQAnalyzer.Reset();
	}

	TSharedRef<SDockTab> FAudioSpectrumAnalyzer::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				GetWidget()
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAudioSpectrumAnalyzer::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		using namespace AudioSpectrumAnalyzerPrivate;

		FAudioSpectrumAnalyzerParams AnalyzerParams;
		AnalyzerParams.NumChannels = Params.AudioBusInfo.GetNumChannels();
		AnalyzerParams.AtomRuntimeID = Params.AudioBusInfo.AtomRuntimeID;
		AnalyzerParams.ExternalAudioBus = Params.AudioBusInfo.AudioBus;

		if (Params.EditorSettingsClass != nullptr)
		{
			// If we have been given a valid editor settings class, bind analyzer options to the settings:
			const FProperty* SpectrumAnalyzerSettingsProperty = Params.EditorSettingsClass->FindPropertyByName("SpectrumAnalyzerSettings");
			if (SpectrumAnalyzerSettingsProperty != nullptr)
			{
				const FRackUnitSettingsHelper SettingsHelper(*SpectrumAnalyzerSettingsProperty);

				AnalyzerParams.Ballistics.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->Ballistics;
					});
				AnalyzerParams.AnalyzerType.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->AnalyzerType;
					});
				AnalyzerParams.FFTAnalyzerFFTSize.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->FFTAnalyzerFFTSize;
					});
				AnalyzerParams.CQTAnalyzerFFTSize.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->CQTAnalyzerFFTSize;
					});
				AnalyzerParams.TiltExponent.BindLambda([=]()
					{
						const EAudioSpectrumPlotTilt TiltSpectrum = SettingsHelper.GetRackUnitSettings()->TiltSpectrum;
						return SAudioSpectrumPlot::GetTiltExponentValue(TiltSpectrum);
					});
				AnalyzerParams.FrequencyAxisPixelBucketMode.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->PixelPlotMode;
					});
				AnalyzerParams.FrequencyAxisScale.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->FrequencyScale;
					});
				AnalyzerParams.bDisplayFrequencyAxisLabels.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->bDisplayFrequencyAxisLabels;
					});
				AnalyzerParams.bDisplaySoundLevelAxisLabels.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->bDisplaySoundLevelAxisLabels;
					});

				AnalyzerParams.OnBallisticsMenuEntryClicked.BindLambda([=](EAtomAudioSpectrumAnalyzerBallistics SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->Ballistics = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnAnalyzerTypeMenuEntryClicked.BindLambda([=](EAtomAudioSpectrumAnalyzerType SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->AnalyzerType = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnFFTAnalyzerFFTSizeMenuEntryClicked.BindLambda([=](EAtomFFTSize SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->FFTAnalyzerFFTSize = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnCQTAnalyzerFFTSizeMenuEntryClicked.BindLambda([=](EAtomConstantQFFTSizeEnum SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->CQTAnalyzerFFTSize = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnTiltSpectrumMenuEntryClicked.BindLambda([=](EAudioSpectrumPlotTilt SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->TiltSpectrum = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnFrequencyAxisPixelBucketModeMenuEntryClicked.BindLambda([=](EAudioSpectrumPlotFrequencyAxisPixelBucketMode SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->PixelPlotMode = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnFrequencyAxisScaleMenuEntryClicked.BindLambda([=](EAudioSpectrumPlotFrequencyAxisScale SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->FrequencyScale = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnDisplayFrequencyAxisLabelsButtonToggled.BindLambda([=]()
					{
						FAtomSpectrumAnalyzerRackUnitSettings* SpectrumAnalyzerSettings = SettingsHelper.GetRackUnitSettings();
						SpectrumAnalyzerSettings->bDisplayFrequencyAxisLabels = !SpectrumAnalyzerSettings->bDisplayFrequencyAxisLabels;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnDisplaySoundLevelAxisLabelsButtonToggled.BindLambda([=]()
					{
						FAtomSpectrumAnalyzerRackUnitSettings* SpectrumAnalyzerSettings = SettingsHelper.GetRackUnitSettings();
						SpectrumAnalyzerSettings->bDisplaySoundLevelAxisLabels = !SpectrumAnalyzerSettings->bDisplaySoundLevelAxisLabels;
						SettingsHelper.SaveConfig();
					});
			}
		}

		AnalyzerParams.PlotStyle = &Params.StyleSet->GetWidgetStyle<FAudioSpectrumPlotStyle>("AudioSpectrumPlot.Style");

		return MakeShared<FAudioSpectrumAnalyzer>(AnalyzerParams);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
