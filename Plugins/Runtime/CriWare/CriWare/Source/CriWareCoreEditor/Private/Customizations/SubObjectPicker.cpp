/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SubObjectPicker.cpp
 *
 ****************************************************************************/

#include "Customizations/SubObjectPicker.h"

#include "SlateGlobals.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "ISinglePropertyView.h"
#include "PropertyCustomizationHelpers.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "Widgets/SAssetView.h"

#define LOCTEXT_NAMESPACE "FSubObjectPicker"

namespace FSubObjectPickerUtils
{
	static const FVector2D ContentBrowserWindowSize(300.0f, 300.0f);

	bool IsPropertyValueChildOf(TSharedPtr<IPropertyHandle> InProp, const UStruct* Class)
	{
		if (InProp.IsValid() && InProp->IsValidHandle())
		{
			FAssetData Val;
			InProp->GetValue(Val);
			return (Val.IsValid() && Val.GetClass()->IsChildOf(Class));
		}
		return false;
	}

	UClass* GetPropertyClass(TSharedPtr<IPropertyHandle> InProp)
	{
		if (InProp.IsValid())
		{
			auto Prop = CastField<FObjectProperty>(InProp->GetProperty());
			if (Prop)
			{
				return Prop->PropertyClass;
			}
		}

		auto SoftProp = CastField<FSoftObjectProperty>(InProp->GetProperty());
		if (SoftProp)
		{
			return SoftProp->PropertyClass;
		}

		auto StructProp = CastField<FStructProperty>(InProp->GetProperty());
		if (StructProp)
		{
			return StructProp->GetOwnerClass();
		}

		// in case it's the UAtomSoundAsset interface we use the UAtomSoundBank as wild card.
		auto InterProp = CastField<FInterfaceProperty>(InProp->GetProperty());
		if (InterProp && UAtomSoundAsset::StaticClass()->ImplementsInterface(InterProp->InterfaceClass))
		{
			return UAtomSoundBank::StaticClass();
		}

		return nullptr;
	}
}

TSharedPtr<IPropertyHandle> UParentObjectPropertyHolder_Base::MakeHandleFromProperty(FName PropertyName)
{
	// create a property widget from PropertyEditor module
	const FSinglePropertyParams InitParams;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	ParentView = PropertyEditorModule.CreateSingleProperty(this, PropertyName, InitParams);

	if (ParentView)
	{
		PropertyHandle = ParentView->GetPropertyHandle();
	}

	return PropertyHandle;
}

FSubObjectPicker::FSubObjectPicker(const FSubObjectPickerConfig& Config)
	: ObjectHandle(Config.ObjectHandle)
	, ParentHandle(Config.ParentHandle)
	, OnGetSubObjects(Config.OnGetSubObjects)
	, bDisplayThumbnail(Config.bDisplayThumbnail)
	, ThumbnailSize(Config.ThumbnailSize)
	, bOpenAssetOnDoubleClick(Config.bOpenAssetOnDoubleClick)
	, bAllowAssetDragging(Config.bAllowAssetDragging)
	, bAllowAssetDropping(Config.bAllowAssetDropping)
{
}

FSubObjectPicker::~FSubObjectPicker()
{}

