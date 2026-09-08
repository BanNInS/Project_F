
#include "SubmixAudioAnalyzerRack.h"

#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"
#include "AudioOscilloscopePanelStyle.h"
#include "AudioVectorscopePanelStyle.h"
#include "AudioWidgetsStyle.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/Mixer/AtomMixerSubmix.h"

#include "AtomLevelMeter.h"
#include "AtomWidgetsStyle.h"
#include "AtomAudioAnalyzerRack.h"

#include "CriWareAtomInsightsEditorModule.h"
#include "CriWareAtomInsightsEditorSettings.h"
#include "AtomInsightsStyle.h"

namespace Atom::Insights
{
	namespace FSubmixAudioAnalyzerRackPrivate
	{
		/**
		 * This style set is given to the AudioWidgets::FAudioAnalyzerRack to override the parent AudioWidgetsStyle.
		 */
		class FAnalyzerRackStyleSet final : public FSlateStyleSet
		{
		public:
			static FAnalyzerRackStyleSet& Get()
			{
				static FAnalyzerRackStyleSet Instance;
				return Instance;
			}

			FAnalyzerRackStyleSet()
				: FSlateStyleSet("AtomInsightsAnalyzerRackStyleSet")
			{
				SetParentStyleName(FAudioWidgetsStyle::Get().GetStyleSetName());

				const FLinearColor AnalyzerForegroundColor(0.025719f, 0.208333f, 0.069907f, 1.0f); // "Audio" Green

				// Override colors for these widget styles:

				FAtomLevelMeterDefaultColorStyle MeterStyle;
				MeterStyle.MeterValueColor = AnalyzerForegroundColor;
				Set("AtomLevelMeter.DefaultColorStyle", MeterStyle);
				
				Set("AudioOscilloscope.PanelStyle", FAudioOscilloscopePanelStyle()
					.SetWaveViewerStyle(FSampledSequenceViewerStyle()
						.SetSequenceColor(AnalyzerForegroundColor)));

				Set("AudioSpectrumPlot.Style", FAudioSpectrumPlotStyle()
					.SetCrosshairColor(FSlateColor(AnalyzerForegroundColor).UseSubduedForeground())
					.SetSpectrumColor(AnalyzerForegroundColor));

				Set("AudioVectorscope.PanelStyle", FAudioVectorscopePanelStyle()
					.SetVectorViewerStyle(FSampledSequenceVectorViewerStyle()
						.SetLineColor(AnalyzerForegroundColor)));
			}

		protected:
			virtual const FSlateWidgetStyle* GetWidgetStyleInternal(const FName DesiredTypeName, const FName StyleName, const FSlateWidgetStyle* DefaultStyle, bool bWarnIfNotFound) const override
			{
				if (DesiredTypeName == FAudioMaterialMeterStyle::TypeName)
				{
					// Return null for this type to disable use of the audio material meter.
					ensure(!bWarnIfNotFound);
					return nullptr;
				}

				return FSlateStyleSet::GetWidgetStyleInternal(DesiredTypeName, StyleName, DefaultStyle, bWarnIfNotFound);
			}
		};

		TSharedRef<AtomWidgets::FAudioAnalyzerRack> CreateAudioAnalyzerRack()
		{
			using namespace AtomWidgets;

			// Set params so that rack layout is stored specific to Audio Insights and the custom style set is used for analyzer widgets:
			const FAudioAnalyzerRack::FRackConstructParams Params
			{
				.TabManagerLayoutName = TEXT("AtomInsights_FAudioAnalyzerRack_v0"),
				.StyleSet = &FAnalyzerRackStyleSet::Get(),
				.EditorSettingsClass = UCriWareAtomInsightsEditorSettings::StaticClass(),
			};

			return MakeShared<AtomWidgets::FAudioAnalyzerRack>(Params);
		}
	} // namespace FSubmixAudioAnalyzerRackPrivate

	FSubmixAudioAnalyzerRack::FSubmixAudioAnalyzerRack(TWeakObjectPtr<UAtomRackBase> InAtomRack)
		: AudioAnalyzerRack(FSubmixAudioAnalyzerRackPrivate::CreateAudioAnalyzerRack())
	{
		RebuildAudioAnalyzerRack(InAtomRack);
	}

	FSubmixAudioAnalyzerRack::FSubmixAudioAnalyzerRack(TWeakObjectPtr<UAtomBus> InAtomBus)
		: AudioAnalyzerRack(FSubmixAudioAnalyzerRackPrivate::CreateAudioAnalyzerRack())
	{
		RebuildAudioAnalyzerRack(InAtomBus);
	}

	FSubmixAudioAnalyzerRack::~FSubmixAudioAnalyzerRack()
	{
		CleanupAudioAnalyzerRack();
	}

	TSharedRef<SWidget> FSubmixAudioAnalyzerRack::MakeWidget(TSharedRef<SDockTab> InOwnerTab, const FSpawnTabArgs& InSpawnTabArgs)
	{
		return AudioAnalyzerRack->CreateWidget(InOwnerTab, InSpawnTabArgs);
	}

