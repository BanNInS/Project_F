
#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "HAL/Platform.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/Interface.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "Atom/AtomAisacPatch.h"
#include "Extensions/IAtomProxyInitializer.h"

#include "IAtomAisacParameterController.generated.h"

class UObject;
struct FFrame;

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint), MinimalAPI)
class UAtomAisacParameterController : public UInterface
{
	GENERATED_BODY()
};

// Base interface for any object implementing parameter control for a given sound instance controller.
class IAtomAisacParameterController : public IInterface
{
	GENERATED_BODY()

public:
	// Resets all parameters to their original values.
	UFUNCTION(BlueprintCallable, Category = "Atom|AISAC")
	virtual void ResetParameters() = 0;

	// Executes a named trigger.  Does *not* cache trigger value, so only executes if the sound
	// is already playing.  If the intent is for the trigger to execute immediately (if playing)
	// and be called on initialization for all future instances, call 'SetBoolParameter' with the
	// intended initial trigger behavior (true if trigger desired on initialization, false if not).
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Trigger Parameter"), Category = "Atom|AISAC")
	virtual void SetTriggerParameter(const FAtomAisacControl& InControl) = 0;

	// Sets a named Float
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Aisac Parameter"), Category = "Atom|AISAC")
	virtual void SetAisacParameter(const FAtomAisacControl& InControl, float InValue) = 0;

	// Sets an array of parameters as a batch
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Parameters"), Category = "Atom|AISAC")
	virtual void SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InParameters) = 0;

	// Sets a named parameter to the given parameter structure value
	virtual void SetParameter(FAtomAisacParameter&& InValue) = 0;

	// Sets an array of parameters as a batch
	virtual void SetParameters(TArray<FAtomAisacParameter>&& InValues) = 0;
};
