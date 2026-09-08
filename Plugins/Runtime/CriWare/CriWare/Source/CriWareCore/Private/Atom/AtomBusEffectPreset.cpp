/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectPreset.cpp
 *
 ****************************************************************************/

#include "Atom/AtomBusEffectPreset.h"

#if WITH_EDITOR
#include "ObjectTools.h" // todo: remove usage
#endif

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomBus.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectPreset"

TMap<FName, TPair<const UClass*, int32> > UAtomBusEffectPreset::BusEffects;

UAtomBusEffectPreset::UAtomBusEffectPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

UAtomBus* UAtomBusEffectPreset::GetBus() const
{
	return Cast<UAtomBus>(GetOuter());
}

void UAtomBusEffectPreset::PostInitProperties()
{
	// CDO register this effect to static map
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		RegisterBusEffectClass(EffectName, GetClass(), NumParameters);
	}

	Super::PostInitProperties();
}

void UAtomBusEffectPreset::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Init();
	}
}

#if WITH_EDITOR
void UAtomBusEffectPreset::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	if (DuplicateMode == EDuplicateMode::Normal)
	{
		TargetBuses.Empty();
	}
}

void UAtomBusEffectPreset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	//const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	//const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

static void GetEffectParameterIndexOfProperty(FPropertyChangedChainEvent& PropertyChangedEvent, FProperty* Property, FEditPropertyChain::TDoubleLinkedListNode* Node, int32& ParamIndex, int ArrayIndex)
{
	if (Property)
	{
		int NumElements = 1;
		ParamIndex++; // count itself

		// if current property is a struct
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			NumElements = 0;
			for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
			{
				NumElements++;
			}

			// check for next node
			FEditPropertyChain::TDoubleLinkedListNode* NextNode = Node->GetNextNode();
			FProperty* NextProperty = NextNode ? NextNode->GetValue() : nullptr;
			if (NextProperty)
			{
				// loop over properties and count until nextnode
				for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
				{
					if (NextProperty == *It)
					{
						GetEffectParameterIndexOfProperty(PropertyChangedEvent, NextProperty, NextNode, --ParamIndex, 0);
						break;
					}

					ParamIndex++;
				}
			}
		}
		// if current property is an array
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			const int32 NextArrayIndex = PropertyChangedEvent.GetArrayIndex(ArrayProperty->GetFName().ToString());

			// get the inner field - the inner property is the element of the array
			TArray<FField*> InnerProperties;
			Property->GetInnerFields(InnerProperties);
			check(InnerProperties.Num() == 1);

			// recall us with the inner property instead of the array property
			GetEffectParameterIndexOfProperty(PropertyChangedEvent, CastField<FProperty>(InnerProperties[0]), Node, --ParamIndex, NextArrayIndex);
		}

		ParamIndex += ArrayIndex * NumElements;
	}
}

void UAtomBusEffectPreset::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	FProperty* ModifiedProperty = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue();
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	//const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	const FName ModifiedPropertyName = ModifiedProperty ? ModifiedProperty->GetFName() : NAME_None;

	if (ModifiedPropertyName == FName(TEXT("Settings")))
	{
		Init();

		if (PropertyName == FName(TEXT("bBypass")))
		{
			// Apply to atom runtime for attached buses.
			UpdateBypass();
		}
		else if (FStructProperty* StructProp = CastField<FStructProperty>(ModifiedProperty))
		{
			FEditPropertyChain::TDoubleLinkedListNode* ActiveNode = PropertyChangedEvent.PropertyChain.GetActiveMemberNode();

			// Get the parameter index from property index
			int32 ParamIndex = INDEX_NONE;
			GetEffectParameterIndexOfProperty(PropertyChangedEvent, ModifiedProperty, ActiveNode, ParamIndex, 0);

			// Apply to atom runtime for attached buses.
			UpdateParameter(ParamIndex);

			// try to apply if preset is owned by a valid bus.
			/*UAtomBus* Bus = GetBus();
			if (Bus)
			{
				//Bus->ApplyEffectParameter();
			}
			// or if preset is used by a valid bus.
			if (GCriWare)
			{
				if (auto Atom = GCriWare->GetActiveAtom())
				{
					for (auto& AtomRack : Atom->AsrRacks)
					{
						UAtomRack* Rack = AtomRack.Key;
						for (auto Bus : Rack->Buses)
						{
							if (Bus->BusEffectChain.Contains(this))
							{
								//Bus->ApplyEffectParameter();
							}
						}
					}
				}
			}*/
		}
	}
}

