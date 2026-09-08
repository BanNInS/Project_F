
#include "AtomEffectPresetWidget.h"

#include "SAtomEffectPresetWidget.h"

UAtomEffectPresetWidget::UAtomEffectPresetWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UAtomEffectPresetWidget::Update()
{
	if (PresetWidget.IsValid())
	{
		PresetWidget->Update();
	}
}

void UAtomEffectPresetWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (PresetWidget.IsValid())
	{
		PresetWidget->Update();
	}
}

void UAtomEffectPresetWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	PresetWidget.Reset();
}

TSharedRef<SWidget> UAtomEffectPresetWidget::RebuildWidget()
{
	return SAssignNew(PresetWidget, AtomWidgets::SAtomEffectPresetWidget)
		.Preset(Preset);
}
