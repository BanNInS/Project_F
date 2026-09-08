/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareWidgetSubsystem.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "AssetRegistry/AssetData.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/Function.h"
#include "UObject/Class.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "CriWareWidgetSubsystem.generated.h"

// Forward Declarations
class UWorld;

UCLASS()
class CRIWARECORE_API UCriWareWidgetSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

private:

	static const TArray<FAssetData> GetBlueprintAssetData(UClass* InInterfaceClass);
	static bool ImplementsInterface(const FAssetData& InAssetData, UClass* InInterfaceClass);

public:

	// Returns user widgets that implement a widget interface of the given subclass and optionally, filters and returns only those which pass the provided filter function.
	TArray<UUserWidget*> CreateUserWidgets(UWorld& InWorld, TSubclassOf<UInterface> InWidgetInterface, TFunction<bool(UUserWidget*)> FilterFunction = TFunction<bool(UUserWidget*)>()) const;
};
