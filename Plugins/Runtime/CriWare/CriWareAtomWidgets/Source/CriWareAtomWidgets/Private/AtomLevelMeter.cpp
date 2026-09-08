
#include "AtomLevelMeter.h"

#include "UObject/UObjectGlobals.h"

#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntime.h"

#include "Misc/CoreDelegates.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomLevelMeter)

#define LOCTEXT_NAMESPACE "ATOM_UMG"

UAtomLevelMeter::UAtomLevelMeter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Orientation = EOrientation::Orient_Vertical;

	// TODO: Move to style set
	BackgroundColor = FLinearColor(0.0075f, 0.0075f, 0.0075, 1.0f);
	MeterBackgroundColor = FLinearColor(0.031f, 0.031f, 0.031f, 1.0f);
	MeterValueColor = FLinearColor(0.025719f, 0.208333f, 0.069907f, 1.0f);
	MeterPeakColor = FLinearColor(0.24349f, 0.708333f, 0.357002f, 1.0f);
	MeterClippingColor = FLinearColor(1.0f, 0.0f, 0.112334f, 1.0f);
	MeterScaleColor = FLinearColor(0.017642f, 0.017642f, 0.017642f, 1.0f);
	MeterScaleLabelColor = FLinearColor(0.442708f, 0.442708f, 0.442708f, 1.0f);

	// Add a single channel as a default just so it can be seen when somebody makes one
	FAtomLevelMeterMeasure DefaultInfo;
	DefaultInfo.Level = -6.0f;
	DefaultInfo.PeakLevel = -3.0f;
	MeterMeasures.Add(DefaultInfo);

	WidgetStyle = FAtomLevelMeterStyle::GetDefault();

#if WITH_EDITORONLY_DATA
	AccessibleBehavior = ESlateAccessibleBehavior::NotAccessible;
	bCanChildrenBeAccessible = false;
#endif
}

TSharedRef<SWidget> UAtomLevelMeter::RebuildWidget()
{
	LevelMeterWidget = SNew(SAtomLevelMeter)
		.Style(&WidgetStyle);

	return LevelMeterWidget.ToSharedRef();
}

void UAtomLevelMeter::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	LevelMeterWidget->SetOrientation(Orientation);
	LevelMeterWidget->SetBackgroundColor(BackgroundColor);
	LevelMeterWidget->SetMeterBackgroundColor(MeterBackgroundColor);
	LevelMeterWidget->SetMeterValueColor(MeterValueColor);
	LevelMeterWidget->SetMeterPeakColor(MeterPeakColor);
	LevelMeterWidget->SetMeterClippingColor(MeterClippingColor);
	LevelMeterWidget->SetMeterScaleColor(MeterScaleColor);
	LevelMeterWidget->SetMeterScaleLabelColor(MeterScaleLabelColor);

	TAttribute<TArray<FAtomLevelMeterChannelInfo>> MeterChannelInfoBinding;
	if (MeterMeasuresDelegate.IsBound() && !IsDesignTime())
	{
		MeterChannelInfoBinding = TAttribute<TArray<FAtomLevelMeterChannelInfo>>::Create(
			TAttribute<TArray<FAtomLevelMeterChannelInfo>>::FGetter::CreateUObject(this, &ThisClass::K2_Gate_MeterMeasures));
	}
	else
	{
		TArray<FAtomLevelMeterChannelInfo> MeterChannelInfo;
		for (auto& Measure : MeterMeasures)
		{
			MeterChannelInfo.Add({ Measure.Level, Measure.PeakHoldLevel });
		}

		MeterChannelInfoBinding = TAttribute<TArray<FAtomLevelMeterChannelInfo>>(MeterChannelInfo);
	}
	LevelMeterWidget->SetMeterChannelInfo(MeterChannelInfoBinding);

	//TAttribute<TArray<FAtomLevelMeterMeasure>> MeterMeasuresBinding = PROPERTY_BINDING(TArray<FAtomLevelMeterMeasure>, MeterMeasures);
	//LevelMeterWidget->SetMeterChannelInfo(MeterMeasuresBinding);
}

void UAtomLevelMeter::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	LevelMeterWidget.Reset();
}

