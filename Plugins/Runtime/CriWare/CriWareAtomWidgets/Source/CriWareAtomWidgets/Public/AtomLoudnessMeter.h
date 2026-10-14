
#pragma once

#include "Components/Widget.h"
#include "Delegates/IDelegateInstance.h"
#include "Styling/SlateTypes.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

#include "Atom/Atom.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomRack.h"
#include "Atom/Mixer/AtomMixerSubmix.h"

#include "SAtomLoudnessMeter.h"
#include "AtomLevelMeter.h"
#include "Analyzers/AtomLoudness.h"
#include "Analyzers/AtomTruePeak.h"
#include "AtomAudioAnalyzerRack.h"

#include "AtomLoudnessMeter.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

namespace AtomWidgets
{
	class FAtomLoudnessMeter : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;

		CRI_API FAtomLoudnessMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomRackBase> InAtomRack, const FAtomLevelMeterDefaultColorStyle* MeterColorStyle = nullptr);
		CRI_API FAtomLoudnessMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr, const FAtomLevelMeterDefaultColorStyle* MeterColorStyle = nullptr);
		CRI_API ~FAtomLoudnessMeter();

		CRI_API UAtomRackBase* GetRack() const;
		CRI_API UAtomAudioBus* GetAudioBus() const;

		CRI_API TSharedRef<SAtomLoudnessMeter> GetWidget() const;

		template<class T>
		TSharedRef<T> GetWidget() const
		{
			return StaticCastSharedRef<T>(Widget->AsShared());
		};

		CRI_API void Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomRackBase> InAtomRack);
		CRI_API void Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus);

		// Begin IAudioAnalyzerRackUnit overrides.
		virtual void SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo) override
		{
			Init(AudioBusInfo.AudioBus->GetNumChannels(), AudioBusInfo.AtomRuntimeID, AudioBusInfo.AudioBus);
		}

		CRI_API virtual TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args) const override;
		// End IAudioAnalyzerRackUnit overrides.
	protected:
		
		CRI_API void OnRackLoudnessMeterMeasure(const FAtomLoudnessMeterMeasure& Measure);
		CRI_API void OnRackTruePeakMeterMeasure(const TArray<FAtomTruePeakMeterMeasure>& ChannelMeasures);
		CRI_API void OnAudioBusLoudnessOutput(UAtomLoudnessAnalyzer* InLoudnessAnalyzer, const FAtomLoudnessResults& InResults);
		CRI_API void OnAudioBusTruePeakOutput(UAtomTruePeakAnalyzer* InTruePeakAnalyzer, const FAtomTruePeakResults& InResults);

	private:
		static CRI_API TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params);
		
		FReply OnReset();

		CRI_API void Teardown();

		/** The rack used for analysis (owns its loudness and true peak meters) */
		TStrongObjectPtr<UAtomRackBase> AtomRack;

		/** Atom analyzer object for AtomAudioBus. */
		TStrongObjectPtr<UAtomLoudnessAnalyzer> LoudnessAnalyzer;
		TStrongObjectPtr<UAtomTruePeakAnalyzer> TruePeakAnalyzer;

		/** The audio bus used for analysis. (uses an internal AtomAudioBus) */
		TStrongObjectPtr<UAtomAudioBus> AudioBus;

		/** Cached measures for the widget. */
		FAtomLoudnessMeterInfo LoudnessMeterInfo;

		/** Handle for results delegate for any Level meter analyzer. */
		FDelegateHandle LoudnessResultsDelegateHandle;
		FDelegateHandle TruePeakResultsDelegateHandle;

		/** Meter settings. */
		TStrongObjectPtr<UAtomLoudnessSettings> LoudnessSettings;
		TStrongObjectPtr<UAtomTruePeakSettings> TruePeakSettings;

		/** Sound Output Meter widget */
		TSharedPtr<SAtomLoudnessMeter> Widget;
		TSharedPtr<const FExtensionBase> ContextMenuExtension;

		bool bUseExternalAudioBus = false;

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;

		// delegate for rack level meter anlyser
		TStrongObjectPtr<UAtomLoudnessMeterDelegate> DelegateObject;
		FOnAtomRackLoudnessMeterMeasureBP LoudnessDelegateHandle;
		FOnAtomRackTruePeakMeterMeasureBP TruePeakDelegateHandle;

		friend UAtomLoudnessMeterDelegate;
	};
} // namespace

UCLASS()
class UAtomLoudnessMeterDelegate : public UObject
{
	GENERATED_BODY()

	AtomWidgets::FAtomLoudnessMeter* LoudnessMeter;

	UFUNCTION()
	void OnLoudnessMeterMeasure(const FAtomLoudnessMeterMeasure& Measure)
	{
		LoudnessMeter->OnRackLoudnessMeterMeasure(Measure);
	}

	void OnTruePeakMeterMeasure(const TArray<FAtomTruePeakMeterMeasure>& ChannelMeasures)
	{
		LoudnessMeter->OnRackTruePeakMeterMeasure(ChannelMeasures);
	}

	friend AtomWidgets::FAtomLoudnessMeter;
};

#undef CRI_API
