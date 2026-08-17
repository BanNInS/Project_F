
#include "Atom/Gameplay/AtomComponentGroupDebug.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

#include "Atom/Gameplay/AtomComponentGroup.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundBase.h"

namespace AtomComponentGroupCVars 
{
	int32 PrintSoundParams = 0;
	static FAutoConsoleVariableRef CVarPrintSoundParams(
		TEXT("atom.AtomComponentGroup.debug.PrintSoundParams"),
		PrintSoundParams,
		TEXT("Set to 1 to print sound parameters for each actor.\n0: Disable (default), 1: Enable"),
		ECVF_Cheat );

	int32 PrintActiveSounds = 0;
	static FAutoConsoleVariableRef CVarPrintActiveSounds(
		TEXT("atom.AtomComponentGroup.debug.PrintActiveSounds"),
		PrintActiveSounds,
		TEXT("Display the active sounds on each actor.\n0: Disable (default), 1: Enable"),
		ECVF_Cheat );

	FString PrintSoundParamsActorFilter = FString(TEXT("All"));
	static FAutoConsoleVariableRef CVarPrintSoundParamsActorFilter(
		TEXT("atom.AtomComponentGroup.debug.ActorFilter"),
		PrintSoundParamsActorFilter,
		TEXT("Only display params on actors that contain this string. Disable filtering with All"),
		ECVF_Cheat );

	FString PrintSoundParamsParamFilter = FString(TEXT("All"));
	static FAutoConsoleVariableRef CVarPrintSoundParamsParamFilter(
		TEXT("atom.AtomComponentGroup.debug.ParamFilter"),
		PrintSoundParamsParamFilter,
		TEXT("Only display params with names that contain this string. Disable filtering with All"),
		ECVF_Cheat );

	float ZOffset = 150.0f;
	static FAutoConsoleVariableRef CVarPrintZOffset(
		TEXT("atom.AtomComponentGroup.debug.ActorZOffset"),
		ZOffset,
		TEXT("How High above an actor to print debug info. Default: 150"),
		ECVF_Cheat );
}

void FAtomComponentGroupDebug::DebugPrint(const UAtomComponentGroup* ComponentGroup)
{
	using namespace AtomComponentGroupCVars ;
	
	if(ComponentGroup == nullptr)
	{
		return;
	}
	
	if(LIKELY(PrintSoundParams <= 0 && PrintActiveSounds <= 0))
	{
		return;
	}

	const AActor* ComponentGroupOwner = ComponentGroup->GetOwner();
	check(ComponentGroupOwner);
	
	if (PrintSoundParamsActorFilter.Equals(TEXT("All"), ESearchCase::IgnoreCase) == false 
		&& ComponentGroupOwner->GetName().Contains(PrintSoundParamsActorFilter) == false)
	{
		return;
	}

	FString OutString = ComponentGroup->GetOwner()->GetName();
	OutString += LINE_TERMINATOR;
	
	if (UNLIKELY(PrintSoundParams >= 1))
	{
		OutString += DebugPrintParams(ComponentGroup);
	}

	if(UNLIKELY(PrintActiveSounds >= 1))
	{
		OutString += DebugPrintActiveSounds(ComponentGroup);
	}

	PrintDebugString(OutString, ComponentGroup->GetOwner(), GetDebugStringColor(ComponentGroup->bIsVirtualized));
}

FString FAtomComponentGroupDebug::DebugPrintParams(const UAtomComponentGroup* ComponentGroup)
{
	using namespace AtomComponentGroupCVars ;

	FString OutString;

	const bool bPrintAllParams = PrintSoundParamsParamFilter.Equals(TEXT("All"), ESearchCase::IgnoreCase);
	
	for (const FAtomAisacParameter& Param : ComponentGroup->PersistentParams)
	{
		if (bPrintAllParams || Param.Control.Name.ToString().Contains(PrintSoundParamsParamFilter))
		{
			OutString += GetDebugParamString(Param);
		}
	}

	OutString += LINE_TERMINATOR;

	return OutString;
}

FColor FAtomComponentGroupDebug::GetDebugStringColor(const bool bVirtualized)
{
	if(bVirtualized)
	{
		return FColor::Yellow;
	}

	return FColor::Green;
}

FString FAtomComponentGroupDebug::GetDebugParamString(const FAtomAisacParameter& Param)
{
	return FString::Printf(TEXT("%s : %f \n"), *Param.Control.Name.ToString(), Param.Value);
}

FString FAtomComponentGroupDebug::DebugPrintActiveSounds(const UAtomComponentGroup* ComponentGroup)
{
	FString OutString;

	for(const UAtomComponent* Component : ComponentGroup->Components)
	{
		if(Component && Component->Sound && Component->IsActive())
		{
			OutString += FString::Printf(TEXT("%s \n"), *(Component->Sound->GetName()));
		}
	}

	for(const TWeakObjectPtr<UAtomComponent>& Component : ComponentGroup->ExternalComponents)
	{
		if(Component.IsValid() && Component->Sound && Component->IsActive())
		{
			OutString += FString::Printf(TEXT("%s \n"), *(Component->Sound->GetName()));
		}
	}

	OutString += LINE_TERMINATOR;

	return OutString;
}

void FAtomComponentGroupDebug::PrintDebugString(const FString& InString, AActor* Owner, const FColor DrawColor)
{
	if(Owner == nullptr)
	{
		return;
	}
	
	DrawDebugString(Owner->GetWorld(), FVector(0.0f, 0.0f, AtomComponentGroupCVars ::ZOffset), *InString, Owner, DrawColor, 0.0f, true);
}