TArray<FAtomLevelMeterMeasure> UAtomLevelMeter::GetMeterMeasures() const
{
	if (LevelMeterWidget.IsValid())
	{
		TArray<FAtomLevelMeterMeasure> Measures;
		for (auto& ChannelInfo : LevelMeterWidget->GetMeterChannelInfo())
		{
			Measures.Add({ ChannelInfo.MeterValue, ChannelInfo.ClippingValue, ChannelInfo.PeakValue });
		}
		return Measures;
	}
	return TArray<FAtomLevelMeterMeasure>();
}

void UAtomLevelMeter::SetMeterMeasures(const TArray<FAtomLevelMeterMeasure>& InMeterMeasures)
{
	if (LevelMeterWidget.IsValid())
	{
		TArray<FAtomLevelMeterChannelInfo> MeterChannelInfo;
		for (auto& Measure : MeterMeasures)
		{
			MeterChannelInfo.Add({ Measure.Level, Measure.PeakHoldLevel });
		}

		// Update the widget
		LevelMeterWidget->SetMeterChannelInfo(MoveTemp(MeterChannelInfo));
	}
}

void UAtomLevelMeter::SetBackgroundColor(FLinearColor InValue)
{
	BackgroundColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetBackgroundColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterBackgroundColor(FLinearColor InValue)
{
	MeterBackgroundColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterBackgroundColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterValueColor(FLinearColor InValue)
{
	MeterValueColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterValueColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterPeakColor(FLinearColor InValue)
{
	MeterPeakColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterPeakColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterClippingColor(FLinearColor InValue)
{
	MeterClippingColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterClippingColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterScaleColor(FLinearColor InValue)
{
	MeterScaleColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterScaleColor(InValue);
	}
}

void UAtomLevelMeter::SetMeterScaleLabelColor(FLinearColor InValue)
{
	MeterScaleLabelColor = InValue;
	if (LevelMeterWidget.IsValid())
	{
		LevelMeterWidget->SetMeterScaleLabelColor(InValue);
	}
}

#if WITH_EDITOR
const FText UAtomLevelMeter::GetPaletteCategory()
{
	return LOCTEXT("Atom", "Atom");
}
#endif

const FName FAtomLevelMeterDefaultColorStyle::TypeName(TEXT("FAtomLevelMeterDefaultColorStyle"));

const FAtomLevelMeterDefaultColorStyle& FAtomLevelMeterDefaultColorStyle::GetDefault()
{
	static FAtomLevelMeterDefaultColorStyle Style;
	return Style;
}

namespace AtomWidgets
{
	const FAudioAnalyzerRackUnitTypeInfo FAtomLevelMeter::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAtomLevelMeter"),
		.DisplayName = LOCTEXT("AtomLevelMeterMeterDisplayName", "Meter"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.5f,
	};

	FAtomLevelMeter::FAtomLevelMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, const TObjectPtr<UAtomAudioBus> InExternalAudioBus, const FAtomLevelMeterDefaultColorStyle* MeterWidgetStyle)
	{
		const FAtomLevelMeterDefaultColorStyle* WidgetStyle = MeterWidgetStyle ? MeterWidgetStyle : &AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
		Widget = SNew(SAtomLevelMeter)
			.Orientation(EOrientation::Orient_Vertical)
			.BackgroundColor(FLinearColor::Transparent)
			.MeterBackgroundColor(WidgetStyle->MeterBackgroundColor)
			.MeterValueColor(WidgetStyle->MeterValueColor)
			.MeterPeakColor(WidgetStyle->MeterPeakColor)
			.MeterClippingColor(WidgetStyle->MeterClippingColor)
			.MeterScaleColor(WidgetStyle->MeterScaleColor)
			.MeterScaleLabelColor(WidgetStyle->MeterScaleLabelColor);

		Init(InNumChannels, InAtomRuntimeID, InExternalAudioBus);
	}

	FAtomLevelMeter::FAtomLevelMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, const TObjectPtr<UAtomRackBase> InAtomRack, const FAtomLevelMeterDefaultColorStyle* MeterWidgetStyle)
	{
		const FAtomLevelMeterDefaultColorStyle* WidgetStyle = MeterWidgetStyle ? MeterWidgetStyle : &AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
		Widget = SNew(SAtomLevelMeter)
			.Orientation(EOrientation::Orient_Vertical)
			.BackgroundColor(FLinearColor::Transparent)
			.MeterBackgroundColor(WidgetStyle->MeterBackgroundColor)
			.MeterValueColor(WidgetStyle->MeterValueColor)
			.MeterPeakColor(WidgetStyle->MeterPeakColor)
			.MeterClippingColor(WidgetStyle->MeterClippingColor)
			.MeterScaleColor(WidgetStyle->MeterScaleColor)
			.MeterScaleLabelColor(WidgetStyle->MeterScaleLabelColor);

		Init(InNumChannels, InAtomRuntimeID, InAtomRack);
	}

	FAtomLevelMeter::~FAtomLevelMeter()
	{
		Teardown();
	}

	UAtomAudioBus* FAtomLevelMeter::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	UAtomRackBase* FAtomLevelMeter::GetRack() const
	{
		return AtomRack.Get();
	}

	TSharedRef<SAtomLevelMeter> FAtomLevelMeter::GetWidget() const
	{
		return StaticCastSharedRef<SAtomLevelMeter>(Widget->AsShared());
	}

	void FAtomLevelMeter::Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomRackBase> InAtomRack)
	{
		AtomRuntimeID = InAtomRuntimeID;

		check(InNumChannels > 0);

		Teardown();

		AtomRack = InAtomRack ? TStrongObjectPtr(InAtomRack.Get()) : nullptr;
		
		if (AtomRack.IsValid())
		{
			DelegateObject = TStrongObjectPtr(NewObject<UAtomLevelMeterDelegate>());
			DelegateObject->LevelMeter = this;

			DelegateHandle.BindUFunction(DelegateObject.Get(), "OnLevelMeterMeasure");

			FAtomRuntime* AtomRuntime = FAtomRuntimeManager::Get()->GetAtomRuntimeRaw(InAtomRuntimeID);
			AtomRuntime->AddLevelMeterDelegate(InAtomRack, DelegateHandle);

			FAtomLevelMeterSettings RackLevelMeterSettings;
			RackLevelMeterSettings.AnalysisPeriod = 0.01f;
			RackLevelMeterSettings.PeakHoldTime = 4000;
			AtomRuntime->StartLevelMeterMeasuring(InAtomRack, RackLevelMeterSettings);
		}

		constexpr float DefaultMeterValue = ATOM_MIN_VOLUME_DECIBELS;
		constexpr float DefaultPeakValue = ATOM_MIN_VOLUME_DECIBELS;
		ChannelInfo.Init(FAtomLevelMeterChannelInfo{ DefaultMeterValue, DefaultPeakValue }, InNumChannels);

		if (Widget.IsValid())
		{
			Widget->SetMeterChannelInfo(ChannelInfo);
		}
	}

	void FAtomLevelMeter::Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus)
	{
		AtomRuntimeID = InAtomRuntimeID;

		check(InNumChannels > 0);

		Teardown();

		Settings = TStrongObjectPtr(NewObject<UAtomMeterSettings>());
		Settings->AnalysisPeriod = 0.01f;
		Settings->PeakHoldTime = 4000.0f;
		Settings->MeterAttackTime = 20;
		Settings->MeterReleaseTime = 20;

		Analyzer = TStrongObjectPtr(NewObject<UAtomMeterAnalyzer>());
		Analyzer->Settings = Settings.Get();

		bUseExternalAudioBus = InExternalAudioBus != nullptr;

		AudioBus = bUseExternalAudioBus ? TStrongObjectPtr(InExternalAudioBus.Get()) : TStrongObjectPtr(NewObject<UAtomAudioBus>());
		if (!bUseExternalAudioBus)
		{
			AudioBus->AudioBusChannels = EAtomAudioBusChannels(InNumChannels - 1);
		}

		ResultsDelegateHandle = Analyzer->OnLatestPerChannelMeterResultsNative.AddRaw(this, &FAtomLevelMeter::OnAudioBusMeterOutput);
		Analyzer->StartAnalyzing(InAtomRuntimeID, AudioBus.Get());

		constexpr float DefaultMeterValue = ATOM_MIN_VOLUME_DECIBELS;
		constexpr float DefaultPeakValue = ATOM_MIN_VOLUME_DECIBELS;
		ChannelInfo.Init(FAtomLevelMeterChannelInfo{ DefaultMeterValue, DefaultPeakValue }, InNumChannels);

		if (Widget.IsValid())
		{
			Widget->SetMeterChannelInfo(ChannelInfo);
		}
	}

	void FAtomLevelMeter::OnRackLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& InMeasures)
	{
		if (Widget.IsValid())
		{
			const int NumChannels = FMath::Min(InMeasures.Num(), ChannelInfo.Num());
			for (int ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				FAtomLevelMeterChannelInfo NewChannelInfo;
				NewChannelInfo.MeterValue = InMeasures[ChannelIndex].Level;
				NewChannelInfo.PeakValue = InMeasures[ChannelIndex].PeakHoldLevel;
				//NewChannelInfo.ClippingValue = InMeasures[ChannelIndex].PeakLevel;
				ChannelInfo[ChannelIndex] = MoveTemp(NewChannelInfo);
			}

			// Update the widget
			Widget->SetMeterChannelInfo(ChannelInfo);
		}
	}

	void FAtomLevelMeter::OnAudioBusMeterOutput(UAtomMeterAnalyzer* InMeterAnalyzer, int32 ChannelIndex, const FAtomMeterResults& InMeterResults)
	{
		if (InMeterAnalyzer == Analyzer.Get())
		{
			FAtomLevelMeterChannelInfo NewChannelInfo;
			NewChannelInfo.MeterValue = InMeterResults.MeterValue;
			NewChannelInfo.PeakValue = InMeterResults.PeakValue;

			if (ChannelIndex < ChannelInfo.Num())
			{
				ChannelInfo[ChannelIndex] = MoveTemp(NewChannelInfo);
			}

			// Update the widget if this is the last channel
			if (ChannelIndex == ChannelInfo.Num() - 1)
			{
				if (Widget.IsValid())
				{
					Widget->SetMeterChannelInfo(ChannelInfo);
				}
			}
		}
	}

	void FAtomLevelMeter::Teardown()
	{
		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			FAtomRuntimeHandle AtomRuntime = RuntimeManager->GetAtomRuntime(AtomRuntimeID);
			if (AtomRuntime.IsValid() && AtomRack.IsValid())
			{
				AtomRuntime->StopLevelMeterMeasuring(AtomRack.Get());
				AtomRuntime->RemoveLevelMeterDelegate(AtomRack.Get(), DelegateHandle);
			}
		}

		if (Analyzer.IsValid() && Analyzer->IsValidLowLevel())
		{
			Analyzer->StopAnalyzing();
			if (ResultsDelegateHandle.IsValid())
			{
				Analyzer->OnLatestPerChannelMeterResultsNative.Remove(ResultsDelegateHandle);
			}

			Analyzer.Reset();
		}

		ResultsDelegateHandle.Reset();

		DelegateHandle.Clear();
		DelegateObject.Reset();

		AtomRack.Reset();
		AudioBus.Reset();
		ChannelInfo.Reset();
		Settings.Reset();

		bUseExternalAudioBus = false;
	}

	TSharedRef<SDockTab> FAtomLevelMeter::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					[
						GetWidget()
					]
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAtomLevelMeter::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		// If we have an AudioMaterialMeterStyle create as an AudioMaterialMeter, otherwise create as a standard Slate meter:
		//const FName MaterialMeterStyleName("AudioMaterialMeter.Style");
		/*if (Params.StyleSet->HasWidgetStyle<FAudioMaterialMeterStyle>(MaterialMeterStyleName))
		{
			const AudioWidgets::FAudioMaterialMeterStyle& AudioMaterialMeterStyle = Params.StyleSet->GetWidgetStyle<FAudioMaterialMeterStyle>(MaterialMeterStyleName);
			return MakeShared<FAtomLevelMeter>(Params.AudioBusInfo.GetNumChannels(), Params.AudioBusInfo.AtomRuntimeID, AudioMaterialMeterStyle, Params.AudioBusInfo.AudioBus);
		}
		else*/
		{
			const FAtomLevelMeterDefaultColorStyle& MeterDefaultColorStyle = Params.StyleSet->GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
			if (Params.AtomRackInfo.AtomRack != nullptr)
			{
				return MakeShared<FAtomLevelMeter>(Params.AtomRackInfo.GetNumChannels(), Params.AtomRackInfo.AtomRuntimeID, Params.AtomRackInfo.AtomRack, &MeterDefaultColorStyle);
			}
			
			return MakeShared<FAtomLevelMeter>(Params.AudioBusInfo.GetNumChannels(), Params.AudioBusInfo.AtomRuntimeID, Params.AudioBusInfo.AudioBus, &MeterDefaultColorStyle);
		}
	}
} // namespace

#undef LOCTEXT_NAMESPACE