#endif

UAtomBusEffectPreset* UAtomBusEffectPreset::CreateBusEffectPreset(UObject* InOuter, const FName& InBusEffectName)
{
	if (const UClass* PresetClass = GetBusEffectPresetClassFromName(InBusEffectName))
	{
#if WITH_EDITOR
		FString SanitizedName = ObjectTools::SanitizeObjectName(InBusEffectName.ToString()); // repalce with CriWareCoreUtils::SanitizeObjectName
		//BusEffectName = MakeUniqueObjectName(InOuter, PresetClass, FName(*SanitizedName));
		return CreateBusEffectPreset(InOuter, PresetClass, *SanitizedName);
#else
		FName BusEffectName = MakeUniqueObjectName(InOuter, PresetClass, InBusEffectName);
		return CreateBusEffectPreset(InOuter, PresetClass, BusEffectName);
#endif

		
	}

	return nullptr;
}

UAtomBusEffectPreset* UAtomBusEffectPreset::CreateBusEffectPreset(UObject* InOuter, const UClass* InPresetClass, const FName& InName /* = NAME_None */)
{
	if (InPresetClass && InPresetClass->IsChildOf(UAtomBusEffectPreset::StaticClass()))
	{
		FName BusEffectName = InName;
		if (BusEffectName.IsNone())
		{
			BusEffectName = GetBusEffectPresetNameFromClass(InPresetClass);
#if WITH_EDITOR
			FString SanitizedName = ObjectTools::SanitizeObjectName(BusEffectName.ToString()); // repalce with CriWareCoreUtils::SanitizeObjectName
			//BusEffectName = MakeUniqueObjectName(InOuter, PresetClass, FName(*SanitizedName));
			BusEffectName = *SanitizedName;
#else
			BusEffectName = MakeUniqueObjectName(InOuter, InPresetClass, BusEffectName);
#endif
		}

		return NewObject<UAtomBusEffectPreset>(InOuter, InPresetClass, BusEffectName, RF_Public);
	}

	return nullptr;
}

const UClass* UAtomBusEffectPreset::GetBusEffectPresetClassFromName(const FName& InBusEffectName)
{
	if (auto ClassPtr = BusEffects.Find(InBusEffectName))
	{
		return ClassPtr->Key;
	}

	return nullptr;
}

const FName UAtomBusEffectPreset::GetBusEffectPresetNameFromClass(const UClass* InBusEffectClass)
{
	for (auto& BusEffect : BusEffects)
	{
		if (BusEffect.Value.Key == InBusEffectClass)
		{
			return BusEffect.Key;
		}
	}

	return NAME_None;
}

int32 UAtomBusEffectPreset::GetBusEffectPresetNumParameters(const FName& InBusEffectName)
{
	if (auto ClassPtr = BusEffects.Find(InBusEffectName))
	{
		return ClassPtr->Value;
	}

	return 0;
}

void UAtomBusEffectPreset::Update() const
{
	for (auto Bus : TargetBuses)
	{
		Bus->ApplyEffectPreset(this);
	}
}

void UAtomBusEffectPreset::UpdateParameter(int ParamIndex) const
{
	for (auto Bus : TargetBuses)
	{
		Bus->ApplyEffectParameter(this, ParamIndex);
	}
}

void UAtomBusEffectPreset::UpdateBypass() const
{
	for (auto Bus : TargetBuses)
	{
		Bus->ApplyEffectBypass(this);
	}
}

void UAtomBusEffectPreset::AddTargetBus(const UAtomBus* InBus)
{
	if (!bInitialized)
	{
		bInitialized = true;
		Init();
	}

	TargetBuses.Add(InBus);
}

void UAtomBusEffectPreset::RemoveTargetBus(const UAtomBus* InBus)
{
	TargetBuses.Remove(InBus);
}

#undef LOCTEXT_NAMESPACE
