
#include "MixerSourceMuteSoloFilter.h"

#include "Atom/AtomDebug.h"
#include "Atom/AtomRuntimeManager.h"

#include "Views/MixerSourceDashboardViewFactory.h"

namespace Atom::Insights
{
	FMuteSoloFilter::FMuteSoloFilter()
	{
		FMixerSourceDashboardViewFactory::OnUpdateMuteSoloState.AddRaw(this, &FMuteSoloFilter::FilterMuteSolo);
	}

	FMuteSoloFilter::~FMuteSoloFilter()
	{
		FMixerSourceDashboardViewFactory::OnUpdateMuteSoloState.RemoveAll(this);
	}

	void FMuteSoloFilter::FilterMuteSolo(ECheckBoxState InMuteState, ECheckBoxState InSoloState, const FString& InCurrentFilterString) const
	{
#if ENABLE_AUDIO_DEBUG
		if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
		{
			::Atom::FAtomDebugger& AtomDebugger = AtomRuntimeManager->GetDebugger();

			FName CurrentFilterStringName{ InCurrentFilterString };
			
			if (InMuteState == ECheckBoxState::Checked && !InCurrentFilterString.IsEmpty())
			{
				AtomDebugger.ToggleMuteSoundWave(CurrentFilterStringName, true);
			}
			else
			{
				AtomDebugger.ToggleMuteSoundWave(NAME_None, true);
			}

			if (InSoloState == ECheckBoxState::Checked && !InCurrentFilterString.IsEmpty())
			{
				AtomDebugger.ToggleSoloSoundWave(CurrentFilterStringName, true);
			}
			else
			{
				AtomDebugger.ToggleSoloSoundWave(NAME_None, true);
			}
		}
#endif // ENABLE_AUDIO_DEBUG
	}
} // namespace UE::Audio::Insights
