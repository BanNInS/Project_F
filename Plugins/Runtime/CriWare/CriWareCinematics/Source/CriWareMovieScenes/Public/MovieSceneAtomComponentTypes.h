#pragma once

#include "Layout/Margin.h"
#include "Slate/WidgetTransform.h"
#include "Animation/WidgetMaterialTrackUtilities.h"
#include "EntitySystem/MovieSceneEntityIDs.h"
#include "EntitySystem/MovieScenePropertySystemTypes.h"
#include "EntitySystem/MovieScenePropertyTraits.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
#include "EntitySystem/MovieScenePropertyMetaData.h"
#else
#include "EntitySystem/MovieScenePropertyMetaDataTraits.h"
#endif

#include "MovieSceneAtomComponentTypes.generated.h"

// Forward Decalarations
class UMovieSceneAtomSection;

/** Component data for Atom tracks */
USTRUCT()
struct FMovieSceneAtomComponentData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMovieSceneAtomSection> Section = nullptr;
};

/**
 * Component data for Atom tracks inputs
 * This provides the names of the inputs whose values are stored on the
 * same entity using the DoubleResult, StringResult, BoolResult, and
 * IntegerResult components.
 */
USTRUCT()
struct FMovieSceneAtomInputData
{
	GENERATED_BODY()

	UPROPERTY()
	FName FloatInputs[9];

	UPROPERTY()
	FName StringInput;

	UPROPERTY()
	FName BoolInput;

	UPROPERTY()
	FName IntInput;
};

namespace UE::MovieScene
{
	struct FMovieSceneAtomComponentTypes
	{
	private:
		
		FMovieSceneAtomComponentTypes();
	
	public:

		CRIWAREMOVIESCENES_API ~FMovieSceneAtomComponentTypes();

		TComponentTypeID<FMovieSceneAtomComponentData> Atom;
		TComponentTypeID<FMovieSceneAtomInputData> AtomInputs;
		TComponentTypeID<FName> AtomTriggerName;

		static CRIWAREMOVIESCENES_API void Destroy();

		static CRIWAREMOVIESCENES_API FMovieSceneAtomComponentTypes* Get();
	};
} // namespace
