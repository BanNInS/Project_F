
#pragma once

#include "UObject/Interface.h"

#include "IAtomGameplayVolumeInteraction.generated.h"

/** Interface for interacting with the Atom gameplay volume system */
UINTERFACE(BlueprintType, MinimalAPI)
class UAtomGameplayVolumeInteraction : public UInterface
{
	GENERATED_BODY()
};

class IAtomGameplayVolumeInteraction
{
	GENERATED_BODY()

public:

	/**
	 * Called when a listener 'enters' the associated proxy
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "AtomGameplayVolume")
	CRIWARECORE_API void OnListenerEnter();
	virtual void OnListenerEnter_Implementation() {}

	/**
	 * Called when a listener 'exits' the associated proxy
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "AtomGameplayVolume")
	CRIWARECORE_API void OnListenerExit();
	virtual void OnListenerExit_Implementation() {}
};
