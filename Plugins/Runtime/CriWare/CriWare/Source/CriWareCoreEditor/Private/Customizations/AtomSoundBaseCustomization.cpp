/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBaseCustomization.cpp
 *
 ****************************************************************************/

#include "AtomSoundBaseCustomization.h"

#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "SAssetDropTarget.h"

#include "Atom/Atom.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"

#include "Widgets/SAssetView.h"

#define LOCTEXT_NAMESPACE "AtomSoundBase"

namespace AtomSoundBaseUtils
{
	bool IsPropertyValueChildOf(TSharedPtr<IPropertyHandle> InProp, const UStruct* Class)
	{
		if (InProp.IsValid())
		{
			FAssetData Val;
			InProp->GetValue(Val);
			return (Val.IsValid() && Val.GetClass()->IsChildOf(Class));
		}
		return false;
	}

	bool IsClassChildOfPropertyClass(const UStruct* Class, TSharedPtr<IPropertyHandle> InProp)
	{
		if (InProp.IsValid())
		{
			auto Prop = CastField<FObjectProperty>(InProp->GetProperty());
			if (Prop)
			{
				return (Prop && Class->IsChildOf(Prop->PropertyClass));
			}

			auto SoftProp = CastField<FSoftObjectProperty>(InProp->GetProperty());
			if (SoftProp)
			{
				return (SoftProp && Class->IsChildOf(SoftProp->PropertyClass));
			}

			// in case it's the UAtomSoundAsset interface we use the UAtomSoundBank as wild card.
			auto InterProp = CastField<FInterfaceProperty>(InProp->GetProperty());
			if (Class->GetOwnerClass()->ImplementsInterface(InterProp->InterfaceClass))
			{
				return (InterProp && Class->IsChildOf(UAtomSoundBank::StaticClass()));
			}
		}
		return false;
	}

	UObject* GetValueAsObject(TSharedPtr<IPropertyHandle> PropertyHandle)
	{
		UObject* Object = nullptr;
		PropertyHandle->GetValue(Object);

		if (Object == nullptr)
		{
			// Check to see if it's pointing to an unloaded object
			FString CurrentObjectPath;
			PropertyHandle->GetValueAsFormattedString(CurrentObjectPath);

			if (CurrentObjectPath.Len() > 0 && CurrentObjectPath != TEXT("None"))
			{
				FSoftObjectPath SoftObjectPath = FSoftObjectPath(CurrentObjectPath);
				Object = SoftObjectPath.TryLoad();
			}
		}

		return Object;
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

		// in case it's the UAtomSoundAsset interface we use the UAtomSoundBank as wild card.
		auto InterProp = CastField<FInterfaceProperty>(InProp->GetProperty());
		if (UAtomSoundAsset::StaticClass()->ImplementsInterface(InterProp->InterfaceClass))
		{
			return UAtomSoundBank::StaticClass();
		}

		return nullptr;
	}
}

/* FAtomSoundBaseCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomSoundBaseCustomization::MakeInstance()
{
	return MakeShareable(new FAtomSoundBaseCustomization);
}

void FAtomSoundBaseCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomSoundBaseCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	UClass* PropertyClass = AtomSoundBaseUtils::GetPropertyClass(StructPropertyHandle);

	// Sound type without bank are not customized.
	if (!CanCustomize(PropertyClass))
	{
		ChildBuilder.AddCustomRow(FText::GetEmpty())
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				StructPropertyHandle->CreatePropertyValueWidget()
			];
		return;
	}

	// Get handle to layout builder to enable adding properties to categories
	//IDetailLayoutBuilder& LayoutBuilder = ChildBuilder.GetParentCategory().GetParentLayout();

	SoundBaseHandle = StructPropertyHandle;

	// Create a fake sound property to show sound bank and select property from it.
	SoundBankHolder = CreateSoundBankPropertyHolder(PropertyClass);
	SoundBankHandle = SoundBankHolder.Get()->GetProperty();
	SoundBankHandle->SetPropertyDisplayName(SoundBaseHandle->GetPropertyDisplayName());
	SoundBankHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomSoundBaseCustomization::OnSoundBankPropertyChanged));
	
	// Set default value
	UObject* SoundBaseObject = AtomSoundBaseUtils::GetValueAsObject(SoundBaseHandle);
	if (UAtomSoundCue* Cue = Cast<UAtomSoundCue>(SoundBaseObject))
	{
		SoundBankHandle->SetValue(Cue->CueSheet);
	}
	else if (UAtomSoundWave* Wave = Cast<UAtomSoundWave>(SoundBaseObject))
	{
		SoundBankHandle->SetValue(Wave->WaveBank);
	}
	else if (UAtomSoundBase* Sound = Cast<UAtomSoundBase>(SoundBaseObject))
	{
		SoundBankHandle->SetValue(Sound);
	}
	else
	{
		SoundBankHandle->SetValue((UObject*)nullptr);
	}

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			SoundBankHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SAssetDropTarget)
				.OnAreAssetsAcceptableForDropWithReason_Lambda([&](TArrayView<FAssetData> InAssets, FText& OutReason) -> bool
					{
						if (InAssets.Num() > 0)
						{
							return AtomSoundBaseUtils::IsClassChildOfPropertyClass(InAssets[0].GetClass(), SoundBankHandle)
								|| OnIsAssetAcceptableForDropWithReason(InAssets[0], OutReason);
						}
						return false;
					})
				.OnAssetsDropped_Lambda([&](const FDragDropEvent& Event, TArrayView<FAssetData> InAssets) -> void
					{
						if (InAssets.Num() > 0)
						{
							if (AtomSoundBaseUtils::IsClassChildOfPropertyClass(InAssets[0].GetClass(), SoundBankHandle))
							{
								SoundBankHandle->SetValue(InAssets[0]);
								SoundBaseHandle->SetValue((UObject*)nullptr);
								//SoundBasePicker-> null
							}
							else
							{
								OnAssetDropped(Event, InAssets[0]);
							}
						}
					})
				.Content()
				[
					SoundBankHandle->CreatePropertyValueWidget()
				]
		];

	// Setup a sub-object picker to select inner AtomSoundBase from any AtomSoundBank.
	FSubObjectPickerConfig Config;
	Config.ParentHandle = SoundBankHandle;
	Config.ObjectHandle = SoundBaseHandle;
	Config.OnGetSubObjects = FOnGetSubObjectAssets::CreateLambda([this](TSharedPtr<IPropertyHandle>, TArray<FAssetData>& Assets)
		{
			if (SoundBankHolder.IsValid())
			{
				for (int Index = 0; Index < SoundBankHolder->GetNumSubObjects(); Index++)
				{
					Assets.Add(SoundBankHolder->GetSubObject(Index));
				}
			}
		});
	Config.bOpenAssetOnDoubleClick = true;
	Config.bAllowAssetDragging = true;
	Config.bAllowAssetDropping = true;

	SoundBasePicker = MakeShared<FSubObjectPicker>(Config);
	auto ValueWidget = SoundBasePicker->CreateSubObjectPropertyValueWidget();
	ValueWidget->SetEnabled(TAttribute<bool>::CreateSP(this, &FAtomSoundBaseCustomization::IsSoundPickerEnabled));

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundBaseCustomization::IsSoundBankSelected))
		.NameContent()
		[
			SNew(STextBlock)
			.Text_Lambda([this]() 
			{
				FText PropertyName = SoundBaseHandle.IsValid() ? SoundBaseHandle->GetPropertyDisplayName() : LOCTEXT("AtomSoundBaseDetailLabel", "Sound");
				return	AtomSoundBaseUtils::IsPropertyValueChildOf(SoundBankHandle, UAtomCueSheet::StaticClass()) ? FText::Format(LOCTEXT("AtomSoundBaseDetailCueLabel", "{0} (Cue)"), PropertyName) :
						AtomSoundBaseUtils::IsPropertyValueChildOf(SoundBankHandle, UAtomWaveBank::StaticClass()) ? FText::Format(LOCTEXT("AtomSoundBaseDetailWaveLabel", "{0} (Wave)"), PropertyName) :
						FText::Format(LOCTEXT("AtomSoundBaseDetailLabel", "{0} (Sound)"), PropertyName);
			})
			.ToolTipText_Lambda([this]()
			{
				return	AtomSoundBaseUtils::IsPropertyValueChildOf(SoundBankHandle, UAtomCueSheet::StaticClass()) ? LOCTEXT("AtomSoundBaseDetailCueTooltip", "The cue in the AtomCueSheet to use.") :
						AtomSoundBaseUtils::IsPropertyValueChildOf(SoundBankHandle, UAtomWaveBank::StaticClass()) ? LOCTEXT("AtomSoundBaseDetailWaveTooltip", "The wave in the AtomWaveBank to use.") :
						LOCTEXT("AtomSoundBaseDetailTooltip", "The sound to use.");
			})
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			ValueWidget
		];
}

bool FAtomSoundBaseCustomization::IsSoundPickerEnabled() const
{
	FAssetData AssetData;
	if (SoundBankHandle.IsValid() && SoundBankHandle->IsValidHandle())
	{
		SoundBankHandle->GetValue(AssetData);
	}
	return AssetData.IsValid();
}

bool FAtomSoundBaseCustomization::OnIsAssetAcceptableForDropWithReason(const FAssetData& Asset, FText& OutReason)
{
	if (SoundBaseHandle.IsValid() && SoundBaseHandle->IsValidHandle())
	{
		return AtomSoundBaseUtils::IsClassChildOfPropertyClass(Asset.GetClass(), SoundBaseHandle);
	}
	return false;
}

void FAtomSoundBaseCustomization::OnAssetDropped(const FDragDropEvent&, const FAssetData& Asset)
{
	if (SoundBaseHandle.IsValid() && SoundBaseHandle->IsValidHandle())
	{
		if (Asset.IsValid())
		{
			UClass* AssetClass = Asset.GetClass();
			// Set bank default value
			if (AssetClass == UAtomSoundCue::StaticClass() ||
				AssetClass == UAtomSoundWave::StaticClass() ||
				AssetClass == UAtomSoundBase::StaticClass())
			{
				SoundBankHandle->SetValue(Asset);
			}
			else
			{
				SoundBankHandle->SetValue((UObject*)nullptr);;
			}

			SoundBaseHandle->SetValue(Asset);
		}
	}
}

EVisibility FAtomSoundBaseCustomization::IsSoundBankSelected() const
{
	if (!AtomSoundBaseUtils::IsPropertyValueChildOf(SoundBankHandle, UAtomSoundBank::StaticClass()))
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}

void FAtomSoundBaseCustomization::OnSoundBankPropertyChanged()
{
	if (auto Holder = Cast<UAtomSoundBankPropertyHolder_Internal>(SoundBankHolder.Get()))
	{
		if (UAtomSoundBase* SoundBase = Cast<UAtomSoundBase>(Holder->GetAsSoundBase()))
		{
			SoundBaseHandle->SetValue(SoundBase);
		}
	}
}

bool FAtomSoundBaseCustomization::CanCustomize(UClass* PropertyClass)
{
	if (PropertyClass == UAtomSoundBase::StaticClass()
		|| PropertyClass == UAtomSoundWave::StaticClass()
		|| PropertyClass == UAtomSoundCue::StaticClass())
	{
		return true;
	}

	return false;
}

TStrongObjectPtr<UParentObjectPropertyHolder_Base> FAtomSoundBaseCustomization::CreateSoundBankPropertyHolder(UClass* PropertyClass)
{
	TStrongObjectPtr<UParentObjectPropertyHolder_Base> PropertyHolderPtr;
	if (PropertyClass == UAtomSoundWave::StaticClass())
	{
		auto BankHolder = NewObject<UAtomWaveBankPropertyHolder_Internal>();
		BankHolder->Init();
		PropertyHolderPtr = TStrongObjectPtr<UParentObjectPropertyHolder_Base>(BankHolder);
	}
	else if (PropertyClass == UAtomSoundCue::StaticClass())
	{
		auto BankHolder = NewObject<UAtomCueSheetPropertyHolder_Internal>();
		BankHolder->Init();
		PropertyHolderPtr = TStrongObjectPtr<UParentObjectPropertyHolder_Base>(BankHolder);
	}
	else
	{
		auto BankHolder = NewObject<UAtomSoundBankPropertyHolder_Internal>();
		BankHolder->Init();
		PropertyHolderPtr = TStrongObjectPtr<UParentObjectPropertyHolder_Base>(BankHolder);
	}

	return PropertyHolderPtr;
}

#undef LOCTEXT_NAMESPACE
