
#include "Atom/Gameplay/AtomGameplayRequirements.h"

#include "CriWareCorePrivate.h"

#define LOCTEXT_NAMESPACE "AtomGameplayRequirements"

UAtomRequirementPreset::UAtomRequirementPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

bool UAtomRequirementPreset::Matches(const FGameplayTagContainer& Tags) const
{
	return Query.IsEmpty() || Query.Matches(Tags);
}

FAtomGameplayRequirements::FAtomGameplayRequirements()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

bool FAtomGameplayRequirements::Matches(const FGameplayTagContainer& Tags) const
{
	if (Preset && !Preset->Matches(Tags))
	{
		return false;
	}

	if (!Custom.IsEmpty() && !Custom.Matches(Tags))
	{
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
