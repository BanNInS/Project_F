
#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Extensions/IAtomAisacParameterController.h"

#include "IAtomSoundAisacParameterController.generated.h"


// Forward Declarations
class FAtomRuntime;
class UAtomSoundBase;
struct FAtomActiveSound;

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint), MinimalAPI)
class UAtomSoundAisacParameterController : public UAtomAisacParameterController
{
	GENERATED_BODY()

public:
	CRIWARECORE_API UAtomSoundAisacParameterController(FObjectInitializer const& InObjectInitializer);
};

// UObject interface for all object types that are controlling parameter values sent
// to sound instances (i.e. sources)
class IAtomSoundAisacParameterController : public IAtomAisacParameterController
{
	GENERATED_BODY()

public:
	// IAtomAisacParameterController
	CRIWARECORE_API void ResetParameters() override;

	CRIWARECORE_API void SetTriggerParameter(const FAtomAisacControl& InControl) override;
	CRIWARECORE_API void SetAisacParameter(const FAtomAisacControl& InControl, float InFloat) override;
	
	CRIWARECORE_API void SetParameter(FAtomAisacParameter&& InValue) override;
	CRIWARECORE_API void SetParameters(TArray<FAtomAisacParameter>&& InValues) override;
	CRIWARECORE_API void SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InValues) override;

	/** Returns the active audio device to use for this component based on whether or not the component is playing in a world. */
	virtual FAtomRuntime* GetAtomRuntime() const = 0;

	/** Returns the id of the sound owner's instance associated with the parameter interface. */
	virtual uint64 GetInstanceOwnerID() const = 0;

	/** Returns the Game Thread copy of parameters to modify in place. */
	virtual TArray<FAtomAisacParameter>& GetInstanceParameters() = 0;

	/** Returns the UAtomSoundBase used to initialize instance parameters to update. */
	virtual UAtomSoundBase* GetSound() = 0;

	virtual bool IsPlaying() const = 0;

	virtual bool GetDisableParameterUpdatesWhilePlaying() const = 0;
};
