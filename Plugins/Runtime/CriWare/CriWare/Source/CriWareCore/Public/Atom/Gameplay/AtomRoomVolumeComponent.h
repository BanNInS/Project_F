
#pragma once

#include "CoreMinimal.h"
#include "Atom/Gameplay/AtomGameplayVolumeMutator.h"

#include "AtomRoomVolumeComponent.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EAtomPortailingMethod : uint8
{
	ClosestPortal, // if sound is audible, moves sound to the closest audible p to the listener
	MultiplePortals // if sound is audible, use multipositing to 'split' the sound to all portals in the listener audible range.
};

/**
 *  FAtomProxyMutator_Room - An Atom thread representation of Room settings
 */
class FAtomProxyMutator_Room
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_Room();
	virtual ~FAtomProxyMutator_Room() = default;

	EAtomPortailingMethod PortailingMethod = EAtomPortailingMethod::ClosestPortal;
	bool bUseOvelappingPortals = false;
	FName ComponentTag = NAME_None;

	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	constexpr static const TCHAR MutatorRoomName[] = TEXT("Room");
};

/**
 * 
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "Room"))
class CRIWARECORE_API UAtomRoomVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomRoomVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomRoomVolumeComponent() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetPortailingMethod(EAtomPortailingMethod PortailingMethod);

	EAtomPortailingMethod GetPortailingMethod() const { return PortailingMethod; }

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void UseOvelappingPortals(bool bUseOvelappingPortals);

	bool IsUsingOvelappingPortals() const { return bUseOvelappingPortals; }

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetComponenetTag(FName ComponentTag);

	FName getComponentTag() const { return ComponentTag; }

private:

	//~ Begin UAtomGameplayVolumeMutator interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator interface

	TMap<uint32, FTransform> OriginalTransforms;

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portailing", meta = (AllowPrivateAccess = "true"))
	EAtomPortailingMethod PortailingMethod = EAtomPortailingMethod::ClosestPortal;

	/** Whether to use AGVC portals overlapping this Room AVGC. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portailing", meta = (AllowPrivateAccess = "true"))
	bool bUseOvelappingPortals = false;

	/** The component tag to use if a custom component is desired as portal shape.
		Use to look up the component on the source actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portailing|Custom Component", meta = (AllowPrivateAccess = "true"))
	FName ComponentTag = NAME_None;
};
