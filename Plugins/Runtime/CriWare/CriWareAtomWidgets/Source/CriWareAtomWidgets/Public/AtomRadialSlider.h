#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SWidget.h"
#include "Components/Widget.h"

#include "AtomRadialSlider.generated.h"

// Forward Definitions
namespace AtomWidgets
{
	class SAtomRadialSlider;
}

UCLASS()
class CRIWAREATOMWIDGETS_API UAtomRadialSlider
	: public UWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "AtomRadialSlider")
	float Value = 0.0f;

	// A bindable delegate to allow logic to drive the value of the widget
	FGetFloat ValueDelegate;

	PROPERTY_BINDING_IMPLEMENTATION(float, Value);

	UAtomRadialSlider(const FObjectInitializer& ObjectInitializer);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

protected:

	// from UWidget
	virtual TSharedRef<SWidget> RebuildWidget() override;

	TSharedPtr<AtomWidgets::SAtomRadialSlider> AtomRadialSlider;
};
