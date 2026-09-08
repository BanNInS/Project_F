
#pragma once

#include "GameFramework/WorldSettings.h"

#include "Atom/AtomVolume.h"

#include "AtomGameplayWorldSettings.generated.h"

// Forward Definitions
#if WITH_EDITOR
class FTransactionObjectEvent;
#endif

UCLASS(Config = Game, NotPlaceable)
class CRIWARECORE_API AAtomGameplayWorldSettings
	: public AWorldSettings
{
	GENERATED_BODY()

public:

	AAtomGameplayWorldSettings(const FObjectInitializer& ObjectInitializer);

	/** Default interior settings applied to sounds that have "apply ambient volumes" set to true on their AtomSoundCategory. */
	UPROPERTY(EditAnywhere, config, Category = Atom, DisplayName = DefaultAmbientZoneSettings)
	FAtomInteriorSettings DefaultAtomAmbientZoneSettings;

	/** Default DSP bus settings. */
	//UPROPERTY(EditAnywhere, Category = Atom)
	//TObjectPtr<UAtomDspBusSetting> DefaultDspBusSetting; // or rack ?? 

	//UPROPERTY(EditAnywhere, Category = Atom)
	//FName Snapshot;

public:

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;
#endif // WITH_EDITOR
	//~ End UObject Interface

	//~ Begin AActor Interface
	virtual void PostRegisterAllComponents() override;
	//~ End AActor Interface

private:

#if WITH_EDITOR
	void InternalPostPropertyChanged(FName PropertyName);
#endif
};
