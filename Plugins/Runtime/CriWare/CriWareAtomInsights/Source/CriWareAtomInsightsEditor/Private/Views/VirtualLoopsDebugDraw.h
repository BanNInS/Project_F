
#pragma once 

#include "CriWareDefines.h"
#include "SoundAttenuationVisualizer.h"

namespace Atom::Insights
{
	class IDashboardDataViewEntry;

	class FVirtualLoopsDebugDraw
	{
	public:
		FVirtualLoopsDebugDraw();
		~FVirtualLoopsDebugDraw();

		void DebugDraw(float InElapsed, const TArray<TSharedPtr<IDashboardDataViewEntry>>& InSelectedItems, FAtomRuntimeId InRumtimeID);

	private:
		void DebugDrawEntries(float InElapsed, const TArray<TSharedPtr<IDashboardDataViewEntry>>& InSelectedItems, FAtomRuntimeId InRumtimeID) const;

		FSoundAttenuationVisualizer AttenuationVisualizer;
	};
} // namespace
