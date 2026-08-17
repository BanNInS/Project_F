
#include "Customizations/AtomTargetPlatformCustomization.h"

#include "EditorDirectories.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "IDetailChildrenBuilder.h"
#include "UObject/UnrealType.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/SListView.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Features/IModularFeatures.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/SToolTip.h"

#if WITH_ENGINE
#include "Atom/AtomRuntime.h"
#endif 

#define LOCTEXT_NAMESPACE "AtomTargetPlatform"

// This string is used for the item on the combo box that, when selected, defers to the custom string entry.
static const TCHAR* ManualEntryItem = TEXT("Other");

FAtomPluginWidgetManager::FAtomPluginWidgetManager()
{
	ManualSourceDataOverrideEntry = TSharedPtr<FText>(new FText(FText::FromString(TEXT("None"))));
}

FAtomPluginWidgetManager::~FAtomPluginWidgetManager()
{
}

TSharedRef<SWidget> FAtomPluginWidgetManager::MakeAtomPluginSelectorWidget(const TSharedPtr<IPropertyHandle>& PropertyHandle, EAtomPlugin AtomPluginType, const FString& PlatformName)
{
	TArray<TSharedPtr<FText>>* ValidPluginNames = nullptr;
	FText TooltipText;
	TSharedPtr<FText> DefaultEffectName;

	switch (AtomPluginType)
	{
	case EAtomPlugin::SourceDataOverride:
		ValidPluginNames = &SourceDataOverridePlugins;
		TooltipText = LOCTEXT("SourceDataOverride", "Choose which Atom plugin should be used for source data override. If your desired source data override plugin isn't found in the drop down menu, ensure that it is enabled on the Plugins panel.");
		DefaultEffectName = TSharedPtr<FText>(new FText(FText::FromString(TEXT("None"))));
		SelectedSourceDataOverride = TSharedPtr<FText>(new FText(*DefaultEffectName));
		PropertyHandle->GetValueAsDisplayText(*SelectedSourceDataOverride);
		break;

	default:
		checkf(false, TEXT("Invalid plugin enumeration type. Need to add a handle for that case here."));
		break;
	}

	ValidPluginNames->Add(DefaultEffectName);


#if WITH_ENGINE
	// Scan through all currently enabled Atom plugins of this specific type:
	switch (AtomPluginType)
	{
	case EAtomPlugin::SourceDataOverride:
	{
		IModularFeatures::Get().LockModularFeatureList();
		TArray<IAtomSourceDataOverrideFactory*> AvailableSourceDataOverridePlugins = IModularFeatures::Get().GetModularFeatureImplementations<IAtomSourceDataOverrideFactory>(IAtomSourceDataOverrideFactory::GetModularFeatureName());
		IModularFeatures::Get().UnlockModularFeatureList();

		for (IAtomSourceDataOverrideFactory* Plugin : AvailableSourceDataOverridePlugins)
		{
			if (Plugin->SupportsPlatform(PlatformName))
			{
				ValidPluginNames->Add(TSharedPtr<FText>(new FText(FText::FromString(Plugin->GetDisplayName()))));
			}
		}
	}
	break;

	// note: if adding cases here, make sure to lock the Modular Feature List after retrieving the implementations
	//		IModularFeatures::Get().LockModularFeatureList(); & IModularFeatures::Get().UnlockModularFeatureList();

	default:
		IModularFeatures::Get().UnlockModularFeatureList();
		break;
	}
#endif // #if WITH_ENGINE

	// This pointer is used to store whatever custom string was input by the user or retrieved from the config file.
	ValidPluginNames->Add(TSharedPtr<FText>(new FText(FText::FromString(ManualEntryItem))));

	// Text box component:
	TSharedRef<SEditableTextBox> EditableTextBox = SNew(SEditableTextBox)
		.Text_Lambda([this, AtomPluginType]() { return OnGetPluginText(AtomPluginType); })
		.OnTextCommitted_Raw(this, &FAtomPluginWidgetManager::OnPluginTextCommitted, AtomPluginType, PropertyHandle)
		.SelectAllTextWhenFocused(true)
		.RevertTextOnEscape(true);

	// Combo box component:
	TSharedRef<SWidget> ComboBox = SNew(SListView<TSharedPtr<FText>>)
		.ListItemsSource(ValidPluginNames)
		.ScrollbarVisibility(EVisibility::Collapsed)
		.OnGenerateRow_Lambda([](TSharedPtr<FText> InItem, const TSharedRef< class STableViewBase >& Owner)
		{
			return SNew(STableRow<TSharedPtr<FText>>, Owner)
				.Padding(FMargin(16, 4, 16, 4))
				[
					SNew(STextBlock).Text(*InItem)
				];
		})
		.OnSelectionChanged_Lambda([this, AtomPluginType, PropertyHandle](TSharedPtr<FText> InText, ESelectInfo::Type)
		{
			const bool bSelectedManualEntry = (InText->ToString() == FString(ManualEntryItem));

			switch (AtomPluginType)
			{
			case EAtomPlugin::SourceDataOverride:
				if (bSelectedManualEntry)
				{
					SelectedSourceDataOverride = ManualSourceDataOverrideEntry;
				}
				else
				{
					SelectedSourceDataOverride = InText;
				}

				OnPluginSelected((bSelectedManualEntry ? ManualSourceDataOverrideEntry->ToString() : InText->ToString()), PropertyHandle);
				break;

			default:
				break;
			}
		});

	//Generate widget:
	const TSharedRef<SWidget> NewWidget = SNew(SComboButton)
		.ContentPadding(FMargin(0, 0, 5, 0))
		.ToolTipText(TooltipText)
		.ButtonContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			.Padding(FMargin(0, 0, 5, 0))
			[
				EditableTextBox
			]
		]
		.MenuContent()
		[
			ComboBox
		];

	return NewWidget;
}

