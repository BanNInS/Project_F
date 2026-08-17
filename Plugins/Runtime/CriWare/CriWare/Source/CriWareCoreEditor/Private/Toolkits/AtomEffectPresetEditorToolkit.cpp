/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEffectPresetEditorToolkit.cpp
 *
 ****************************************************************************/

#include "AtomEffectPresetEditorToolkit.h"

#include "Blueprint/UserWidget.h"
#include "Containers/Set.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "WidgetBlueprint.h"

#include "Atom/Atom.h"
#include "Atom/AtomEffectPreset.h"
#include "Atom/AtomEffectPresetWidgetInterface.h"

#include "AtomEditorStyle.h"

#define LOCTEXT_NAMESPACE "AtomEffectPresetEditor"


const FName FAtomEffectPresetEditorToolkit::AppIdentifier("AtomEffectPresetEditorApp");
const FName FAtomEffectPresetEditorToolkit::PropertiesTabId("AtomEffectPresetEditor_Properties");
const FName FAtomEffectPresetEditorToolkit::UserWidgetTabId("AtomEffectPresetEditor_UserWidget");

FAtomEffectPresetEditorToolkit::FAtomEffectPresetEditorToolkit()
	: AtomEffectPreset(nullptr)
{
}

void FAtomEffectPresetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_AtomEffectPresetEditor", "Atom Effect Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FAtomEffectPresetEditorToolkit::SpawnTab_Properties))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));


	for (int32 i = 0; i < UserWidgets.Num(); i++)
	{
		TObjectPtr<UUserWidget> UserWidget = UserWidgets[i];

		FText Label = IAtomEffectPresetWidgetInterface::Execute_GetEditorName(UserWidget.Get());
		if (Label.IsEmpty())
		{
			Label = FText::FromString(AtomEffectPreset->GetClass()->GetName());
		}

		FSlateIcon BPIcon;
		FName IconBrushName = IAtomEffectPresetWidgetInterface::Execute_GetIconBrushName(UserWidget.Get());
		if (IconBrushName.IsNone())
		{
			BPIcon = FSlateIcon(FAtomEditorStyle::Get().GetStyleSetName(), "CriWare.ADXIcon");
		}
		else
		{
			BPIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), IconBrushName);
		}

		const FName UserWidgetTabIdIndexed = FName(UserWidgetTabId.ToString() + FString(TEXT("_")) + FString::FromInt(i));
		InTabManager->RegisterTabSpawner(UserWidgetTabIdIndexed, FOnSpawnTab::CreateLambda([this, i](const FSpawnTabArgs& Args) { return SpawnTab_UserWidgetEditor(Args, i); }))
			.SetDisplayName(Label)
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(BPIcon);
	}
}

void FAtomEffectPresetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(PropertiesTabId);

	for (int32 i = 0; i < UserWidgets.Num(); i++)
	{
		const FName UserWidgetTabIdIndexed = FName(UserWidgetTabId.ToString() + FString(TEXT("_")) + FString::FromInt(i));
		InTabManager->UnregisterTabSpawner(UserWidgetTabIdIndexed);
	}
}

void FAtomEffectPresetEditorToolkit::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAtomEffectPreset* InPresetToEdit, const TArray<UUserWidget*>& InWidgetBlueprints)
{
	if (!ensure(InPresetToEdit))
	{
		return;
	}

	AtomEffectPreset = TStrongObjectPtr<UAtomEffectPreset>(InPresetToEdit);
	InitPresetWidgets(InWidgetBlueprints);
	
	// Support undo/redo
	InPresetToEdit->SetFlags(RF_Transactional);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertiesView = PropertyModule.CreateDetailView(Args);
	PropertiesView->SetObject(InPresetToEdit);

	TSharedRef<FTabManager::FSplitter> TabSplitter = FTabManager::NewSplitter()
		->SetSizeCoefficient(0.9f)
		->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.225f)
			->AddTab(PropertiesTabId, ETabState::OpenedTab)
		);

	if (!UserWidgets.IsEmpty())
	{
		TabSplitter->Split
		(
			FTabManager::NewSplitter()
			->SetSizeCoefficient(0.775f)
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetHideTabWell(true)
				->SetSizeCoefficient(0.33f)
				->AddTab(UserWidgetTabId, ETabState::OpenedTab)
			)
		);
	}

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_AtomEffectPresetEditor_Layout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split(TabSplitter)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	const bool bToolbarFocusable = false;
	const bool bUseSmallIcons = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode,
		InitToolkitHost,
		AppIdentifier,
		StandaloneDefaultLayout,
		bCreateDefaultStandaloneMenu,
		bCreateDefaultToolbar,
		InPresetToEdit,
		bToolbarFocusable,
		bUseSmallIcons);
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
bool FAtomEffectPresetEditorToolkit::CloseWindow(EAssetEditorCloseReason InCloseReason)
{
	if (FAssetEditorToolkit::CloseWindow(InCloseReason))
	{
		UserWidgets.Reset();
		return true;
	}

	return false;
}
#else
bool FAtomEffectPresetEditorToolkit::CloseWindow()
{
	if (FAssetEditorToolkit::CloseWindow())
	{
		UserWidgets.Reset();
		return true;
	}

	return false;
}
#endif

