
#pragma once

#include "AtomGameplayComponent.h"
#include "Interfaces/IAtomGameplayVolumeInteraction.h"

#include "AtomGameplayVolumeComponent.generated.h"

// Forward Declarations 
class UAtomGameplayVolumeProxy;
class UAtomGameplayVolumeSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAtomGameplayVolumeProxyStateChange);

/**
 *  UAtomGameplayVolumeComponent - Component used to drive interaction with AtomGameplayVolumeSubsystem.
 *   NOTE: Do not inherit from this class, use UAtomGameplayVolumeComponentBase or UAtomGameplayVolumeMutator to create extendable functionality
 */
UCLASS(Config = Game, ClassGroup = ("AtomGameplay"), meta = (BlueprintSpawnableComponent, IsBlueprintBase = false, DisplayName = "Volume Proxy"))
class CRIWARECORE_API UAtomGameplayVolumeComponent final
	: public UAtomGameplayComponent
{
	GENERATED_BODY()

public:

	UAtomGameplayVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumeComponent() = default;

	void SetProxy(UAtomGameplayVolumeProxy* NewProxy);
	UAtomGameplayVolumeProxy* GetProxy() const { return Proxy; }

	/** Called by a component on same actor to notify our proxy may need updating */
	void OnComponentDataChanged();

	/** Called when the proxy is 'entered' - This is when the proxy goes from zero listeners to at least one. */
	void EnterProxy() const;

	/** Called when the proxy is 'exited' - This is when the proxy goes from at least one listeners to zero. */
	void ExitProxy() const;

	/** Blueprint event for proxy enter */
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnAtomGameplayVolumeProxyStateChange OnProxyEnter;

	/** Blueprint event for proxy exit */
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnAtomGameplayVolumeProxyStateChange OnProxyExit;

protected:

	// A representation of this volume for the audio thread
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = "AtomGameplay", Meta = (ShowOnlyInnerProperties, AllowPrivateAccess = "true"))
	TObjectPtr<UAtomGameplayVolumeProxy> Proxy = nullptr;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	//~ Begin UActorComponent Interface
	virtual void OnUnregister() override;
	//~ End UActorComponent Interface

	//~ Begin UAtomGameplayComponent Interface
	virtual void Enable() override;
	virtual void Disable() override;
	//~ End UAtomGameplayComponent Interface

	void AddProxy() const;
	void RemoveProxy() const;
	void UpdateProxy() const;

	UAtomGameplayVolumeSubsystem* GetSubsystem() const;
};

/**
 *  UAtomGameplayVolumeComponentBase - Blueprintable component used to craft custom functionality with AtomGameplayVolumes.
 *  NOTE: Inherit from this class to get easy access to OnListenerEnter and OnListenerExit Blueprint Events
 */
UCLASS(Blueprintable, ClassGroup = ("AtomGameplay"), hidecategories = (Tags, Collision), meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomGameplayVolumeComponentBase
	: public UAtomGameplayComponent
	, public IAtomGameplayVolumeInteraction
{
	GENERATED_BODY()

public:

	UAtomGameplayVolumeComponentBase(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumeComponentBase() = default;
};
