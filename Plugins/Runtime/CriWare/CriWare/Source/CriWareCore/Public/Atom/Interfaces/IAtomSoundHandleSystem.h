
#pragma once

#include "Features/IModularFeatures.h"

class IAtomSoundHandleOwner;
class UAtomSoundBase;

namespace Atom
{
	using FSoundHandleID = uint32;

	enum class EResult : uint8
	{
		Success = 0,
		Failure = 1,
		NotImplemented = 2
	};
}

/**
 * This interface should be used with systems aiming to create Sound Handles: gameplay thread representations of sounds
 */
class IAtomSoundHandleSystem : public IModularFeature
{
public:
	/**	
	 * Get the name of all Sound Handle implementations in the Modular Features registry.
	 * @return "AtomSoundHandles"
	 */
	static FName GetModularFeatureName()
	{
		static FName FeatureName = FName(TEXT("AtomSoundHandles"));
		return FeatureName;
	}

	virtual ~IAtomSoundHandleSystem() = default;

	/**
	 * @brief This should create a sound and we should map it to an identifier.
	 * Then return the identifier to the owner to keep track of
	 * 
	 * @param Sound
	 * @param Owner 
	 * @return 
	 */
	virtual Atom::FSoundHandleID CreateSoundHandle(UAtomSoundBase* Sound, IAtomSoundHandleOwner* Owner) { return INDEX_NONE; }

	/**
	 * @brief Set the transform on the sound that is represented by the sound handle with the following ID
	 * 
	 * @param ID 
	 * @param Transform 
	 */
	virtual void SetTransform(Atom::FSoundHandleID ID, const FTransform& Transform) {}

	/**
	 * @brief Play the sound that is represented by the sound handle with the following ID
	 *
	 * @param ID 
	 */
	virtual Atom::EResult Play(Atom::FSoundHandleID ID) { return Atom::EResult::NotImplemented; }

	/**
	 * @brief Stop the sound that is represented by the sound handle with the following ID
	 *
	 * @param ID 
	 */
	virtual void Stop(Atom::FSoundHandleID ID) {}

	static TArray<IAtomSoundHandleSystem*> GetRegisteredInterfaces()
	{
		// Get all IAudioHandleInterface implementations
		IModularFeatures::Get().LockModularFeatureList();
		TArray<IAtomSoundHandleSystem*> RegisteredInterfaces = IModularFeatures::Get().GetModularFeatureImplementations<IAtomSoundHandleSystem>(GetModularFeatureName());
		IModularFeatures::Get().UnlockModularFeatureList();
		return RegisteredInterfaces;
	}
};

