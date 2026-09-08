
#include "CriWareAtomInsightsEditorSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CriWareAtomInsightsEditorSettings)

#define LOCTEXT_NAMESPACE "UCriWareAtomInsightsEditorSettings"

FName UCriWareAtomInsightsEditorSettings::GetCategoryName() const
{
	return "Plugins";
}

FText UCriWareAtomInsightsEditorSettings::GetSectionText() const
{
	return LOCTEXT("CriWareAtomInsightsEditorSettings_SectionText", "ADX Atom Insights");
}

FText UCriWareAtomInsightsEditorSettings::GetSectionDescription() const
{
	return LOCTEXT("CriWareAtomInsightsEditorSettings_SectionDesc", "Configure ADX Atom Insights editor settings.");
}

#undef LOCTEXT_NAMESPACE
