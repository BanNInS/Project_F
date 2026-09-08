/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : FMovieSceneAtomSectionDetails.cpp
 *
 ****************************************************************************/

#include "MovieSceneAtomSectionDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"

#include "Atom/AtomComponent.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

#include "MovieSceneAtomSection.h"

#define LOCTEXT_NAMESPACE	"FMovieSceneAtomSectionDetailsInspector"

TSharedRef<IDetailCustomization> FMovieSceneAtomSectionDetails::MakeInstance()
{
	return MakeShareable(new FMovieSceneAtomSectionDetails);
}

void FMovieSceneAtomSectionDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FMovieSceneAtomSectionDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> AtomComponentClassHandle = DetailBuilder.GetProperty(TEXT("AtomComponentClass"));
	TSharedRef<IPropertyHandle> ApplyExtensionSettingsHandle = DetailBuilder.GetProperty(TEXT("bApplyExtensionSettings"));
	AtomComponentClassHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FMovieSceneAtomSectionDetails::OnAtomComponentClassChanged));
	ApplyExtensionSettingsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FMovieSceneAtomSectionDetails::OnAtomComponentClassChanged));

	DetailBuilder.EditCategory(TEXT("Atom"), FText::GetEmpty(), ECategoryPriority::Uncommon);
	DetailBuilder.EditCategory(TEXT("Attenuation"), FText::GetEmpty(), ECategoryPriority::Uncommon);

	TArray<TWeakObjectPtr<UObject>> OutObjects;
	DetailBuilder.GetObjectsBeingCustomized(OutObjects);
	UMovieSceneAtomSection* AtomSection = nullptr;
	if (OutObjects.Num() != 0)
	{
		AtomSection = Cast<UMovieSceneAtomSection>(OutObjects[0].Get());
	}

	if (AtomSection && AtomSection->IsApplyingExtensionSettings())
	{
		if (UAtomComponentExtensionSettings* ExtensionSettings = AtomSection->GetAtomComponentExtensionSettings())
		{
			// Add extra category.
			IDetailCategoryBuilder& ExtraCategoryBuilder = DetailBuilder.EditCategory(ExtensionSettings->GetDisplayName(), FText::GetEmpty(), ECategoryPriority::Uncommon);
			TArray<UObject*> Objects;
			Objects.Add(ExtensionSettings);
			FAddPropertyParams Params;
			Params.HideRootObjectNode(true);
			Params.CreateCategoryNodes(false);
			ExtraCategoryBuilder.AddExternalObjects(Objects, EPropertyLocation::Default, Params);
		}
	}
}

void FMovieSceneAtomSectionDetails::OnAtomComponentClassChanged()
{
	// Here we can only take the ptr as ForceRefreshDetails() checks that the reference is unique.
	if (IDetailLayoutBuilder* DetailBuilder = CachedDetailBuilder.Pin().Get())
	{
		DetailBuilder->ForceRefreshDetails();
	}
}

#undef LOCTEXT_NAMESPACE
