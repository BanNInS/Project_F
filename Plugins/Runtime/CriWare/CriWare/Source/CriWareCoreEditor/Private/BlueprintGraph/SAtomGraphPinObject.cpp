
#include "SAtomGraphPinObject.h"

#include "PropertyEditorModule.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Widgets/SCustomizedObjectCombo.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"
#include "AtomSoundGraphPinObject.h"

#define LOCTEXT_NAMESPACE "SAtomGraphPinObject"

void SAtomGraphPinObject::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	PropertyHolderObject = InArgs._PropertyHolderObject;
	DisplayThumbnail = InArgs._DisplayThumbnail;

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);	
}

TSharedRef<SWidget>	SAtomGraphPinObject::GetDefaultValueWidget()
{
	if (PropertyHolderObject)
	{
		// Load and show thumbnail, only if clicked 
		TAttribute<UObject*> PropertyHolderObjectAttribute = TAttribute<UObject*>::Create(TAttribute<UObject*>::FGetter::CreateSP(this, &SAtomGraphPinObject::GetPropertyHolderObject));
		
		// check if load to show the thumbnail.
		const bool bDisplay = GetAssetData().IsAssetLoaded() ? DisplayThumbnail : false;

		// Create widget
		return SAssignNew(Box, SVerticalBox)
			.IsEnabled(this, &SGraphPin::GetDefaultValueIsEditable)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			+ SVerticalBox::Slot()
			[
				SNew(SCustomizedObjectCombo)
					.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
					//.Filter(FilterString)
					.Object(PropertyHolderObjectAttribute)
					.OnSelectionChanged(this, &SAtomGraphPinObject::OnSelectionChanged)
					.TargetObject(this, &SAtomGraphPinObject::GetValue)
					.TargetClass(PropertyHolderObject->GetObjectClass())
					.DisplayThumbnail(bDisplay)
			];
	}

	return SGraphPin::GetDefaultValueWidget();
}

UObject* SAtomGraphPinObject::GetPropertyHolderObject()
{
	PropertyHolderObject->SetObject(GetAssetData().GetAsset());

	Box->GetSlot(0)
	[
		SNew(SCustomizedObjectCombo)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		//.Filter(FilterString)
		.Object(PropertyHolderObject)
		.OnSelectionChanged(this, &SAtomGraphPinObject::OnSelectionChanged)
		.TargetObject(this, &SAtomGraphPinObject::GetValue)
		.TargetClass(PropertyHolderObject->GetObjectClass())
		.DisplayThumbnail(DisplayThumbnail)
	];

	return PropertyHolderObject;
}

void SAtomGraphPinObject::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PropertyHolderObject);
}

bool SAtomGraphPinObject::OnSelectionChanged(UObject* SoundObject)
{
	if (!Cast<UAtomGraphPinObject>(SoundObject))
	{
		PropertyHolderObject->SetObject(SoundObject);
	}

	return SetValue(FAssetData(PropertyHolderObject->GetObject())) && PropertyHolderObject->GetObject() != nullptr;
}

bool SAtomGraphPinObject::SetValue(const FAssetData& Sound)
{
	if (GraphPinObj->PinType.PinCategory == GetDefault<UEdGraphSchema_K2>()->PC_SoftObject)
	{
		const FString ObjectPathString = Sound.GetObjectPathString();
		if (GraphPinObj->DefaultValue != ObjectPathString)
		{
			GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, ObjectPathString);
			return true;
		}
		return false;
	}

	UObject* Object = Sound.GetAsset();
	if (GraphPinObj->DefaultObject != Object)
	{
		GraphPinObj->GetSchema()->TrySetDefaultObject(*GraphPinObj, Object);
		return true;
	}

	return false;
}

FAssetData SAtomGraphPinObject::GetValue() const
{
	return GetAssetData();
}

const FAssetData& SAtomGraphPinObject::GetAssetData() const
{
	// For normal assets, the editor and runtime path are the same
	if (GraphPinObj->DefaultObject)
	{
		if (!GraphPinObj->DefaultObject->GetPathName().Equals(CachedAssetData.GetObjectPathString(), ESearchCase::CaseSensitive))
		{
			// This always uses the exact object pointed at
			CachedAssetData = FAssetData(GraphPinObj->DefaultObject, true);
		}
	}
	else if (!GraphPinObj->DefaultValue.IsEmpty())
	{
		FSoftObjectPath ObjectPath = FSoftObjectPath(GraphPinObj->DefaultValue);
		if (ObjectPath != CachedAssetData.GetSoftObjectPath())
		{
			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

			CachedAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(ObjectPath);

			if (!CachedAssetData.IsValid())
			{
				FString PackageName = FPackageName::ObjectPathToPackageName(GraphPinObj->DefaultValue);
				FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
				FString ObjectName = FPackageName::ObjectPathToObjectName(GraphPinObj->DefaultValue);

				// Fake one
				CachedAssetData = FAssetData(FName(*PackageName), FName(*PackagePath), FName(*ObjectName), UObject::StaticClass()->GetClassPathName());
			}
		}
	}
	else
	{
		if (CachedAssetData.IsValid())
		{
			CachedAssetData = FAssetData();
		}
	}

	return CachedAssetData;
}

#undef LOCTEXT_NAMESPACE