void FAtomPluginWidgetManager::BuildAtomCategory(IDetailLayoutBuilder& DetailLayout, const FString& PlatformName, const UStruct* ClassOuterMost)
{
	IDetailCategoryBuilder& AtomCategory = DetailLayout.EditCategory(TEXT("Atom"));

	TSharedPtr<IPropertyHandle> AtomPropagationPropertyHandle = DetailLayout.GetProperty("AtomSourceDataOverridePlugin", ClassOuterMost);
	IDetailPropertyRow& AtomPropagationPropertyRow = AtomCategory.AddProperty(AtomPropagationPropertyHandle);

	AtomPropagationPropertyRow.CustomWidget()
		.NameContent()
		[
			AtomPropagationPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(500.0f)
		.MinDesiredWidth(100.0f)
		[
			MakeAtomPluginSelectorWidget(AtomPropagationPropertyHandle, EAtomPlugin::SourceDataOverride, PlatformName)
		];

	// Not really a plugin, but this is common to all TargetPlatforms
	/*TSharedPtr<IPropertyHandle> SoundQualityNamePropHandle = DetailLayout.GetProperty("SoundCueCookQualityIndex", ClassOuterMost);
	ensure(SoundQualityNamePropHandle.IsValid());
	IDetailPropertyRow& SoundQualityNamePropRow = AtomCategory.AddProperty(SoundQualityNamePropHandle);

	SoundQualityNamePropRow.CustomWidget()
		.NameContent()
		[
			SoundQualityNamePropHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MaxDesiredWidth(500.0f)
		.MinDesiredWidth(100.0f)
		[
			PropertyCustomizationHelpers::MakePropertyComboBox(
				SoundQualityNamePropHandle,
				FOnGetPropertyComboBoxStrings::CreateLambda([](TArray<TSharedPtr<FString>>& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems) -> void
					{
						for (const FAudioQualitySettings& i : GetDefault<UAudioSettings>()->QualityLevels)
						{
							OutComboBoxStrings.Add(MakeShared<FString>(i.DisplayName.ToString()));
							OutRestrictedItems.Add(false);
							OutToolTips.Add(SNew(SToolTip).Text(i.DisplayName));
						}
					}),
				FOnGetPropertyComboBoxValue::CreateLambda([SoundQualityNamePropHandle]() -> FString
					{
						int32 IndexVal = INDEX_NONE;
					SoundQualityNamePropHandle->GetValue(IndexVal);
					return GetDefault<UAudioSettings>()->FindQualityNameByIndex(IndexVal);
					}),
						FOnPropertyComboBoxValueSelected::CreateLambda([SoundQualityNamePropHandle](FString Value) -> void
							{
								const TArray<FAudioQualitySettings>& Values = GetDefault<UAudioSettings>()->QualityLevels;
					int32 Index = Values.IndexOfByPredicate([TextValue = FText::FromString(Value)](const FAudioQualitySettings& i) -> bool { return i.DisplayName.CompareTo(TextValue) == 0; });
					SoundQualityNamePropHandle->SetValue(Index);
							})
						)
		];*/
}

void FAtomPluginWidgetManager::OnPluginSelected(FString PluginName, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	PropertyHandle->SetValue(PluginName);
}

void FAtomPluginWidgetManager::OnPluginTextCommitted(const FText& InText, ETextCommit::Type CommitType, EAtomPlugin AtomPluginType, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	switch (AtomPluginType)
	{
	case EAtomPlugin::SourceDataOverride:
		*ManualSourceDataOverrideEntry = InText;
		SelectedSourceDataOverride = ManualSourceDataOverrideEntry;
		break;

	default:
		break;
	}

	OnPluginSelected(InText.ToString(), PropertyHandle);
}

FText FAtomPluginWidgetManager::OnGetPluginText(EAtomPlugin AtomPluginType)
{
	switch (AtomPluginType)
	{
	case EAtomPlugin::SourceDataOverride:
		return *SelectedSourceDataOverride;
		break;

	default:
		return FText::FromString(FString(TEXT("ERROR")));
		break;
	}
}

#undef LOCTEXT_NAMESPACE
