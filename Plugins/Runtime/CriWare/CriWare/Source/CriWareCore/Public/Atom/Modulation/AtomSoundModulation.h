#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "AtomSoundModulation.generated.h"

// Forward Definitions
class UAtomModulatorBase;

/** Structure allowing modulation override for a sound parameter. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomSoundModulationSettings
{
	GENERATED_BODY()

public:

	/** The base value to assign to the destination control if modulable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value Modulation")
	float Value = 1.0f;

#if WITH_EDITORONLY_DATA
	/** Base value of parameter */
	UPROPERTY(EditAnywhere, Category = Modulation, meta = (DisplayName = "Modulate"))
	bool bEnableModulation = false;
#endif // WITH_EDITORONLY_DATA

	/** Modulation source, which provides value to mix with base value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	TSet<TObjectPtr<UAtomModulatorBase>> Modulators;
};
