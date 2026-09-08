
#pragma once 

#include "HAL/Platform.h"
#include "Styling/SlateTypes.h"

namespace Atom::Insights
{
	class FMuteSoloFilter
	{
	public:
		FMuteSoloFilter();
		~FMuteSoloFilter();
		
	private:
		void FilterMuteSolo(ECheckBoxState InMuteState, ECheckBoxState InSoloState, const FString& InCurrentFilterString) const;
	};
} // namespace
