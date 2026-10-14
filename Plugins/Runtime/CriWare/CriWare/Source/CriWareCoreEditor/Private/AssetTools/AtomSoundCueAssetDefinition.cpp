/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundCueAssetDefinition.cpp
 *
 ****************************************************************************/

#include "AtomSoundCueAssetDefinition.h"

#include "AssetToolsModule.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "ContentBrowserModule.h"
#include "Delegates/Delegate.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/PackageName.h"
#include "Misc/Attribute.h"
#include "Modules/ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
#include "AssetDefinitionAssetInfo.h"
#include "IAssetStatusInfoProvider.h"
#endif

#include "Atom/AtomAttenuation.h"

#include "AtomEditorStyle.h"
#include "Factories/AtomAttenuationFactory.h"

#define LOCTEXT_NAMESPACE "AtomSoundCueDefinition"

namespace MenuExtension_AtomSoundCue
{
	static void GetAssetActions(FToolMenuSection& Section);
}

void UAtomSoundCueAssetDefinition::GetAssetActions(FToolMenuSection& Section) const
{
	Super::GetAssetActions(Section);

	MenuExtension_AtomSoundCue::GetAssetActions(Section);
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6

void UAtomSoundCueAssetDefinition::GetAssetStatusInfo(const TSharedPtr<IAssetStatusInfoProvider>&InAssetStatusInfoProvider, TArray<FAssetDisplayInfo>&OutStatusInfo) const
{
	Super::GetAssetStatusInfo(InAssetStatusInfoProvider, OutStatusInfo);

	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle(FAtomEditorStyle::Get().GetStyleSetName());
	if (ensure(AtomEditorStyle))
	{
		FAssetDisplayInfo Sound3DStatus;
		Sound3DStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.Sound3DIcon");
		Sound3DStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, -1);
		Sound3DStatus.StatusDescription = LOCTEXT("UAtomSoundCueAssetDefinition_Sound3DIcon_ToolTip", "Cue uses 3D attenuation settings defined in Atom Craft.");
		Sound3DStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundCueAssetDefinition::GetThumbnailSound3DStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(Sound3DStatus);

		FAssetDisplayInfo AisacStatus;
		AisacStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.AisacIcon");
		AisacStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, -2);
		AisacStatus.StatusDescription = LOCTEXT("UAtomSoundCueAssetDefinition_AisacIcon_ToolTip", "Cue uses Aisac Controls defined in Atom Craft.");
		AisacStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundCueAssetDefinition::GetThumbnailAisacStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(AisacStatus);

		FAssetDisplayInfo LoopStatus;
		LoopStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.LoopIcon");
		LoopStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, -3);
		LoopStatus.StatusDescription = LOCTEXT("UAtomSoundCueAssetDefinition_LoopIcon_ToolTip", "Cue is looping.");
		LoopStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundCueAssetDefinition::GetThumbnailLoopStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(LoopStatus);

		FAssetDisplayInfo LockStatus;
		LockStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.LockIcon");
		LockStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, -4);
		LockStatus.StatusDescription = LOCTEXT("UAtomSoundCueAssetDefinition_LockIcon_ToolTip", "Cue playback parameters are locked by Atom Craft.");
		LockStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundCueAssetDefinition::GetThumbnailLockStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(LockStatus);

		FAssetDisplayInfo PrivateStatus;
		PrivateStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.PrivateIcon");
		PrivateStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, -5);
		PrivateStatus.StatusDescription = LOCTEXT("UAtomSoundCueAssetDefinition_PrivateIcon_ToolTip", "Cue is marked as private by Atom Craft. A private cue cannot be played directly.");
		PrivateStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundCueAssetDefinition::GetThumbnailPrivateStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(PrivateStatus);
	}
}

EVisibility UAtomSoundCueAssetDefinition::GetThumbnailSound3DStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("Is3D"));
		if (AssetTagValue.IsSet())
		{
			return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
};

