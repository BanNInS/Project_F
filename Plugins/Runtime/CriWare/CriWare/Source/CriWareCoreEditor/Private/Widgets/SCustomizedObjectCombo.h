
#pragma once

#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboButton.h"
#include "GameplayTagContainer.h"
#include "EditorUndoClient.h"
#include "DetailLayoutBuilder.h"

#include "Widgets/SAssetView.h"
#include "Atom/AtomSoundBase.h"

// Forward Declarations
class SMenuAnchor;

/**
 * Widget for selecting an object that use a customized detail view. 
 */
class SCustomizedObjectCombo : public SCompoundWidget, public FEditorUndoClient
{
public:

	DECLARE_DELEGATE_RetVal_OneParam(bool /*bCloseMenu*/, FOnSelectionChanged, UObject* /*Object*/)

	SLATE_BEGIN_ARGS(SCustomizedObjectCombo)
		: _Object(nullptr)
		, _TargetObject()
		, _TargetClass(UObject::StaticClass())
		, _PropertyHandle(nullptr)
		, _DisplayThumbnail(true)
		{}

		// Object for detail view
		SLATE_ATTRIBUTE(UObject*, Object)

		// Object for Combo button
		SLATE_ATTRIBUTE(FAssetData, TargetObject)

		// Allowed class for drag and drop
		SLATE_ARGUMENT(UClass*, TargetClass)

		// If set, the object is read from the property, and the property is changed when object is edited. 
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, PropertyHandle)

		// If set, the asset thumbnail is generated in the combo box button.
		SLATE_ARGUMENT(bool, DisplayThumbnail)

		// Callback for when button body is pressed with RMB
		//SLATE_EVENT(SGameplayTagChip::FOnMenu, OnMenu)

		// Called on selection
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS();

	CRIWARECOREEDITOR_API SCustomizedObjectCombo() : ObjectAttribute(nullptr) {}
	CRIWARECOREEDITOR_API virtual ~SCustomizedObjectCombo() override {}
	CRIWARECOREEDITOR_API void Construct(const FArguments& InArgs)
	{
		ObjectAttribute = InArgs._Object;
		TargetObject = InArgs._TargetObject;
		TargetClass = InArgs._TargetClass;
		OnSelectionChanged = InArgs._OnSelectionChanged;
		PropertyHandle = InArgs._PropertyHandle;
		bDisplayThumbnail = InArgs._DisplayThumbnail;

		/*if (PropertyHandle.IsValid())
		{
			PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &SGameplayTagCombo::RefreshTagsFromProperty));
			RefreshTagsFromProperty();
			GEditor->RegisterForUndo(this);
			bRegisteredForUndo = true;

			if (Filter.IsEmpty())
			{
				Filter = UGameplayTagsManager::Get().GetCategoriesMetaFromPropertyHandle(PropertyHandle);
			}
			bIsReadOnly = PropertyHandle->IsEditConst();
		}*/

		ChildSlot
		[
			SNew(SHorizontalBox) // Extra box to make the combo hug the chip
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ComboButton, SComboButton)
				//.ComboButtonStyle(FGameplayTagStyle::Get(), "GameplayTags.ComboButton")
				.HasDownArrow(true)
				.ContentPadding(1)
				.IsEnabled(this, &SCustomizedObjectCombo::IsValueEnabled)
				.Clipping(EWidgetClipping::OnDemand)
				.OnMenuOpenChanged(this, &SCustomizedObjectCombo::OnMenuOpenChanged)
				.OnGetMenuContent(this, &SCustomizedObjectCombo::OnGetMenuContent)
				.ButtonContent()
				[
					SAssignNew(ComboBoxContentView, SAssetView)
						.DisplayThumbnail(bDisplayThumbnail)
						.AllowAssetDropping(true)
						.OnAssetDropped(this, &SCustomizedObjectCombo::OnAssetDropped)
						.OnIsAssetAcceptableForDropWithReason(this, &SCustomizedObjectCombo::OnIsAssetAcceptableForDropWithReason)
						.OpenAssetOnDoubleClick(true)
						.AssetData(this, &SCustomizedObjectCombo::GetTargetObject)
						.ToolTipText(this, &SCustomizedObjectCombo::GetToolTipText)
				]
			]
		];
	}

protected:

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override { if (bSuccess) { /*RefreshTagsFromProperty();*/ } }
	virtual void PostRedo(bool bSuccess) override { if (bSuccess) { /*RefreshTagsFromProperty();*/ } }
	//~ End FEditorUndoClient Interface

