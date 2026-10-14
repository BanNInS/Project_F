#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SWidget.h"
#include "Components/TextWidgetTypes.h"

#include "SBandpassEQWidget.h"
#include "SBandpassEQGraph.h"
#include "SAtomEffectPresetWidget.h"

#include "BandpassEQWidget.generated.h"

UCLASS()
class CRIWAREATOMWIDGETS_API UBandpassEQWidget
	: public UWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BandEQWidget")
	TObjectPtr<UAtomBusEffectPreset> Preset = nullptr;

	UBandpassEQWidget(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{}

	UFUNCTION(BlueprintCallable, Category = "BandEQWidget")
	void Update()
	{
		if (AtomEffectPresetWidget.IsValid())
		{
			AtomEffectPresetWidget->Update();
		}
	}
	// UWidget interface
	virtual void SynchronizeProperties() override
	{
		Super::SynchronizeProperties();
		//AtomEffectPresetWidget->Update();

	}

	virtual void ReleaseSlateResources(bool bReleaseChildren) override
	{
		Super::ReleaseSlateResources(bReleaseChildren);

		BandpassEQWidget.Reset();
	}

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		return SAssignNew(BandpassEQWidget, AtomWidgets::SBandpassEQWidget)
			.Preset(Preset);
	}

	TSharedPtr<AtomWidgets::SBandpassEQWidget> BandpassEQWidget;
	TSharedPtr<AtomWidgets::SAtomEffectPresetWidget> AtomEffectPresetWidget;
};
