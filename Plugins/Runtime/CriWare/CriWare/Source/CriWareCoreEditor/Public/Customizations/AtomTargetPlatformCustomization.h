#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"

#include "Atom/AtomPluginUtilities.h"

class CRIWARECOREEDITOR_API FAtomPluginWidgetManager
{
public:

	FAtomPluginWidgetManager();
	~FAtomPluginWidgetManager();

	/* Builds out the Atom category for a specific Atom section for a platform settings page. */
	void BuildAtomCategory(IDetailLayoutBuilder& DetailLayout, const FString& PlatformName, const UStruct* ClassOuterMost = nullptr);

	/** Creates widget from a scan of loaded Atom plugins for an individual plugin type. */
	TSharedRef<SWidget> MakeAtomPluginSelectorWidget(const TSharedPtr<IPropertyHandle>& PropertyHandle, EAtomPlugin AtomPluginType, const FString& PlatformName);

private:

	/** Handles when a new plugin is selected. */
	static void OnPluginSelected(FString PluginName, TSharedPtr<IPropertyHandle> PropertyHandle);

	void OnPluginTextCommitted(const FText& InText, ETextCommit::Type CommitType, EAtomPlugin AtomPluginType, TSharedPtr<IPropertyHandle> PropertyHandle);

	FText OnGetPluginText(EAtomPlugin AtomPluginType);

private:

	/** Cached references to text for Source Data Override settings. */
	TSharedPtr<FText> SelectedSourceDataOverride;

	TSharedPtr<FText> ManualSourceDataOverrideEntry;
	
	TArray<TSharedPtr<FText>> SourceDataOverridePlugins;
};
