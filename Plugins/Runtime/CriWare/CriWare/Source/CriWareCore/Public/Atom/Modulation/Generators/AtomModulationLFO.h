
#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationGenerator.h"

#include "AtomModulationLFO.generated.h"

UENUM(BlueprintType)
enum class EAtomModulationLFOShape : uint8
{
	Sine			  UMETA(DisplayName = "Sine"),
	UpSaw			  UMETA(DisplayName = "Saw (Up)"),
	DownSaw			  UMETA(DisplayName = "Saw (Down)"),
	Square			  UMETA(DisplayName = "Square"),
	Triangle		  UMETA(DisplayName = "Triangle"),
	Exponential		  UMETA(DisplayName = "Exponential"),
	RandomSampleHold  UMETA(DisplayName = "Random"),

	COUNT UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FAtomModulationLFOParams
{
	GENERATED_BODY()

	/** Shape of oscillating waveform */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 20, EditCondition = "!bBypass"))
	EAtomModulationLFOShape Shape = EAtomModulationLFOShape::Sine;

	/** Factor increasing/decreasing curvature of exponential LFO shape. */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 21, EditCondition = "!bBypass && Shape == EAtomModulationLFOShape::Exponential", EditConditionHides, ClampMin = "0.000001", UIMin = "0.25", UIMax = "10.0"))
	float ExponentialFactor = 3.5f;

	/** Pulse width of square LFO shape. */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 21, EditCondition = "!bBypass && Shape == EAtomModulationLFOShape::Square", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
	float Width = 0.5f;

	/** Amplitude of oscillator */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 30, EditCondition = "!bBypass", UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float Amplitude = 1.0f;

	/** Frequency of oscillator */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 40, EditCondition = "!bBypass", UIMin = "0", UIMax = "20", ClampMin = "0", ClampMax = "20", Unit=Hz))
	float Frequency = 1.0f;

	/** Amplitude offset of oscillator */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 50, EditCondition = "!bBypass", UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float Offset = 0.0f;

	/** Unit phase offset of oscillator */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 60, EditCondition = "!bBypass", UIMin = "0", UIMax = "1", ClampMin = "0"))
	float Phase = 0.0f;

	/** Whether or not to loop the oscillation more than once */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 70, EditCondition = "!bBypass"))
	bool bLooping = true;

	/** If true, bypasses LFO bus from being modulated by parameters, patches, or mixed (LFO remains active and computed). */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (DisplayPriority = 10))
	bool bBypass = false;
};


UCLASS(BlueprintType, hidecategories = Object, editinlinenew, MinimalAPI)
class UAtomModulationGeneratorLFO 
	: public UAtomModulationGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (ShowOnlyInnerProperties))
	FAtomModulationLFOParams Params;

	virtual AtomModulation::FGeneratorPtr CreateInstance() const override;
};
