
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

#include "SAtomLevelMeter.h"
#include "AtomLevelMeterTypes.h"
#include "AtomLevelMeterStyle.h"
#include "Analyzers/AtomMeter.h"
#include "AtomAudioAnalyzerRack.h"

#include "AtomLevelMeter.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

USTRUCT(BlueprintType)
struct FAtomLevelMeterDefaultColorStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

public:
	static CRI_API const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };

	static CRI_API const FAtomLevelMeterDefaultColorStyle& GetDefault();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterBackgroundColor = FLinearColor(0.031f, 0.031f, 0.031f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterValueColor = FLinearColor(0.025719f, 0.208333f, 0.069907f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterPeakColor = FLinearColor(0.24349f, 0.708333f, 0.357002f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterClippingColor = FLinearColor(1.0f, 0.0f, 0.112334f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterScaleColor = FLinearColor(0.017642f, 0.017642f, 0.017642f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor MeterScaleLabelColor = FLinearColor(0.442708f, 0.442708f, 0.442708f, 1.0f);
};

/**
 * An hybrid audio meter widget that can use ADX level meter or audio meter.
 *
 * Supports displaying a slower moving peak-hold value as well as the current meter value.
 *
 * A clipping value is also displayed which shows a customizable color to indicate clipping.
 *
 * Internal values are stored and interacted with as linear volume values.
 * 
 */
UCLASS(MinimalAPI)
class UAtomLevelMeter : public UWidget
{
	GENERATED_BODY()

public:

	DECLARE_DYNAMIC_DELEGATE_RetVal(TArray<FAtomLevelMeterMeasure>, FGetMeterMeasure);

	UAtomLevelMeter(const FObjectInitializer& ObjectInitializer);

	/** The current meter value to display. */
	UPROPERTY(EditAnywhere, Category = MeterValues)
	TArray<FAtomLevelMeterMeasure> MeterMeasures;

	/** A bindable delegate to allow logic to drive the value of the meter */
	UPROPERTY()
	FGetMeterMeasure MeterMeasuresDelegate;

public:
	
	/** The audio meter style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (DisplayName = "Style"))
	FAtomLevelMeterStyle WidgetStyle;

	/** The slider's orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EOrientation> Orientation;

	/** The color to draw the background. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor BackgroundColor;

	/** The color to draw the meter background. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FLinearColor MeterBackgroundColor;

	/** The color to draw the meter value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor MeterValueColor;

	/** The color to draw the meter peak value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor MeterPeakColor;

	/** The color to draw the meter clipping value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor MeterClippingColor;

	/** The color to draw the meter scale hashes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor MeterScaleColor;

	/** The color to draw the meter scale label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FLinearColor MeterScaleLabelColor;

public:

 	/** Gets the current linear value of the meter. */
 	UFUNCTION(BlueprintCallable, Category="Behavior")
	CRI_API TArray<FAtomLevelMeterMeasure> GetMeterMeasures() const;
 
 	/** Sets the current meter values. */
 	UFUNCTION(BlueprintCallable, Category="Behavior")
	CRI_API void SetMeterMeasures(const TArray<FAtomLevelMeterMeasure>& InMeterMeasures);

	/** Sets the background color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetBackgroundColor(FLinearColor InValue);

	/** Sets the meter background color */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	CRI_API void SetMeterBackgroundColor(FLinearColor InValue);

	/** Sets the meter value color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetMeterValueColor(FLinearColor InValue);

	/** Sets the meter peak color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetMeterPeakColor(FLinearColor InValue);

	/** Sets the meter clipping color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetMeterClippingColor(FLinearColor InValue);

	/** Sets the meter scale color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetMeterScaleColor(FLinearColor InValue);

	/** Sets the meter scale color */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	CRI_API void SetMeterScaleLabelColor(FLinearColor InValue);

	// UWidget interface
	CRI_API virtual void SynchronizeProperties() override;
	// End of UWidget interface

	// UVisual interface
	CRI_API virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	// End of UVisual interface

#if WITH_EDITOR
	CRI_API virtual const FText GetPaletteCategory() override;
#endif

protected:
	/** Native Slate Widget */
	TSharedPtr<SAtomLevelMeter> LevelMeterWidget;

	// UWidget interface
	CRI_API virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	//PROPERTY_BINDING_IMPLEMENTATION(TArray<FAtomLevelMeterMeasure>, MeterMeasures);

	// K2 Binding with conversion
	TArray<FAtomLevelMeterMeasure> K2_Cache_MeterMeasures;

	CRI_API TArray<FAtomLevelMeterChannelInfo> K2_Gate_MeterMeasures()
	{
		if (CanSafelyRouteEvent())
		{
			K2_Cache_MeterMeasures = TAttribute<TArray<FAtomLevelMeterMeasure>>::Create(
				MeterMeasuresDelegate.GetUObject(),
				MeterMeasuresDelegate.GetFunctionName()
			).Get();
		}
		TArray<FAtomLevelMeterChannelInfo> K2_MeterChannelInfo;
		for (auto& K2_Cache_Measure : K2_Cache_MeterMeasures)
		{
			K2_MeterChannelInfo.Add({ K2_Cache_Measure.Level, K2_Cache_Measure.PeakHoldLevel });
		}
		return K2_MeterChannelInfo;
	}
};

namespace AtomWidgets
{
	class FAtomLevelMeter : public IAudioAnalyzerRackUnit
	{
	public:
		static CRI_API const FAudioAnalyzerRackUnitTypeInfo RackUnitTypeInfo;

		CRI_API FAtomLevelMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomRackBase> InAtomRack, const FAtomLevelMeterDefaultColorStyle* MeterColorStyle = nullptr);
		CRI_API FAtomLevelMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr, const FAtomLevelMeterDefaultColorStyle* MeterColorStyle = nullptr);
		CRI_API ~FAtomLevelMeter();

		CRI_API UAtomRackBase* GetRack() const;
		CRI_API UAtomAudioBus* GetAudioBus() const;

		CRI_API TSharedRef<SAtomLevelMeter> GetWidget() const;

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
		
		CRI_API void OnRackLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& Measures);
		CRI_API void OnAudioBusMeterOutput(UAtomMeterAnalyzer* InMeterAnalyzer, int32 ChannelIndex, const FAtomMeterResults& InMeterResults);

	private:
		static CRI_API TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params);
		
		CRI_API void Teardown();

		/** The rack used for analysis (own its level meter) */
		TStrongObjectPtr<UAtomRackBase> AtomRack;

		/** Atom analyzer object for AtomAudioBus. */
		TStrongObjectPtr<UAtomMeterAnalyzer> Analyzer;

		/** The audio bus used for analysis. */
		TStrongObjectPtr<UAtomAudioBus> AudioBus;

		/** Cached measures for the widget. */
		TArray<FAtomLevelMeterChannelInfo> ChannelInfo;

		/** Handle for results delegate for any Level meter analyzer. */
		FDelegateHandle ResultsDelegateHandle;

		/** Meter settings. */
		TStrongObjectPtr<UAtomMeterSettings> Settings;

		/** Sound Output Meter widget */
		TSharedPtr<SAtomLevelMeter> Widget;

		bool bUseExternalAudioBus = false;

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;

		// delegate for rack level meter anlyser
		TStrongObjectPtr<UAtomLevelMeterDelegate> DelegateObject;
		FOnAtomRackLevelMeterMeasureBP DelegateHandle;

		friend UAtomLevelMeterDelegate;
	};
} // namespace

UCLASS()
class UAtomLevelMeterDelegate : public UObject
{
	GENERATED_BODY()

	AtomWidgets::FAtomLevelMeter* LevelMeter;

	UFUNCTION()
	void OnLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& Measures)
	{
		LevelMeter->OnRackLevelMeterMeasure(Measures);
	}

	friend AtomWidgets::FAtomLevelMeter;
};

#undef CRI_API
