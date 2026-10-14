
#pragma once

#include "GameFramework/Volume.h"
#include "UObject/Class.h"
#include "UObject/ObjectMacros.h"

#include "Atom/Atom.h"

#include "AtomGameplayVolume.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAtomGameplayVolumeEvent);

// Forward Declarations 
class UAtomGameplayVolumeComponent;
class UAtomGameplayVolumeSubsystem;

/**
 * AtomGameplayVolume - A spatial volume used to notify Atom gameplay systems when the nearest Atom audio listener
   enters or exits the volume. Additionally, these volumes can influence Atom sources depending on the relative
   position of the listener.

   NOTE: Will only impact Atom sound sources that have "apply ambient volumes" set on their Atom sound category.
 */
UCLASS(hidecategories = (Advanced, Attachment, Collision, Volume))
class CRIWARECORE_API AAtomGameplayVolume
	: public AVolume
{
	GENERATED_BODY()

public:

	AAtomGameplayVolume(const FObjectInitializer& ObjectInitializer);

private:

	// A representation of this volume for the audio thread
	UPROPERTY(Transient)
	TObjectPtr<UAtomGameplayVolumeComponent> AGVComponent = nullptr;

	// Whether this volume is currently enabled.  Disabled volumes will not have a volume proxy, 
	// and therefore will not be considered for intersection checks.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_bEnabled, Category = "AtomGameplay", Meta = (AllowPrivateAccess = "true"))
	bool bEnabled = true;

public:

	bool GetEnabled() const { return bEnabled; }
	
	/** Sets whether the volume is enabled */
	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetEnabled(bool bEnable);

	/** Blueprint event for listener enter */
	UFUNCTION(BlueprintNativeEvent, Category = Events)
	void OnListenerEnter();

	/** Blueprint event for listener exit */
	UFUNCTION(BlueprintNativeEvent, Category = Events)
	void OnListenerExit();

	UPROPERTY(BlueprintAssignable, Category = Events)
	FAtomGameplayVolumeEvent OnListenerEnterEvent;

	UPROPERTY(BlueprintAssignable, Category = Events)
	FAtomGameplayVolumeEvent OnListenerExitEvent;

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldCheckCollisionComponentForErrors() const override { return false; }
#endif // WITH_EDITOR
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface

	//~ Begin AActor Interface
	virtual void PostInitializeComponents() override;
	virtual void PostRegisterAllComponents() override;
	virtual void PostUnregisterAllComponents() override;
	//~ End AActor Interface

	/** Called by a child component to notify our proxy may need updating */
	void OnComponentDataChanged();

	bool CanSupportProxy() const;

protected:

	UFUNCTION()
	virtual void OnRep_bEnabled();

	void TransformUpdated(USceneComponent* InRootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	void AddProxy() const;
	void RemoveProxy() const;
	void UpdateProxy() const;
};
