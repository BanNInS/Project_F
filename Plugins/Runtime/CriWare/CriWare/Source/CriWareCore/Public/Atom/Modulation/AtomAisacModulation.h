#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "Atom/AtomAisacPatch.h"
#include "Atom/Modulation/AtomSoundModulation.h"

#include "AtomAisacModulation.generated.h"

// Forward Definitions
class UAtomModulatorBase;

/** Structure allowing modulation AISAC control override. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAisacControlSettings 
	: public FAtomSoundModulationSettings
{
	GENERATED_BODY()

public:

	/** The destination AISAC control to modulate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AISAC)
	FAtomAisacControl Control;
};

/** Structure allowing AISAC setup. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAisacModulationDefaultSettings
{
	GENERATED_BODY()

public:

	/** List of available AISAC patches for this sound. Can be from Cue or Categories. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AISAC, DisplayName = "Patches")
	TArray<TObjectPtr<UAtomAisacPatch>> AisacPatches;

	/** List of additional global AISAC patches from current AtomConfig to attach to this sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AISAC, DisplayName = "Additional Patches")
	TArray<TObjectPtr<UAtomAisacPatch>> AdditionalAisacPatches;

	/** List of controls to modulate for this sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AISAC, DisplayName = "Control Modulations", meta = (AtomParam = "AISAC", AtomParamClass = "AtomModulationParameter"))
	TArray<FAtomAisacControlSettings> AisacControlModulations;
};