TSharedRef<SWidget> FSubObjectPicker::CreateParentObjectPropertyNameWidget()
{
	if (ParentHandle.IsValid() && ParentHandle->IsValidHandle())
	{
		TSharedRef<SWidget> Widget = ParentHandle->CreatePropertyNameWidget();
		Widget->SetEnabled(TAttribute<bool>::Create([Handle = ObjectHandle]()
		{
			return Handle.IsValid() ? Handle->IsEditable() : true;
		}));
		return Widget;
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> FSubObjectPicker::CreateParentObjectPropertyValueWidget()
{
	if (ParentHandle.IsValid() && ParentHandle->IsValidHandle())
	{
		TSharedRef<SWidget> Widget = ParentHandle->CreatePropertyValueWidget();
		Widget->SetEnabled(TAttribute<bool>::Create([Handle = ObjectHandle]()
		{
			return Handle.IsValid() ? Handle->IsEditable() : true;
		}));
		return Widget;
	}

	return SNullWidget::NullWidget;
}

void FSubObjectPicker::AddParentPropertyRowContent(FDetailWidgetRow& WidgetRow)
{
	WidgetRow.NameContent()
	[
		CreateParentObjectPropertyNameWidget()
	]
	.ValueContent()
	[
		CreateParentObjectPropertyValueWidget()
	];
}

TSharedRef<SWidget> FSubObjectPicker::CreateSubObjectPropertyNameWidget()
{
	return SNew(STextBlock)
		.Text_Lambda([this]()
			{
				FText PropertyName = ObjectHandle.IsValid() ? ObjectHandle->GetPropertyDisplayName() : LOCTEXT("SubObjectPickerLabel", "Object");
				return	PropertyName;
			})
		.ToolTipText_Lambda([this]()
			{
				return	LOCTEXT("SubObjectPickerTooltip", "The object to use.");
			})
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

TSharedRef<SWidget> FSubObjectPicker::CreateSubObjectPropertyValueWidget()
{
	if (!ObjectHandle->IsValidHandle())
	{
		return SNullWidget::NullWidget;
	}

	FAssetData AssetData;
	ObjectHandle->GetValue(AssetData);

	if (ParentHandle.IsValid() && ParentHandle->IsValidHandle())
	{
		ParentHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FSubObjectPicker::OnParentPropertyChanged));
	}

	// combo-button with asset view thumbnail and asset picker as menu
	return SNew(SComboButton)
		//.HasDownArrow(true)
		//.ContentPadding(FMargin(2.f, 2.f, 2.f, 1.f))
		//.ToolTipText(LOCTEXT("SetRig_ToolTip", "Selects the camera rig"))
		.OnGetMenuContent(this, &FSubObjectPicker::OnGetMenuContent)
		.ButtonContent()
		[
			SAssignNew(ObjectComboButtonContentView, SAssetView)
			.AssetData(AssetData)
			.DisplayThumbnail(bDisplayThumbnail)
			.ThumbnailSize(ThumbnailSize)
			.OpenAssetOnDoubleClick(bOpenAssetOnDoubleClick)
			.AllowAssetDragging(bAllowAssetDragging)
			.AllowAssetDropping(bAllowAssetDropping)
			.OnIsAssetAcceptableForDropWithReason_Lambda([&](const FAssetData& Asset, FText& OutReason)
				{
					if (ObjectHandle.IsValid() && ObjectHandle->IsValidHandle())
					{
						auto ObjectProperty = CastField<FObjectProperty>(ObjectHandle->GetProperty());
						return ObjectProperty && ObjectProperty->PropertyClass == Asset.GetClass();
					}
					return false;
				})
			.OnAssetDropped_Lambda([&](const FDragDropEvent&, const FAssetData& Asset)
				{
					if (ObjectHandle.IsValid() && ObjectHandle->IsValidHandle())
					{
							ObjectHandle->SetValue(Asset);
							ObjectComboButtonContentView->SetObject(Asset);
					}
				})
			.ToolTipText(this, &FSubObjectPicker::GetObjectComboButtonToolTip)
		];
}

void FSubObjectPicker::OnParentPropertyChanged()
{
	if (ParentHandle.IsValid() && ParentHandle->IsValidHandle())
	{
		// reset
		ObjectHandle->SetValue((UObject*)nullptr);

		// reset the combobox content also
		if (ObjectComboButtonContentView)
		{
			ObjectComboButtonContentView->SetObject(FAssetData());
		}
	}
}

TSharedRef<SWidget> FSubObjectPicker::OnGetMenuContent()
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;
	const bool bCloseSelfOnly = true;
	const bool bSearchable = false;

	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly, &FCoreStyle::Get(), bSearchable);

	if (ObjectHandle.IsValid() && ObjectHandle->IsValidHandle())
	{
		UClass* ObjectClass = FSubObjectPickerUtils::GetPropertyClass(ObjectHandle);
		FAssetData CurrentAssetData;
		ObjectHandle->GetValue(CurrentAssetData);

		FAssetData ParentAssetData;
		ParentHandle->GetValue(ParentAssetData);

		MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
		{
			// initialize asset picker
			FAssetPickerConfig AssetPickerConfig;
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				AssetPickerConfig.Filter.ClassPaths.Add(ObjectClass->GetClassPathName());
				AssetPickerConfig.Filter.SoftObjectPaths.Add(ParentAssetData.GetSoftObjectPath());
#else
				AssetPickerConfig.Filter.ClassNames.Add(ObjectClass->GetFName());
				AssetPickerConfig.Filter.ObjectPaths.Add(ParentAssetData.ObjectPath);
#endif
				AssetPickerConfig.Filter.bRecursiveClasses = false;
				AssetPickerConfig.OnGetCustomSourceAssets = FOnGetCustomSourceAssets::CreateSP(this, &FSubObjectPicker::OnGetCustomSourceAssets);
				AssetPickerConfig.AssetShowWarningText = LOCTEXT("NoObjectFound", "No object found.");
				AssetPickerConfig.bAllowDragging = false;
				AssetPickerConfig.bCanShowRealTimeThumbnails = true;
				AssetPickerConfig.bCanShowDevelopersFolder = false;
				AssetPickerConfig.bForceShowEngineContent = false;
				AssetPickerConfig.bForceShowPluginContent = false;
				AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
				AssetPickerConfig.InitialAssetSelection = CurrentAssetData;
				AssetPickerConfig.SaveSettingsName = TEXT("SubObjectAssetPropertyPicker");

				AssetPickerConfig.bAllowNullSelection = true;
				AssetPickerConfig.OnAssetSelected = FOnAssetDoubleClicked::CreateSP(this, &FSubObjectPicker::OnAssetSelected);
				AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateSP(this, &FSubObjectPicker::OnAssetEnterPressed);
			}

			auto& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

			TSharedRef<SWidget> MenuContent =
				SNew(SBox)
				.WidthOverride(static_cast<float>(FSubObjectPickerUtils::ContentBrowserWindowSize.X))
				.HeightOverride(static_cast<float>(FSubObjectPickerUtils::ContentBrowserWindowSize.Y))
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				];

			MenuBuilder.AddWidget(MenuContent, FText::GetEmpty(), true);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void FSubObjectPicker::OnGetCustomSourceAssets(const FARFilter& Filter, TArray<FAssetData>& OutAssets) const
{
	if (ParentHandle.IsValid() && ParentHandle->IsValidHandle())
	{
		// get sub-objects
		OnGetSubObjects.ExecuteIfBound(ParentHandle, OutAssets);
	}
}

