
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"

#include "AtomModulation.h"
#include "AtomModulationValue.h"

#include "AtomModulationControlBusMix.generated.h"

#define CRI_API CRIWARECORE_API

// Forward Declarations
class UAtomModulationControlBus;

USTRUCT(BlueprintType)
struct FAtomModulationControlBusMixStage
{
	GENERATED_BODY()

public:

	CRI_API FAtomModulationControlBusMixStage();
	CRI_API FAtomModulationControlBusMixStage(UAtomModulationControlBus* InBus, const float TargetValue);

	/* Bus controlled by stage. */
	UPROPERTY(EditAnywhere, Category = Stage, BlueprintReadWrite)
	TObjectPtr<UAtomModulationControlBus> Bus;

	/* Value mix is set to. */
	UPROPERTY(EditAnywhere, Category = Stage, BlueprintReadWrite)
	FAtomModulationMixValue Value;
};

UCLASS(config = CriWare, AutoExpandCategories = (Stage, Mix), EditInLineNew, BlueprintType, MinimalAPI, meta = (PrioritizeCategories = "Mix Config Stages"))
class UAtomModulationControlBusMix : public UObject
{
	GENERATED_BODY()

public:

	UAtomModulationControlBusMix(const FObjectInitializer& ObjectInitializer);

protected:
	// Loads the mix from the provided profile index
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "50"))
	void LoadMixFromProfile();

	// Saves the mix to the provided profile index
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "60"))
	void SaveMixToProfile();

	// Solos this mix, deactivating all others and activating this
	// (if its not already active), while testing in-editor in all
	// active worlds
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "40"))
	void SoloMix();

	// Activates this mix in all active worlds
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "10"))
	void ActivateMix();

	// Deactivates this mix in all active worlds
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "20"))
	void DeactivateMix();

	// Deactivates all mixes in all active worlds
	UFUNCTION(Category = Mix, meta = (CallInEditor = "true", DisplayPriority = "30"))
	void DeactivateAllMixes();

public:
	UPROPERTY(EditAnywhere, Transient, Category = Config, meta = (DisplayPriority = "30"))
	uint32 ProfileIndex = 0;

	// Once activated, the mix will start a timer for the given duration (seconds).
	// When the timer ends, the mix will be deactivated.
	// As a result, the attack time is included in this duration, but not the release time.
	// When set to 0, the mix is activated and then immediately deactivated.
	// If less than 0, the mix will remain activated until manually deactivated.
	UPROPERTY(EditAnywhere, Category = Config, meta = (DisplayPriority = "10"), BlueprintReadOnly)
	double Duration = -1.0;

	// If a Mix is already active and you activate it again, one of two things will happen:
	// If set to true, the stages will all go back to their default values and the mix will activate again,
	// allowing the attack to trigger again.
	// If set to false, calling activate will only reset the timer to deactivate (based on the Duration value).
	UPROPERTY(EditAnywhere, Category = Config, meta = (DisplayPriority = "20"), BlueprintReadOnly)
	bool bRetriggerOnActivation = false;

	virtual void BeginDestroy() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent) override;
	virtual void OnPropertyChanged(FProperty* Property, EPropertyChangeType::Type ChangeType);
#endif // WITH_EDITOR

	/* Array of stages controlled by mix. */
	UPROPERTY(EditAnywhere, Category = Stages, BlueprintReadOnly)
	TArray<FAtomModulationControlBusMixStage> MixStages;
};

#undef CRI_API
