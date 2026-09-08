
#pragma once

#include "Atom/AtomActiveSound.h"
#include "Atom/AtomEngineSubsystem.h"
#include "Atom/Interfaces/IAtomActiveSoundUpdate.h"
#include "Atom/Interfaces/IAtomSoundHandleSystem.h"

#include "AtomSoundHandleSubsystem.generated.h"

class IAtomSoundHandleOwner;

/**
 * An implementation of IAtomSoundHandleSystem using AtomEngineSubsystem
 */
UCLASS()
class UAtomSoundHandleSubsystem : public UAtomEngineSubsystem
	, public IAtomActiveSoundUpdate
	, public IAtomSoundHandleSystem
{
	GENERATED_BODY()

public:
	virtual ~UAtomSoundHandleSubsystem() override = default;

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface
	
	//~ Begin IActiveSoundUpdateInterface
	virtual void NotifyActiveSoundDeleting(const FAtomActiveSound& ActiveSound) override;
	virtual void NotifyVirtualizedSoundDeleting(const FAtomActiveSound& ActiveSound) override;
	//~ End IActiveSoundUpdateInterface

	//~ Begin IAtomSoundHandleSystem
	virtual Atom::FSoundHandleID CreateSoundHandle(UAtomSoundBase* Sound, IAtomSoundHandleOwner* Owner) override;
	virtual void SetTransform(Atom::FSoundHandleID ID, const FTransform& Transform) override;
	virtual Atom::EResult Play(Atom::FSoundHandleID ID) override;
	virtual void Stop(Atom::FSoundHandleID ID) override;
	//~ End IAtomSoundHandleSystem

protected:
	struct FSoundHandle
	{
		FAtomActiveSound ActiveSound;
	};
	
	TMap<Atom::FSoundHandleID, FSoundHandle> ActiveHandles;
	TMap<Atom::FSoundHandleID, IAtomSoundHandleOwner*> Owners;

	void NotifySoundDeleting(const FAtomActiveSound& ActiveSound);
};
