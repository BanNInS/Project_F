
#include "AtomAudioSpectrogram.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Docking/SDockTab.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#define LOCTEXT_NAMESPACE "AtomWidgets::FAudioSpectrogram"

namespace AtomWidgets
{
	namespace AudioSpectrogramPrivate
	{
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

			FAtomSpectrogramRackUnitSettings* GetRackUnitSettings() const
			{
				UObject* EditorSettingsObject = GetEditorSettingsObject();
				return SettingsProperty.ContainerPtrToValuePtr<FAtomSpectrogramRackUnitSettings>(EditorSettingsObject);
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

	const FAudioAnalyzerRackUnitTypeInfo FAudioSpectrogram::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAudioSpectrogram"),
		.DisplayName = LOCTEXT("AudioSpectrogramDisplayName", "Spectrogram"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.25f,
	};

	FAudioSpectrogram::FAudioSpectrogram(const FAudioSpectrogramParams& Params)
		: SpectrumAnalysisSettings(NewObject<UAtomSpectrumAnalysisSettings>())
		, ConstantQSettings(NewObject<UAtomConstantQSettings>())
		, Widget(SNew(SAudioSpectrogram)
			.Clipping(EWidgetClipping::ClipToBounds)
			.FrequencyAxisScale(Params.FrequencyAxisScale)
			.FrequencyAxisPixelBucketMode(Params.FrequencyAxisPixelBucketMode)
			.ColorMap(Params.ColorMap)
			.Orientation(Params.Orientation)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
			.FillBackground(true)
			.OnFrequencyAxisPixelBucketModeMenuEntryClicked(Params.OnFrequencyAxisPixelBucketModeMenuEntryClicked)
			.OnFrequencyAxisScaleMenuEntryClicked(Params.OnFrequencyAxisScaleMenuEntryClicked)
			.OnColorMapMenuEntryClicked(Params.OnColorMapMenuEntryClicked)
			.OnOrientationMenuEntryClicked(Params.OnOrientationMenuEntryClicked)
#endif
		)
		, AnalyzerType(Params.AnalyzerType)
		, FFTAnalyzerFFTSize(Params.FFTAnalyzerFFTSize)
		, CQTAnalyzerFFTSize(Params.CQTAnalyzerFFTSize)
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

		ContextMenuExtension = Widget->AddContextMenuExtension(EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FAudioSpectrogram::ExtendSpectrumPlotContextMenu));

		ActiveTimer = Widget->RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateRaw(this, &FAudioSpectrogram::Update));

		Init(Params.NumChannels, Params.AtomRuntimeID, Params.ExternalAudioBus);
	}

	FAudioSpectrogram::~FAudioSpectrogram()
	{
		Teardown();

		if (ContextMenuExtension.IsValid())
		{
			Widget->RemoveContextMenuExtension(ContextMenuExtension.ToSharedRef());
		}

		if (ActiveTimer.IsValid())
		{
			Widget->UnRegisterActiveTimer(ActiveTimer.ToSharedRef());
		}
	}

	UAtomAudioBus* FAudioSpectrogram::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	TSharedRef<SWidget> FAudioSpectrogram::GetWidget() const
	{
		return Widget->AsShared();
	}

	void FAudioSpectrogram::Init(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus)
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

	void FAudioSpectrogram::StartAnalyzing(const EAtomAudioSpectrumAnalyzerType InAnalyzerType)
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

	void FAudioSpectrogram::StopAnalyzing()
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

namespace
{
	EAudioSpectrumType AtomToAudioSpectrumType(EAtomSpectrumType SpectrumType)
	{
		switch (SpectrumType)
		{
		case EAtomSpectrumType::MagnitudeSpectrum:
			return EAudioSpectrumType::MagnitudeSpectrum;
		case EAtomSpectrumType::Decibel:
			return EAudioSpectrumType::Decibel;
		case EAtomSpectrumType::PowerSpectrum:
		default:
			return EAudioSpectrumType::PowerSpectrum;
		};
	}

