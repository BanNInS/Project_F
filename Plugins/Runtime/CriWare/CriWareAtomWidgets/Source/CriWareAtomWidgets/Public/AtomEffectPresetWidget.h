#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SWidget.h"
#include "Components/Widget.h"

#include "AtomEffectPresetWidget.generated.h"

// Forward Declarations
class UAtomBusEffectPreset;
namespace AtomWidgets
{
	class SAtomEffectPresetWidget;
}

UCLASS()
class CRIWAREATOMWIDGETS_API UAtomEffectPresetWidget
	: public UWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Preset")
	TObjectPtr<UAtomBusEffectPreset> Preset = nullptr;

	UAtomEffectPresetWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Atom|Effect Preset")
	void Update();

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~ End UWidget Interface

	TSharedPtr<AtomWidgets::SAtomEffectPresetWidget> PresetWidget;
};
