#pragma once

#include "CoreMinimal.h"

#include "AtomEnvelope.generated.h"

/** Type of cure to use for envelope's Attack, Decay and Release interpolations. */
UENUM(BlueprintType)
enum class EAtomEnvelopeCurve : uint8
{
	// Linear Curve
	Linear = 0,

	// Exponential, Fast Change Curve
	Exponential,

	// Logarithmic, Slow Change Curve
	Logarithmic,

	// S-Curve, Sinusoidal Curve
	SCurve UMETA(DisplayName = "Sin (S-Curve)"),

	// Inverted S-Curve, Sinusoidal Curve
	InvertedSCurve UMETA(DisplayName = "Sin (Inverted S-Curve)"),

	Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomEnvelope
{
	GENERATED_BODY()

public:

	/** The envelope attack time in miliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ForceUnits = ms))
	float AttackTime = 0.0f;

	/** The envelope hold time in miliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ForceUnits = ms))
	float HoldTime = 0.0f;

	/** The envelope decay time in miliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ForceUnits = ms))
	float DecayTime = 0.0f;

	/** The envelope sustain level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SustainLevel = 1.0f;

	/** The envelope release time in miliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ForceUnits = ms, SliderExponent = "5.0"))
	float ReleaseTime = 0.0f;

	/** The envelope attack curve strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayAfter = "AttackCurve"))
	float AttackCurveStrength = 1.0f;

	/** The envelope attack curve strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayAfter = "DecayCurve"))
	float DecayCurveStrength = 1.0f;

	/** The envelope attack curve strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", DisplayAfter = "ReleaseCurve"))
	float ReleaseCurveStrength = 1.0f;

	/** The envelope attack curve type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay)
	EAtomEnvelopeCurve AttackCurve = EAtomEnvelopeCurve::Linear;

	/** The envelope decay curve type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay)
	EAtomEnvelopeCurve DecayCurve = EAtomEnvelopeCurve::Linear;

	/** The envelope release curve type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Envelope, AdvancedDisplay)
	EAtomEnvelopeCurve ReleaseCurve = EAtomEnvelopeCurve::Linear;
};

