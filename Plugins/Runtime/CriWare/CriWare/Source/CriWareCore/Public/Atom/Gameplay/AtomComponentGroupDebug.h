
#pragma once

#include "HAL/Platform.h"

class FString;
struct FColor;
struct FAtomAisacParameter;

class AActor;
class UAtomComponentGroup;

class FAtomComponentGroupDebug
{
public:
	// Print debug info above the component group's owner, depending on which console variables are enabled
	static void CRIWARECORE_API DebugPrint(const UAtomComponentGroup* ComponentGroup);

	// print the names and values of parameters set on this component group
	static FString DebugPrintParams(const UAtomComponentGroup* ComponentGroup);

	// return a string with the name and value of an audio parameter, depending on which underlying type is held by the parameter
	static FString GetDebugParamString(const FAtomAisacParameter& Param);

	// green, or yellow if the component group is virtualized
	static FColor GetDebugStringColor(const bool bVirtualized);

	// print a list of all playing sounds on the component group
	static FString DebugPrintActiveSounds(const UAtomComponentGroup* ComponentGroup);

	// print the final debug string above the target actor
	static void PrintDebugString(const FString& InString, AActor* Owner, const FColor DrawColor);
};
