/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankEditorToolkit.cpp
 *
 ****************************************************************************/

#include "AtomSoundBankEditorToolkit.h"

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
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomSoundBase.h"

#include "Widgets/SAtomSoundBrowser.h"
#include "Widgets/SAtomSoundBankBrowser.h"

#define LOCTEXT_NAMESPACE "AtomSoundBankEditor"

const FName FAtomSoundBankEditorToolkit::AppIdentifier("AtomSoundBankEditorApp");
const FName FAtomSoundBankEditorToolkit::PropertiesTabId("AtomSoundBankEditor_Properties");
const FName FAtomSoundBankEditorToolkit::BankBrowserTabId("AtomSoundBankEditor_BankBrowser");
const FName FAtomSoundBankEditorToolkit::SoundBrowserTabId("AtomSoundBankEditor_SoundBrowser");
const FName FAtomSoundBankEditorToolkit::SoundPropertiesTabId("AtomSoundBankEditor_SoundProperties");

FAtomSoundBankEditorToolkit::FAtomSoundBankEditorToolkit()
	: SoundBank(nullptr)
{
}

void FAtomSoundBankEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_AtomSoundBankEditor", "Atom Effect Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundBankEditorToolkit::SpawnTab_Properties))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(BankBrowserTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundBankEditorToolkit::SpawnTab_BankBrowser))
		.SetDisplayName(LOCTEXT("BankBrowserTab", "Banks"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));

	InTabManager->RegisterTabSpawner(SoundBrowserTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundBankEditorToolkit::SpawnTab_SoundBrowser))
		.SetDisplayName(LOCTEXT("SoundBrowserTab", "Sounds"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.ContentBrowser"));

	InTabManager->RegisterTabSpawner(SoundPropertiesTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundBankEditorToolkit::SpawnTab_SoundProperties))
		.SetDisplayName(LOCTEXT("SoundDetailsTab", "Sound Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FAtomSoundBankEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(PropertiesTabId);
	InTabManager->UnregisterTabSpawner(BankBrowserTabId);
	InTabManager->UnregisterTabSpawner(SoundBrowserTabId);
	InTabManager->UnregisterTabSpawner(SoundPropertiesTabId);
}

void FAtomSoundBankEditorToolkit::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAtomSoundBank* InBankToEdit)
{
	if (!ensure(InBankToEdit))
	{
		return;
	}

	SoundBank = TStrongObjectPtr<UAtomSoundBank>(InBankToEdit);
	
	// Support undo/redo
	InBankToEdit->SetFlags(RF_Transactional);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertiesView = PropertyModule.CreateDetailView(Args);
	PropertiesView->SetObject(InBankToEdit);

	SoundPropertiesView = PropertyModule.CreateDetailView(Args);
	SoundPropertiesView->SetObject(nullptr);

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_AtomSoundBankEditor_Layout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				// details
				FTabManager::NewStack()
				->SetSizeCoefficient(0.225f)
				->AddTab(PropertiesTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.550f)
				->Split
				(
					// sound library
					FTabManager::NewStack()
					->AddTab(SoundBrowserTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.7f)
				)
				->Split
				(
					// bank library
					FTabManager::NewStack()
					->AddTab(BankBrowserTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.3f)
				)
			)
			->Split
			(
				// sound details
				FTabManager::NewStack()
				->SetSizeCoefficient(0.225f)
				->AddTab(SoundPropertiesTabId, ETabState::OpenedTab)
			)
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
		InBankToEdit,
		bToolbarFocusable,
		bUseSmallIcons);
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
bool FAtomSoundBankEditorToolkit::CloseWindow(EAssetEditorCloseReason InCloseReason)
{
	return FAssetEditorToolkit::CloseWindow(InCloseReason);
}
#else
bool FAtomSoundBankEditorToolkit::CloseWindow()
{
	return FAssetEditorToolkit::CloseWindow();
}
#endif

FName FAtomSoundBankEditorToolkit::GetEditorName() const
{
	return "Sound Bank Editor";
}

FText FAtomSoundBankEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Atom Sound Bank Editor");
}

FName FAtomSoundBankEditorToolkit::GetToolkitFName() const
{
	return FName("AtomSoundBankEditor");
}

FText FAtomSoundBankEditorToolkit::GetToolkitName() const
{
	UObject* EditObject = GetEditingObjects()[0];
	return GetLabelForObject(EditObject);
}

FText FAtomSoundBankEditorToolkit::GetToolkitToolTipText() const
{
	return GetToolTipTextForObject(GetEditingObjects()[0]);
}

FString FAtomSoundBankEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "AtomSoundBank ").ToString();
}

FLinearColor FAtomSoundBankEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

