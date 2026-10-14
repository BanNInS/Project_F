/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : ActorFactoryAtomAmbientSound.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActorFactories/ActorFactory.h"
#include "Runtime/Launch/Resources/Version.h"

#include "ActorFactoryAtomAmbientSound.generated.h"

class AActor;
struct FAssetData;

/**
 * The base class of all Atom ambient sound types
 *****************************************************************************/

UCLASS(MinimalAPI, Config = Editor, CollapseCategories, HideCategories = Object)
class UActorFactoryAtomAmbientSound
	: public UActorFactory
{
	GENERATED_BODY()

public:

	UActorFactoryAtomAmbientSound(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorFactory Interface
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
#endif
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};
