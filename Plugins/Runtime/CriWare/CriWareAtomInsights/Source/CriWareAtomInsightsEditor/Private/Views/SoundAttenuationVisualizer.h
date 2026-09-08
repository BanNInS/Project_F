
#pragma once 

#include "Math/Color.h"

#include "Atom/AtomAttenuation.h"

namespace Atom::Insights
{
	class IDashboardDataViewEntry;

	class FSoundAttenuationVisualizer
	{
	public:
		explicit FSoundAttenuationVisualizer(const FColor& InColor);
		void Draw(float InDeltaTime, const FTransform& InTransform, const UObject& InObject, const UWorld& InWorld) const;
		
		const FColor& GetColor() const { return Color; }

	private:
		const FColor Color { 155, 155, 255 };
		
		mutable TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails> ShapeDetailsMap;
		mutable uint32 LastObjectId = INDEX_NONE;
	};
} // namespace
