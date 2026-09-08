
#include "MovieSceneAtomComponentTypes.h"

#include "Animation/MovieSceneUMGComponentTypes.h"
#include "Animation/WidgetMaterialTrackUtilities.h"
#include "EntitySystem/BuiltInComponentTypes.h"
#include "EntitySystem/MovieSceneComponentRegistry.h"
#include "EntitySystem/MovieSceneEntityFactoryTemplates.h"
#include "EntitySystem/MovieScenePropertyComponentHandler.h"
#include "MovieSceneTracksComponentTypes.h"
#include "Systems/MovieScenePiecewiseDoubleBlenderSystem.h"

namespace UE::MovieScene
{
	static bool GMovieSceneAtomComponentTypesDestroyed = false;
	static TUniquePtr<FMovieSceneAtomComponentTypes> GMovieSceneAtomComponentTypes;

	FMovieSceneAtomComponentTypes::FMovieSceneAtomComponentTypes()
	{
		FBuiltInComponentTypes* BuiltInComponents = FBuiltInComponentTypes::Get();
		FComponentRegistry* ComponentRegistry = UMovieSceneEntitySystemLinker::GetComponents();

		ComponentRegistry->NewComponentType(&Atom, TEXT("Atom"), EComponentTypeFlags::CopyToChildren);
		ComponentRegistry->NewComponentType(&AtomInputs, TEXT("Atom Inputs"), EComponentTypeFlags::CopyToChildren);
		ComponentRegistry->NewComponentType(&AtomTriggerName, TEXT("Atom Trigger Name"), EComponentTypeFlags::CopyToChildren);
	}

	FMovieSceneAtomComponentTypes::~FMovieSceneAtomComponentTypes()
	{
	}

	void FMovieSceneAtomComponentTypes::Destroy()
	{
		GMovieSceneAtomComponentTypes.Reset();
		GMovieSceneAtomComponentTypesDestroyed = true;
	}

	FMovieSceneAtomComponentTypes* FMovieSceneAtomComponentTypes::Get()
	{
		if (!GMovieSceneAtomComponentTypes.IsValid())
		{
			check(!GMovieSceneAtomComponentTypesDestroyed);
			GMovieSceneAtomComponentTypes.Reset(new FMovieSceneAtomComponentTypes);
		}
		return GMovieSceneAtomComponentTypes.Get();
	}

} // namespace