void FSubObjectPicker::OnAssetSelected(const FAssetData& SelectedAsset)
{
	if (ObjectHandle.IsValid() && ObjectHandle->IsValidHandle())
	{
		// setup
		ObjectHandle->SetValue(SelectedAsset.GetAsset());

		// also the combo-button content with selection
		if (ObjectComboButtonContentView)
		{
			ObjectComboButtonContentView->SetObject(SelectedAsset);
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

void FSubObjectPicker::OnAssetEnterPressed(const TArray<FAssetData>& AssetData)
{
	if (AssetData.Num() > 0)
	{
		OnAssetSelected(AssetData[0]);
	}
	else
	{
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FSubObjectPicker::AddSubObjectPropertyRowContent(FDetailWidgetRow& WidgetRow, bool bIsAlwaysVisible /* = false */)
{
	WidgetRow.NameContent()
	[
		CreateSubObjectPropertyNameWidget()
	]
	.ValueContent()
	[
		CreateSubObjectPropertyValueWidget()
	];

	if (!bIsAlwaysVisible)
	{
		WidgetRow.Visibility(TAttribute<EVisibility>(this, &FSubObjectPicker::IsParentSelected));
	}
}

EVisibility FSubObjectPicker::IsParentSelected() const
{
	if (!FSubObjectPickerUtils::IsPropertyValueChildOf(ParentHandle, UObject::StaticClass()))
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}

FText FSubObjectPicker::GetObjectComboButtonToolTip() const
{
	if (ObjectComboButtonContentView)
	{
		FAssetData AssetData;
		ObjectComboButtonContentView->GetObject(AssetData);
		return FText::FromString(AssetData.GetFullName());
	}

	return FText::FromName(NAME_None);
}

#undef LOCTEXT_NAMESPACE
