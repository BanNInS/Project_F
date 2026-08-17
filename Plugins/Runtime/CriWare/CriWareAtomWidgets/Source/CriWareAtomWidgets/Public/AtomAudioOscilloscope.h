
#pragma once

#include "UObject/StrongObjectPtr.h"
#include "SAudioOscilloscopePanelWidget.h"
#include "AudioOscilloscopePanelStyle.h"
#include "AudioWidgetsEnums.h"

#include "CriWareDefines.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomRack.h"

#include "AtomAudioAnalyzerRack.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class SAudioOscilloscopePanelWidget;

namespace AtomWidgets
{
	class FAudioSamplesDataProvider;

	class FAudioOscilloscope : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;

		CRI_API FAudioOscilloscope(FAtomRuntimeId InAtomRuntimeID,
			const uint32 InNumChannels, 
			const float InTimeWindowMs, 
			const float InMaxTimeWindowMs, 
			const float InAnalysisPeriodMs, 
			const EAudioPanelLayoutType InPanelLayoutType,
			const FAudioOscilloscopePanelStyle* InOscilloscopePanelStyle,
			TObjectPtr<UAtomAudioBus> InExternalAudioBus);

		CRI_API FAudioOscilloscope(FAtomRuntimeId InAtomRuntimeID,
			TObjectPtr<UAtomRackBase> InAtomRack,
			const uint32 InNumChannels,
			const float InTimeWindowMs,
			const float InMaxTimeWindowMs,
			const float InAnalysisPeriodMs,
			const EAudioPanelLayoutType InPanelLayoutTypeconst,
			const FAudioOscilloscopePanelStyle* InOscilloscopePanelStyle);

		CRI_API void CreateAudioBus(const uint32 InNumChannels);
		CRI_API void SetAtomRack(TObjectPtr<UAtomRackBase> InAtomRack);

		CRI_API void CreateDataProvider(FAtomRuntimeId InAtomRuntimeID,
			const float InTimeWindowMs,
			const float InMaxTimeWindowMs,
			const float InAnalysisPeriodMs,
			const EAudioPanelLayoutType InPanelLayoutType);

		CRI_API void CreateOscilloscopeWidget(const uint32 InNumChannels, const EAudioPanelLayoutType InPanelLayoutType);

		CRI_API void StartProcessing();
		CRI_API void StopProcessing();

		CRI_API UAtomAudioBus* GetAudioBus() const;
		CRI_API TSharedRef<SWidget> GetPanelWidget() const;

		// Begin IAudioAnalyzerRackUnit overrides.
		CRI_API virtual void SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo) override;
		CRI_API virtual TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args) const override;
		// End IAudioAnalyzerRackUnit overrides.

	private:
		static TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params);
		static constexpr float RackUnitTimeWindowMs = 10.0f;
		static constexpr float RackUnitMaxTimeWindowMs = 10.0f;
		static constexpr float RackUnitAnalysisPeriodMs = 10.0f;
		static constexpr EAudioPanelLayoutType RackUnitPanelLayoutType = EAudioPanelLayoutType::Basic;

		FAudioOscilloscopePanelStyle OscilloscopePanelStyle;

		TSharedPtr<FAudioSamplesDataProvider> AudioSamplesDataProvider = nullptr;
		TSharedPtr<SAudioOscilloscopePanelWidget> OscilloscopePanelWidget = nullptr;

		TStrongObjectPtr<UAtomAudioBus> AudioBus = nullptr;
		TStrongObjectPtr<UAtomRackBase> AtomRack = nullptr;
	};
} // namespace

#undef CRI_API
