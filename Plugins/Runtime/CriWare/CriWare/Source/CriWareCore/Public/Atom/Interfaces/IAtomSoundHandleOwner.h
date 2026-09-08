
#pragma once

#include "Atom/Atom.h"
#include "Atom/Interfaces/IAtomSoundHandleSystem.h"

/**
 * An interface to be used for any classes that we want to respond to SoundHandle updates
 * TODO: Add a child of this that uses World?
 */
class IAtomSoundHandleOwner
{
public:
	virtual ~IAtomSoundHandleOwner() = default;
	
	Atom::EResult CreateSoundHandle(UAtomSoundBase* Sound, Atom::FSoundHandleID& OutSoundHandleID)
	{
		if(Sound)
		{
			for(IAtomSoundHandleSystem* Interface : IAtomSoundHandleSystem::GetRegisteredInterfaces())
			{
				OutSoundHandleID = Interface->CreateSoundHandle(Sound, this);
				return Atom::EResult::Success;
			}
		}
		else
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Couldn't create Sound Handle because we didn't pass through a valid sound!"));
		}
		
		UE_LOG(LogCriWareAtom, Warning, TEXT("Couldn't create Sound Handle because we couldn't find any ISoundHandleSystems!"));
		return Atom::EResult::Failure;
	}
	
	/**
	 * Called when the sound handle is being removed from a ISoundHandle implementer,
	 * i.e. when the active sound it handles is notified for delete
	 */
	virtual void OnSoundHandleRemoved() {}
};
