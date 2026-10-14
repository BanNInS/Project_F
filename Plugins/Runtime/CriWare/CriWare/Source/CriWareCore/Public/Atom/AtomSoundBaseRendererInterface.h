
#pragma once

#include "Engine/EngineTypes.h"
#include "UObject/Interface.h"

#include "AtomSoundBaseRendererInterface.generated.h"

// Forward Declarations
class UAtomSoundBase;

/**
 * Interface for an object that can snapshot an Atom sound to a wave data array.
 */
UINTERFACE(MinimalAPI)
class UAtomSoundBaseRendererInterface : public UInterface
{
	GENERATED_BODY()
};

class CRIWARECORE_API IAtomSoundBaseRendererInterface
{
	GENERATED_BODY()

public:
	
	/**
	 * Open the Atom sound to get a wave data snapshot for.
	 *
	 * @param  InAtomSound  Atom sound to play.
	 * @param  InNumFrames  The number of sound frame to snapshot.
	 * @return	ByteBulkData that will hold the wave data.
	 */
	virtual FByteBulkData& Open(UAtomSoundBase* InAtomSound, int InNumFrames) = 0;
	virtual FByteBulkData& Open(UAtomSoundBase* InAtomSound) = 0;
};