EVisibility UAtomSoundCueAssetDefinition::GetThumbnailAisacStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("NumAisacControls"));
		if (AssetTagValue.IsSet())
		{
			return FCString::Atoi(*AssetTagValue.GetValue()) > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility UAtomSoundCueAssetDefinition::GetThumbnailLoopStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsOneShot"));
		if (AssetTagValue.IsSet())
		{
			return AssetTagValue.GetValue() == TEXT("false") ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

EVisibility UAtomSoundCueAssetDefinition::GetThumbnailLockStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IgnorePlayerParameters"));
		if (AssetTagValue.IsSet())
		{
			return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility UAtomSoundCueAssetDefinition::GetThumbnailPrivateStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsPrivate"));
		if (AssetTagValue.IsSet())
		{
			return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

#else

TSharedPtr<SWidget> UAtomSoundCueAssetDefinition::GetSoundCueThumbnailOverlay(const FAssetData& AssetData)
{
	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle(FAtomEditorStyle::Get().GetStyleSetName());
	if (ensure(AtomEditorStyle))
	{
		// From UE 5.3 we can use the right side before the place was used by Source Control's status overlay.
		const EHorizontalAlignment HAlign = (FEngineVersion::Current().GetMajor() == 5 && FEngineVersion::Current().GetMinor() >= 3) ? HAlign_Right : HAlign_Left;
		const FMargin Padding = HAlign == HAlign_Right ? FMargin(0.0f, 3.0f, 3.0f, 0.0f) : FMargin(3.0f, 3.0f, 0.0f, 0.0f);
		const FSlateBrush* LockIcon = AtomEditorStyle->GetBrush("CriWare.LockIcon");
		const FSlateBrush* LoopIcon = AtomEditorStyle->GetBrush("CriWare.LoopIcon");
		const FSlateBrush* AisacIcon = AtomEditorStyle->GetBrush("CriWare.AisacIcon");
		const FSlateBrush* Sound3DIcon = AtomEditorStyle->GetBrush("CriWare.Sound3DIcon");
		const FSlateBrush* PrivateIcon = AtomEditorStyle->GetBrush("CriWare.PrivateIcon");

		auto OnGetAisacOverlayVisibilityLambda = [AssetData]() -> EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("NumAisacControls"));
			if (AssetTagValue.IsSet())
			{
				return FCString::Atoi(*AssetTagValue.GetValue()) > 0 ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		auto OnGetLoopOverlayVisibilityLambda = [AssetData]() -> EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsOneShot"));
			if (AssetTagValue.IsSet())
			{
				return AssetTagValue.GetValue() == TEXT("false") ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		auto OnGetLockOverlayVisibilityLambda = [AssetData]()->EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IgnorePlayerParameters"));
			if (AssetTagValue.IsSet())
			{
				return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		auto OnGetSound3DOverlayVisibilityLambda = [AssetData]() -> EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("Is3D"));
			if (AssetTagValue.IsSet())
			{
				return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		auto OnGetPrivateOverlayVisibilityLambda = [AssetData]() -> EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsPrivate"));
			if (AssetTagValue.IsSet())
			{
				return AssetTagValue.GetValue() == TEXT("true") ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		return SNew(SWrapBox)
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// Sound3D
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetSound3DOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundCueAssetDefinition_Sound3DIcon_ToolTip", "Cue uses 3D attenuation settings defined in Atom Craft."))
					.Image(Sound3DIcon)
				]
			]
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// AISAC
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetAisacOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundCueAssetDefinition_AisacIcon_ToolTip", "Cue uses Aisac Controls defined in Atom Craft."))
					.Image(AisacIcon)
				]
			]
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// Looping
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetLoopOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundCueAssetDefinition_LoopIcon_ToolTip", "Cue is looping."))
					.Image(LoopIcon)
				]
			]
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// Locked
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetLockOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundCueAssetDefinition_LockIcon_ToolTip", "Cue parameters are locked by Atom Craft. Any parameters or modulations set in Engine for playback are ignored."))
					.Image(LockIcon)
				]
			]
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// Private
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetPrivateOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundCueAssetDefinition_PrivateIcon_ToolTip", "Cue is marked as private by Atom Craft. A private cue cannot be played directly."))
					.Image(PrivateIcon)
				]
			];
	}

	return SNullWidget::NullWidget;
}

