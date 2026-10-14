
#pragma once

#include "UObject/StrongObjectPtr.h"
#include "AudioVectorscopePanelStyle.h"
#include "AudioWidgetsEnums.h"

#include "CriWareDefines.h"
#include "Atom/AtomAudioBus.h"
#include "AtomWaveformAudioSamplesDataProvider.h"
#include "AtomAudioAnalyzerRack.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class SAudioVectorscopePanelWidget;

namespace AtomWidgets
{
	class FWaveformAudioSamplesDataProvider;

	class FAudioVectorscope : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;

		CRI_API FAudioVectorscope(FAtomRuntimeId InAtomRuntimeID,
			const uint32 InNumChannels, 
			const float InTimeWindowMs, 
			const float InMaxTimeWindowMs, 
			const float InAnalysisPeriodMs, 
			const EAudioPanelLayoutType InPanelLayoutType,
			const FAudioVectorscopePanelStyle* PanelStyle = nullptr,
			TObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr);

		CRI_API void CreateAudioBus(const uint32 InNumChannels);
		CRI_API void CreateDataProvider(Audio::FDeviceId InAudioDeviceId, const float InTimeWindowMs, const float InMaxTimeWindowMs, const float InAnalysisPeriodMs);
		CRI_API void CreateVectorscopeWidget(const EAudioPanelLayoutType InPanelLayoutType, const FAudioVectorscopePanelStyle* PanelStyle = nullptr);

		CRI_API virtual void StartProcessing() override;
		CRI_API virtual void StopProcessing() override;

		CRI_API UAtomAudioBus* GetAudioBus() const;
		CRI_API TSharedRef<SWidget> GetPanelWidget() const;

		// Begin IAudioAnalyzerRackUnit overrides.
		CRI_API virtual void SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo) override;
		CRI_API virtual TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args) const override;
		// End IAudioAnalyzerRackUnit overrides.

	private:
		static TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params);
		static constexpr float RackUnitTimeWindowMs = 30.0f;
		static constexpr float RackUnitMaxTimeWindowMs = 30.0f;
		static constexpr float RackUnitAnalysisPeriodMs = 10.0f;
		static constexpr EAudioPanelLayoutType RackUnitPanelLayoutType = EAudioPanelLayoutType::Basic;

		FAudioVectorscopePanelStyle VectorscopePanelStyle;

		TSharedPtr<FWaveformAudioSamplesDataProvider> AudioSamplesDataProvider = nullptr;
		TSharedPtr<SAudioVectorscopePanelWidget> VectorscopePanelWidget        = nullptr;

		TStrongObjectPtr<UAtomAudioBus> AudioBus = nullptr;
	};
} // namespace

#undef CRI_API