EOrientation FAtomSoundBankEditorToolkit::GetSnapLabelOrientation() const
{
	return FMultiBoxSettings::UseSmallToolBarIcons.Get()
		? EOrientation::Orient_Horizontal
		: EOrientation::Orient_Vertical;
}

void FAtomSoundBankEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	
}

void FAtomSoundBankEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged)
{
	
}

TSharedRef<SDockTab> FAtomSoundBankEditorToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == PropertiesTabId);

	return SNew(SDockTab)
		.Label_Lambda([this]()
			{
				if (PropertiesView.IsValid())
				{
					auto& Objects = PropertiesView->GetSelectedObjects();
					if (Objects.Num() == 1)
					{
						if (UObject* Object = Objects[0].Get())
						{
							return FText::Format(LOCTEXT("AtomBankDetailsTitle", "{0}"), FText::FromName(Object->GetFName()));
						}
					}
					else if (Objects.Num() > 1)
					{
						return FText::Format(LOCTEXT("AtomBankDetailsTitleMultiple", "{0} banks"), FText::AsNumber(Objects.Num()));
					}
				}

				return LOCTEXT("AtomSoundBankDetailsTitle", "Details");
			})
		[
			PropertiesView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAtomSoundBankEditorToolkit::SpawnTab_BankBrowser(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == BankBrowserTabId);

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("AtomBankBrowserTitle", "SoundBanks"))
		[
			SNew(SAtomSoundBankBrowser, *SoundBank)
			.OnSelectionChanged(this, &FAtomSoundBankEditorToolkit::OnBankSelectionChanged)
		];
}

TSharedRef<SDockTab> FAtomSoundBankEditorToolkit::SpawnTab_SoundBrowser(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SoundBrowserTabId);

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("AtomSoundBrowserTitle", "Sounds"))
		[
			SAssignNew(SoundBrowser, SAtomSoundBrowser, *SoundBank)
				.OnSelectionChanged(this, &FAtomSoundBankEditorToolkit::OnSoundSelectionChanged)
		];
}

TSharedRef<SDockTab> FAtomSoundBankEditorToolkit::SpawnTab_SoundProperties(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == SoundPropertiesTabId);

	return SNew(SDockTab)
		.Label_Lambda([this]()
			{
				if (SoundPropertiesView.IsValid())
				{
					auto& Objects = SoundPropertiesView->GetSelectedObjects();
					if (Objects.Num() == 1)
					{
						if (UObject* Object = Objects[0].Get())
						{
							return FText::Format(LOCTEXT("AtomSoundDetailsTitle", "{0}"), FText::FromName(Object->GetFName()));
						}
					}
					else if (Objects.Num() > 1)
					{
						return FText::Format(LOCTEXT("AtomSoundDetailsTitleMultiple", "{0} sounds"), FText::AsNumber(Objects.Num()));
					}
				}

				return LOCTEXT("AtomSoundDetailsTitleDefault", "Sound Details");
			})
		[
			SoundPropertiesView.ToSharedRef()
		];
}

void FAtomSoundBankEditorToolkit::OnBankSelectionChanged(const TArray<UObject*>& NewSelection)
{
	UObject* EditingObject = GetEditingObjects()[0];
	const TArray<UObject*> EditingCopy = GetEditingObjects();
	for (UObject* Obj : EditingCopy)
	{
		RemoveEditingObject(Obj);
	}

	if (NewSelection.Num() > 0)
	{
		for (auto Object : NewSelection)
		{
			AddEditingObject(Object);
		}

		if (PropertiesView.IsValid())
		{
			PropertiesView->SetObjects(NewSelection);
		}

		if (SoundBrowser.IsValid())
		{
			SoundBrowser->SetSoundBanks(NewSelection);
		}
	}
	else
	{
		AddEditingObject(EditingObject);

		if (PropertiesView.IsValid())
		{
			PropertiesView->SetObject(GetEditingObjects()[0]);
		}

		if (SoundBrowser.IsValid())
		{
			SoundBrowser->SetSoundBanks(NewSelection);
		}
	}
}

void FAtomSoundBankEditorToolkit::OnSoundSelectionChanged(const TArray<UObject*>& NewSelection)
{
	if (SoundPropertiesView.IsValid())
	{
		SoundPropertiesView->SetObjects(NewSelection);
	}
}

void FAtomSoundBankEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
    Collector.AddReferencedObjects(GetEditingObjectPtrs());
#else
	for (UObject* Obj : GetEditingObjects())
	{
		Collector.AddReferencedObject(Obj);
	}
#endif
}

void FAtomSoundBankEditorToolkit::PostUndo(bool bSuccess)
{
}

void FAtomSoundBankEditorToolkit::PostRedo(bool bSuccess)
{
}

#undef LOCTEXT_NAMESPACE