	void WidgetAddFrame(TSharedPtr<SAudioSpectrogram> Widget, const FAtomSpectrumResults& SpectrumResults, const EAtomSpectrumType SpectrumType, const float SampleRate)
	{
		const TConstArrayView<float> SpectrumValues(SpectrumResults.SpectrumValues);
		const int32 FFTSize = 2 * (SpectrumValues.Num() - 1);
		const float BinSize = SampleRate / FFTSize;

		// Just ignoring the Nyquist sample here to get us a power of two length:
		ensure(FMath::IsPowerOfTwo(SpectrumValues.Num() - 1));
		const TConstArrayView<float> SpectrumValuesNoNyquist = SpectrumValues.LeftChop(1);

		const FAudioSpectrogramFrameData SpectrogramFrameData
		{
			.SpectrumValues = SpectrumValuesNoNyquist,
			.SpectrumType = AtomToAudioSpectrumType(SpectrumType),
			.MinFrequency = 0.0f,
			.MaxFrequency = (SpectrumValuesNoNyquist.Num() - 1) * BinSize,
			.bLogSpacedFreqencies = false,
		};

		Widget->AddFrame(SpectrogramFrameData);
	}

	void WidgetAddFrame(TSharedPtr<SAudioSpectrogram> Widget, const FAtomConstantQResults& ConstantQResults, const float StartingFrequencyHz, const float NumBandsPerOctave, const EAtomSpectrumType SpectrumType)
	{
		const int32 NumBands = ConstantQResults.SpectrumValues.Num();

		const FAudioSpectrogramFrameData SpectrogramFrameData
		{
			.SpectrumValues = ConstantQResults.SpectrumValues,
			.SpectrumType = AtomToAudioSpectrumType(SpectrumType),
			.MinFrequency = StartingFrequencyHz,
			.MaxFrequency = StartingFrequencyHz * FMath::Pow(2.0f, (NumBands - 1) / NumBandsPerOctave),
			.bLogSpacedFreqencies = true,
		};

		Widget->AddFrame(SpectrogramFrameData);
	}
} // namespace

	void FAudioSpectrogram::OnSpectrumResults(UAtomSpectrumAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomSpectrumResults>& InSpectrumResultsArray)
	{
		if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::FFT && InSpectrumAnalyzer == SpectrumAnalyzer.Get())
		{
			for (const FAtomSpectrumResults& SpectrumResults : InSpectrumResultsArray)
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

				WidgetAddFrame(Widget, SpectrumResults, SpectrumAnalysisSettings->SpectrumType, SampleRate);
			}
		}
	}

	void FAudioSpectrogram::OnConstantQResults(UAtomConstantQAnalyzer* InSpectrumAnalyzer, int32 ChannelIndex, const TArray<FAtomConstantQResults>& InSpectrumResultsArray)
	{
		if (ActiveAnalyzerType == EAtomAudioSpectrumAnalyzerType::CQT && InSpectrumAnalyzer == ConstantQAnalyzer.Get())
		{
			for (const FAtomConstantQResults& SpectrumResults : InSpectrumResultsArray)
			{
				WidgetAddFrame(Widget, SpectrumResults, ConstantQSettings->StartingFrequencyHz, ConstantQSettings->NumBandsPerOctave, ConstantQSettings->SpectrumType);
			}
		}
	}

	void FAudioSpectrogram::Teardown()
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

		AudioBus.Reset();
		bUseExternalAudioBus = false;
	}

	void FAudioSpectrogram::ExtendSpectrumPlotContextMenu(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("AnalyzerSettings", LOCTEXT("AnalyzerSettings", "Analyzer Settings"));
		MenuBuilder.AddSubMenu(
			LOCTEXT("AnalyzerType", "Analyzer Type"),
			FText(),
			FNewMenuDelegate::CreateSP(this, &FAudioSpectrogram::BuildAnalyzerTypeSubMenu));
		MenuBuilder.AddSubMenu(
			LOCTEXT("FFTSize", "FFT Size"),
			FText(),
			FNewMenuDelegate::CreateSP(this, &FAudioSpectrogram::BuildFFTSizeSubMenu));
		MenuBuilder.EndSection();
	}

	void FAudioSpectrogram::BuildAnalyzerTypeSubMenu(FMenuBuilder& SubMenu)
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

	void FAudioSpectrogram::BuildFFTSizeSubMenu(FMenuBuilder& SubMenu)
	{
		// There is a different FFTSize enum depending on the analyzer type.

		if (AnalyzerType.Get() == EAtomAudioSpectrumAnalyzerType::FFT)
		{
			const UEnum* EnumClass = StaticEnum<EFFTSize>();
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

	EActiveTimerReturnType FAudioSpectrogram::Update(double InCurrentTime, float InDeltaTime)
	{
		if (AtomRuntimeID == FAudioBusInfo::InvalidAtomRuntimeID)
		{
			// No analyzers available if no valid audio device.
			ensure(!ActiveAnalyzerType.IsSet());
			return EActiveTimerReturnType::Continue;
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

		return EActiveTimerReturnType::Continue;
	}

	void FAudioSpectrogram::CreateAtomSpectrumAnalyzer()
	{
		ensure(!SpectrumAnalyzer.IsValid());
		ensure(!SpectrumResultsDelegateHandle.IsValid());

		SpectrumAnalyzer = TStrongObjectPtr(NewObject<UAtomSpectrumAnalyzer>());
		SpectrumAnalyzer->Settings = SpectrumAnalysisSettings.Get();
		SpectrumResultsDelegateHandle = SpectrumAnalyzer->OnSpectrumResultsNative.AddRaw(this, &FAudioSpectrogram::OnSpectrumResults);
	}

	void FAudioSpectrogram::ReleaseAtomSpectrumAnalyzer()
	{
		if (ensure(SpectrumAnalyzer.IsValid() && SpectrumResultsDelegateHandle.IsValid()))
		{
			SpectrumAnalyzer->OnSpectrumResultsNative.Remove(SpectrumResultsDelegateHandle);
		}

		SpectrumResultsDelegateHandle.Reset();
		SpectrumAnalyzer.Reset();
	}

	void FAudioSpectrogram::CreateConstantQAnalyzer()
	{
		ConstantQAnalyzer = TStrongObjectPtr(NewObject<UAtomConstantQAnalyzer>());
		ConstantQAnalyzer->Settings = ConstantQSettings.Get();
		ConstantQResultsDelegateHandle = ConstantQAnalyzer->OnConstantQResultsNative.AddRaw(this, &FAudioSpectrogram::OnConstantQResults);
	}

	void FAudioSpectrogram::ReleaseConstantQAnalyzer()
	{
		if (ensure(ConstantQAnalyzer.IsValid() && ConstantQResultsDelegateHandle.IsValid()))
		{
			ConstantQAnalyzer->OnConstantQResultsNative.Remove(ConstantQResultsDelegateHandle);
		}

		ConstantQResultsDelegateHandle.Reset();
		ConstantQAnalyzer.Reset();
	}

	TSharedRef<SDockTab> FAudioSpectrogram::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				GetWidget()
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAudioSpectrogram::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		using namespace AudioSpectrogramPrivate;

		FAudioSpectrogramParams AnalyzerParams;
		AnalyzerParams.NumChannels = Params.AudioBusInfo.GetNumChannels();
		AnalyzerParams.AtomRuntimeID = Params.AudioBusInfo.AtomRuntimeID;
		AnalyzerParams.ExternalAudioBus = Params.AudioBusInfo.AudioBus;

		if (Params.EditorSettingsClass != nullptr)
		{
			// If we have been given a valid editor settings class, bind analyzer options to the settings:
			const FProperty* SpectrogramSettingsProperty = Params.EditorSettingsClass->FindPropertyByName("SpectrogramSettings");
			if (SpectrogramSettingsProperty != nullptr)
			{
				const FRackUnitSettingsHelper SettingsHelper(*SpectrogramSettingsProperty);

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
				AnalyzerParams.FrequencyAxisPixelBucketMode.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->PixelPlotMode;
					});
				AnalyzerParams.FrequencyAxisScale.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->FrequencyScale;
					});
				AnalyzerParams.ColorMap.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->ColorMap;
					});
				AnalyzerParams.Orientation.BindLambda([=]()
					{
						return SettingsHelper.GetRackUnitSettings()->Orientation;
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
				AnalyzerParams.OnFrequencyAxisPixelBucketModeMenuEntryClicked.BindLambda([=](EAudioSpectrogramFrequencyAxisPixelBucketMode SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->PixelPlotMode = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnFrequencyAxisScaleMenuEntryClicked.BindLambda([=](EAudioSpectrogramFrequencyAxisScale SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->FrequencyScale = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnColorMapMenuEntryClicked.BindLambda([=](EAudioColorGradient SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->ColorMap = SelectedValue;
						SettingsHelper.SaveConfig();
					});
				AnalyzerParams.OnOrientationMenuEntryClicked.BindLambda([=](EOrientation SelectedValue)
					{
						SettingsHelper.GetRackUnitSettings()->Orientation = SelectedValue;
						SettingsHelper.SaveConfig();
					});
#endif
			}
		}

		return MakeShared<FAudioSpectrogram>(AnalyzerParams);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