FName FAtomEffectPresetEditorToolkit::GetEditorName() const
{
	return "Preset Editor";
}

FName FAtomEffectPresetEditorToolkit::GetToolkitFName() const
{
	return FName("AtomEffectPresetEditor");
}

FText FAtomEffectPresetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Atom Effect Preset Editor");
}

void FAtomEffectPresetEditorToolkit::InitPresetWidgets(const TArray<UUserWidget*>& InWidgets)
{
	if (!AtomEffectPreset)
	{
		return;
	}

	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		for (UUserWidget* Widget : InWidgets)
		{
			if (Widget)
			{
				UserWidgets.Add(Widget);
				IAtomEffectPresetWidgetInterface::Execute_OnConstructed(Widget, AtomEffectPreset.Get());
			}
		}
	}
}

FString FAtomEffectPresetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "AtomEffect ").ToString();
}

FLinearColor FAtomEffectPresetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

EOrientation FAtomEffectPresetEditorToolkit::GetSnapLabelOrientation() const
{
	return FMultiBoxSettings::UseSmallToolBarIcons.Get()
		? EOrientation::Orient_Horizontal
		: EOrientation::Orient_Vertical;
}

void FAtomEffectPresetEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	if (AtomEffectPreset)
	{
		for (TObjectPtr<UUserWidget>& UserWidget : UserWidgets)
		{
			const FName PropertyName = PropertyThatChanged->GetFName();
			IAtomEffectPresetWidgetInterface::Execute_OnPropertyChanged(UserWidget.Get(), AtomEffectPreset.Get(), PropertyName);
		}
	}
}

void FAtomEffectPresetEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{
	if (AtomEffectPreset)
	{
		for (TObjectPtr<UUserWidget>& UserWidget : UserWidgets)
		{
			auto Node = PropertyThatChanged->GetHead();
			while(Node)
			{
				if (FProperty* Property = Node->GetValue())
				{
					const FName PropertyName = Property->GetFName();
					IAtomEffectPresetWidgetInterface::Execute_OnPropertyChanged(UserWidget.Get(), AtomEffectPreset.Get(), PropertyName);
				}
				Node = Node->GetNextNode();
			}
		}
	}
}

TSharedRef<SDockTab> FAtomEffectPresetEditorToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PropertiesTabId);

	return SNew(SDockTab)
		.Label(LOCTEXT("AtomAtomEffectDetailsTitle", "Details"))
		[
			PropertiesView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAtomEffectPresetEditorToolkit::SpawnTab_UserWidgetEditor(const FSpawnTabArgs& Args, int32 WidgetIndex)
{
	const FSlateBrush* IconBrush = nullptr;
	FName IconBrushName = IAtomEffectPresetWidgetInterface::Execute_GetIconBrushName(UserWidgets[WidgetIndex].Get());
	if (IconBrushName.IsNone())
	{
		IconBrush = FAtomEditorStyle::Get().GetBrush("CriWare.ADXIcon");
	}
	else
	{
		IconBrush = FAppStyle::GetBrush(IconBrushName);
	}

	FText Label = FText::FromString(AtomEffectPreset->GetName());
	if (UserWidgets.Num() < WidgetIndex)
	{
		TSharedPtr<SDockTab> NewTab = SNew(SDockTab)
			.Label(Label)
			.TabColorScale(GetTabColorScale())
			[
				SNew(STextBlock)
					.Text(LOCTEXT("InvalidPresetEditor", "No editor available for AtomEffectPreset.  Widget Blueprint not found."))
			];
		NewTab->SetTabIcon(IconBrush);
		return NewTab.ToSharedRef();
	}

	const FText CustomLabel = IAtomEffectPresetWidgetInterface::Execute_GetEditorName(UserWidgets[WidgetIndex].Get());
	if (!CustomLabel.IsEmpty())
	{
		Label = CustomLabel;
	}

	TSharedPtr<SDockTab> NewTab = SNew(SDockTab)
		.Label(Label)
		.TabColorScale(GetTabColorScale())
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.0f)
			[
				UserWidgets[WidgetIndex]->TakeWidget()
			]
		];
	NewTab->SetTabIcon(IconBrush);
	return NewTab.ToSharedRef();
}

void FAtomEffectPresetEditorToolkit::PostUndo(bool bSuccess)
{
}

void FAtomEffectPresetEditorToolkit::PostRedo(bool bSuccess)
{
}

void FAtomEffectPresetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(UserWidgets);
}

#undef LOCTEXT_NAMESPACE
