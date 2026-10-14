
#include "Atom/AtomVolume.h"

#include "CriWareCorePrivate.h"
#include "CriWareDefines.h"

#define LOCTEXT_NAMESPACE "AtomGameplayVolume"

FAtomInteriorSettings::FAtomInteriorSettings()
	: bIsWorldSettings(false)
	, ExteriorVolume(1.0f)
	, ExteriorTime(0.5f)
	, ExteriorLPF(ATOM_MAX_FILTER_FREQUENCY)
	, ExteriorLPFTime(0.5f)
	, InteriorVolume(1.0f)
	, InteriorTime(0.5f)
	, InteriorLPF(ATOM_MAX_FILTER_FREQUENCY)
	, InteriorLPFTime(0.5f)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

bool FAtomInteriorSettings::operator==(const FAtomInteriorSettings& Other) const
{
	return (Other.bIsWorldSettings == bIsWorldSettings)
		&& (Other.ExteriorVolume == ExteriorVolume) && (Other.ExteriorTime == ExteriorTime)
		&& (Other.ExteriorLPF == ExteriorLPF) && (Other.ExteriorLPFTime == ExteriorLPFTime)
		&& (Other.InteriorVolume == InteriorVolume) && (Other.InteriorTime == InteriorTime)
		&& (Other.InteriorLPF == InteriorLPF) && (Other.InteriorLPFTime == InteriorLPFTime);
}

bool FAtomInteriorSettings::operator!=(const FAtomInteriorSettings& Other) const
{
	return !(*this == Other);
}

#undef LOCTEXT_NAMESPACE