TSharedPtr<SWidget> UAtomSoundCueAssetDefinition::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	TSharedPtr<SWidget> PreviewOverlay = UAtomSoundBaseAssetDefinition::GetThumbnailOverlay(AssetData);

	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle("AtomEditorStyle");
	if (ensure(AtomEditorStyle))
	{
		// From UE 5.3 we can use the right side before the place was used by Source Control's status overlay.
		const EHorizontalAlignment HAlign = (FEngineVersion::Current().GetMajor() == 5 && FEngineVersion::Current().GetMinor() >= 3) ? HAlign_Right : HAlign_Left;

		return SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				GetSoundCueThumbnailOverlay(AssetData).ToSharedRef()
			]
			+ SOverlay::Slot()
			[
				PreviewOverlay.ToSharedRef()
			];
	}

	return PreviewOverlay;
}

#endif

// Menu Extensions
//--------------------------------------------------------------------

namespace MenuExtension_AtomSoundCue
{
	static bool CanExecuteConsolidateAttenuation(const FToolMenuContext& MenuContext)
	{
		return UContentBrowserAssetContextMenuContext::GetNumAssetsSelected(MenuContext) > 1;
	}

	static void ExecuteConsolidateAttenuation(const FToolMenuContext& MenuContext)
	{
		TMap<FAtomAttenuationSettings*, TArray<UAtomSoundCue*>> UnmatchedAttenuations;

		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			for (UAtomSoundCue* SoundCue : Context->LoadSelectedObjects<UAtomSoundCue>())
			{
				bool bFound = false;
				if (SoundCue && SoundCue->bOverrideAttenuation)
				{
					for (auto UnmatchedIt = UnmatchedAttenuations.CreateIterator(); UnmatchedIt; ++UnmatchedIt)
					{
						// Found attenuation settings to consolidate together
						if (SoundCue->AttenuationOverrides == *UnmatchedIt.Key())
						{
							UnmatchedIt.Value().Add(SoundCue);
							bFound = true;
							break;
						}
					}
					if (!bFound)
					{
						UnmatchedAttenuations.FindOrAdd(&SoundCue->AttenuationOverrides).Add(SoundCue);
					}
				}
			}
		}

		if (UnmatchedAttenuations.Num() > 0)
		{
			FString DefaultSuffix;
			TArray<UObject*> ObjectsToSync;

			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			UAtomAttenuationFactory* Factory = NewObject<UAtomAttenuationFactory>();

			for (auto UnmatchedIt = UnmatchedAttenuations.CreateConstIterator(); UnmatchedIt; ++UnmatchedIt)
			{
				if (UnmatchedIt.Value().Num() > 1)
				{
					FString Name;
					FString PackageName;
					AssetToolsModule.Get().CreateUniqueAssetName("/Game/Sounds/AtomAttenuations/SharedAttenuation", DefaultSuffix, PackageName, Name);

					UAtomAttenuation* AtomAttenuation = Cast<UAtomAttenuation>(AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UAtomAttenuation::StaticClass(), Factory));
					if (AtomAttenuation)
					{
						AtomAttenuation->Attenuation = *UnmatchedIt.Key();

						for (int32 AtomCueIndex = 0; AtomCueIndex < UnmatchedIt.Value().Num(); ++AtomCueIndex)
						{
							UAtomSoundCue* AtomCue = UnmatchedIt.Value()[AtomCueIndex];
							AtomCue->bOverrideAttenuation = false;
							AtomCue->AttenuationSettings = AtomAttenuation;
							AtomCue->MarkPackageDirty();
						}
					}
				}
			}

			if (ObjectsToSync.Num() > 0)
			{
				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
			}
		}
	}

	static void GetAssetActions(FToolMenuSection& Section)
	{
		const TAttribute<FText> Label = LOCTEXT("AtomSoundCue_ConsolidateAttenuation", "Consolidate Attenuation");
		const TAttribute<FText> ToolTip = LOCTEXT("AtomSoundCue_ConsolidateAttenuationTooltip", "Creates shared attenuation packages for sound cues with identical override attenuation settings.");
		const FSlateIcon Icon = FSlateIcon(FAtomEditorStyle::Get().GetStyleSetName(), "ClassIcon.AtomSoundCue");

		FToolUIAction UIAction;
		UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteConsolidateAttenuation);
		UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&CanExecuteConsolidateAttenuation);
		Section.AddMenuEntry("AtomSoundCue_ConsolidateAttenuation", Label, ToolTip, Icon, UIAction);
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, [] {
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);

			{
				UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UAtomSoundCue::StaticClass());

				FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
				Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
				{
					GetAssetActions(InSection);
				}));
			}
		}));
	});
} // namespace

#undef LOCTEXT_NAMESPACE