private:

	UObject* GetObject() const { return ObjectAttribute.Get(); }
	FAssetData GetTargetObject() const { return TargetObject.Get(); }
	FText GetText() const { return FText::FromName(TargetObject.Get().AssetName); }

	bool IsValueEnabled() const
	{
		//if (PropertyHandle.IsValid())
		//{
		//	return !PropertyHandle->IsEditConst();
		//}

		//return !bIsReadOnly;
		return true;
	}

	FText GetToolTipText() const
	{
		if (FAssetData AssetData = TargetObject.Get(); AssetData.IsValid())
		{
			return FText::FromString(AssetData.GetFullName());
		}

		return FText::FromName(NAME_None);
	}

	bool IsSelected() const { return TargetObject.Get().IsValid(); }
	bool ShowClearButton() const { return IsSelected(); }

	FReply OnClearPressed()
	{
		OnClearObject();
		return FReply::Handled();
	}

	void OnMenuOpenChanged(const bool bOpen) const {}

	bool OnObjectSelected(UObject* InHolder)
	{
		bool bCloseMenu = true;
		if (OnSelectionChanged.IsBound())
		{
			bCloseMenu = OnSelectionChanged.Execute(InHolder);
		}

		// refresh
		ComboBoxContentView->SetObject(TargetObject.Get());

		return bCloseMenu;
	}

	void OnClearObject()
	{
		if (OnSelectionChanged.IsBound())
		{
			OnSelectionChanged.Execute(nullptr);
		}
	}

	TSharedRef<SWidget> OnGetMenuContent()
	{
		//if (!PropertyHandle.IsValid() && !PropertyHolderObject)
		//{
		//	PropertyHolderObject = NewObject<UAtomComboSoundHolder>();
		//}

		//PropertyHolderObject->Sound = SoundAttribute.Get();
		UObject* DetailObject = ObjectAttribute.Get();

		// create a property widget from PropertyEditor module
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs Args;
		Args.bHideSelectionTip = true;
		Args.bAllowSearch = false;

		if (!DetailView)
		{
			DetailView = PropertyEditorModule.CreateDetailView(Args);
			DetailView->OnFinishedChangingProperties().AddLambda([this, InDetailView = DetailView](const FPropertyChangedEvent& PropertyChangedEvent)
			{
				TArray<TWeakObjectPtr<UObject>> SelectedObjs = InDetailView->GetSelectedObjects();
				TArray<FAssetData> AssetDataArray;
				if (PropertyChangedEvent.Property && SelectedObjs.Num() > 0 && SelectedObjs[0].IsValid())
				{
					if (UObject* HolderObject = CastChecked<UObject>(SelectedObjs[0].Get()))
					{
						if (OnObjectSelected(HolderObject))
						{
							ComboButton->SetIsOpen(false);
						}
					}
				}
			});
		}

		DetailView->SetObject(DetailObject, true);

		return DetailView.ToSharedRef();
	}

	bool OnIsAssetAcceptableForDropWithReason(const FAssetData& Asset, FText& OutReason)
	{
		UClass* AssetDataClass = FindObject<UClass>(Asset.AssetClassPath);
		return AssetDataClass && AssetDataClass->IsChildOf(TargetClass);
	}

	void OnAssetDropped(const FDragDropEvent&, const FAssetData& Asset)
	{
		UClass* AssetDataClass = FindObject<UClass>(Asset.AssetClassPath);
		if (AssetDataClass && AssetDataClass->IsChildOf(TargetClass))
		{
			if (UObject* Object = Asset.GetAsset())
			{
				OnObjectSelected(Object);
			}
		}
	}

	TAttribute<UObject*> ObjectAttribute;
	TAttribute<FAssetData> TargetObject;
	UClass* TargetClass = nullptr;
	bool bRegisteredForUndo = false;
	bool bDisplayThumbnail = true;
	FOnSelectionChanged OnSelectionChanged;
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<SMenuAnchor> MenuAnchor;
	TSharedPtr<SComboButton> ComboButton;

	TSharedPtr<SAssetView> ComboBoxContentView;
	TSharedPtr<IDetailsView> DetailView;

	//////

	/*void RefreshTagsFromProperty()
	{
		check(PropertyHandle.IsValid());

		bHasMultipleValues = false;
		TagsFromProperty.Reset();

		SGameplayTagPicker::EnumerateEditableTagContainersFromPropertyHandle(PropertyHandle.ToSharedRef(), [this](const FGameplayTagContainer& TagContainer)
			{
				const FGameplayTag TagFromProperty = TagContainer.IsEmpty() ? FGameplayTag() : TagContainer.First();
				if (TagsFromProperty.Num() > 0 && TagsFromProperty[0] != TagFromProperty)
				{
					bHasMultipleValues = true;
				}
				TagsFromProperty.Add(TagFromProperty);

				return true;
			});
	}*/

	/*void OnCopyTag(const FGameplayTag TagToCopy) const
	{
		// Copy tag as a plain string, GameplayTag's import text can handle that.
		FPlatformApplicationMisc::ClipboardCopy(*TagToCopy.ToString());
	}*/
	
	/*void OnPasteTag()
	{
		FString PastedText;
		FPlatformApplicationMisc::ClipboardPaste(PastedText);
		const FGameplayTag PastedTag = UE::GameplayTags::EditorUtilities::GameplayTagTryImportText(PastedText);

		if (PastedTag.IsValid())
		{
			if (PropertyHandle.IsValid())
			{
				FScopedTransaction Transaction(LOCTEXT("GameplayTagCombo_PasteTag", "Paste Gameplay Tag"));
				PropertyHandle->SetValueFromFormattedString(PastedText);
				RefreshTagsFromProperty();
			}

			OnTagChanged.ExecuteIfBound(PastedTag);
		}
	}*/
	
	/*bool CanPaste() const
	{
		//FString PastedText;
		//FPlatformApplicationMisc::ClipboardPaste(PastedText);
		//FGameplayTag PastedTag = UE::GameplayTags::EditorUtilities::GameplayTagTryImportText(PastedText);

		//return PastedTag.IsValid();
		return true;
	}*/

	//FReply OnTagMenu(const FPointerEvent& MouseEvent)
	//{
	//	FMenuBuilder MenuBuilder(/*bShouldCloseWindowAfterMenuSelection=*/ true, /*CommandList=*/ nullptr);
		/*
			const FGameplayTag GameplayTag = GetCommonTag();

			auto IsValidTag = [GameplayTag]()
				{
					return GameplayTag.IsValid();
				};

			MenuBuilder.AddMenuEntry(
				LOCTEXT("GameplayTagCombo_SearchForReferences", "Search For References"),
				FText::Format(LOCTEXT("GameplayTagCombo_SearchForReferencesTooltip", "Find references to the tag {0}"), FText::AsCultureInvariant(GameplayTag.ToString())),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
				FUIAction(FExecuteAction::CreateLambda([GameplayTag]()
					{
						// Single tag search
						const FName TagFName = GameplayTag.GetTagName();
						if (FEditorDelegates::OnOpenReferenceViewer.IsBound() && !TagFName.IsNone())
						{
							TArray<FAssetIdentifier> AssetIdentifiers;
							AssetIdentifiers.Emplace(FGameplayTag::StaticStruct(), TagFName);
							FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
						}
					}))
			);

			MenuBuilder.AddSeparator();

			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("PropertyView", "CopyProperty", "Copy"),
				FText::Format(LOCTEXT("GameplayTagCombo_CopyTagTooltip", "Copy tag {0} to clipboard"), FText::AsCultureInvariant(GameplayTag.ToString())),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
				FUIAction(FExecuteAction::CreateSP(this, &SGameplayTagCombo::OnCopyTag, GameplayTag), FCanExecuteAction::CreateLambda(IsValidTag)));

			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("PropertyView", "PasteProperty", "Paste"),
				LOCTEXT("GameplayTagCombo_PasteTagTooltip", "Paste tags from clipboard."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste"),
				FUIAction(FExecuteAction::CreateSP(this, &SGameplayTagCombo::OnPasteTag), FCanExecuteAction::CreateSP(this, &SGameplayTagCombo::CanPaste)));

			MenuBuilder.AddMenuEntry(
				LOCTEXT("GameplayTagCombo_ClearTag", "Clear Gameplay Tag"),
				FText::GetEmpty(),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.X"),
				FUIAction(FExecuteAction::CreateSP(this, &SGameplayTagCombo::OnClearTag), FCanExecuteAction::CreateLambda(IsValidTag)));

			// Spawn context menu
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuBuilder.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

			return FReply::Handled();
	}*/
};