	void FSubmixAudioAnalyzerRack::RebuildAudioAnalyzerRack(TWeakObjectPtr<UAtomRackBase> InAtomRack)
	{
		using namespace ::Audio;

		if (AtomRack.IsValid() || AtomBus.IsValid())
		{
			CleanupAudioAnalyzerRack();
		}

		AtomRack = InAtomRack;

		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
		const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

		const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);
		if (!AtomRuntime)
		{
			return;
		}

		if (!AtomRack.IsValid())
		{
			return;
		}

		AudioAnalyzerRack->Init(AtomRack.Get(), AtomRuntimeID);
		AudioAnalyzerRack->SetTitleText(FText::FromString(AtomRack->GetName()), FText::FromString(AtomRack->GetPathName()));

		// Start processing
		AudioAnalyzerRack->StartProcessing();
	}

	void FSubmixAudioAnalyzerRack::RebuildAudioAnalyzerRack(TWeakObjectPtr<UAtomBus> InAtomBus)
	{
		using namespace ::Audio;

		if (AtomRack.IsValid() || AtomBus.IsValid())
		{
			CleanupAudioAnalyzerRack();
		}

		AtomBus = InAtomBus;

		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
		const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

		const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);
		if (!AtomRuntime)
		{
			return;
		}

		if (!AtomBus.IsValid())
		{
			return;
		}

		Atom::FMixerSubmixWeakPtr MixerSubmixWeakPtr = AtomRuntime->GetSubmixInstance(AtomBus.Get());
		if (!MixerSubmixWeakPtr.IsValid())
		{
			return;
		}

		const int32 MixerNumChannels = MixerSubmixWeakPtr.Pin()->GetNumOutputChannels();

		AudioAnalyzerRack->Init(MixerNumChannels, AtomRuntimeID);

		FText TitleText = FText::Format(NSLOCTEXT("AtomInsights", "AudioAnalyzerRack_TitleText_Format", "{0}:{1}"), FText::FromString(AtomBus->GetRack()->GetName()), FText::FromString(AtomBus->GetName()));
		AudioAnalyzerRack->SetTitleText(TitleText, FText::FromString(AtomBus->GetPathName()));

		// Start processing
		AudioAnalyzerRack->StartProcessing();

		// Register audio bus in submix
		const TObjectPtr<UAtomAudioBus> AudioBus = AudioAnalyzerRack->GetAudioBus();
		if (!AudioBus)
		{
			return;
		}

		const FAudioBusKey AudioBusKey(AudioBus->GetUniqueID());
		const int32 AudioBusNumChannels = AudioBus->GetNumChannels();

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, MixerSubmixWeakPtr, AudioBusKey, AudioBusNumChannels]()
		{
			TObjectPtr<UAtomAudioBusSubsystem> AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
			check(AudioBusSubsystem);

			if (FMixerSubmixPtr MixerSubmix = MixerSubmixWeakPtr.Pin();
				MixerSubmix.IsValid())
			{
				MixerSubmix->RegisterAudioBus(AudioBusKey, AudioBusSubsystem->AddPatchInputForAudioBus(AudioBusKey, AtomRuntime->GetRuntimeNumOutputFrames(), AudioBusNumChannels));
			}
		});
	}

	void FSubmixAudioAnalyzerRack::CleanupAudioAnalyzerRack()
	{
		using namespace ::Audio;

		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const ICriWareAtomInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<ICriWareAtomInsightsModule>(ICriWareAtomInsightsModule::GetName());
		const FAtomRuntimeId AtomRuntimeID = InsightsModule.GetRuntimeID();

		const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);
		if (!AtomRuntime)
		{
			return;
		}

		if (AtomRack.IsValid())
		{
			// Stop processing
			AudioAnalyzerRack->StopProcessing();

			AtomRack.Reset();
		}
		else if (AtomBus.IsValid())
		{
			FMixerSubmixWeakPtr MixerSubmixWeakPtr = AtomRuntime->GetSubmixInstance(AtomBus.Get());
			if (!MixerSubmixWeakPtr.IsValid())
			{
				return;
			}

			// Unregister audio bus from submix
			const TObjectPtr<UAtomAudioBus> AudioBus = AudioAnalyzerRack->GetAudioBus();
			if (!AudioBus)
			{
				return;
			}

			const FAudioBusKey AudioBusKey(AudioBus->GetUniqueID());

			FAtomThread::RunCommandOnAtomThread([MixerSubmixWeakPtr, AudioBusKey]()
			{
				if (FMixerSubmixPtr MixerSubmix = MixerSubmixWeakPtr.Pin();
					MixerSubmix.IsValid())
				{
					MixerSubmix->UnregisterAudioBus(AudioBusKey);
				}
			});
		
			// Stop processing
			AudioAnalyzerRack->StopProcessing();

			AtomBus.Reset();
		}
	}
} // namespace
