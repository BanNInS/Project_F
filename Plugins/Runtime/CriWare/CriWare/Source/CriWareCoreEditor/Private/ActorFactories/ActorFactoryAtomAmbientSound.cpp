/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : ActorFactoryAtomAmbientSound.cpp
 *
 ****************************************************************************/

#include "ActorFactoryAtomAmbientSound.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomAmbientSound.h"

#define LOCTEXT_NAMESPACE "AtomActorFactory"

/*
 * UActorFactoryAtomAmbientSound implementation.
 *****************************************************************************/

UActorFactoryAtomAmbientSound::UActorFactoryAtomAmbientSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = LOCTEXT("AtomAmbientSoundDisplayName", "Atom Ambient Sound");
	NewActorClass = AAtomAmbientSound::StaticClass();
}

bool UActorFactoryAtomAmbientSound::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	// We allow creating AAtomAmbientSounds without an existing sound asset.
	if (UActorFactory::CanCreateActorFrom(AssetData, OutErrorMsg))
	{
		return true;
	}

	if (AssetData.IsValid() && !AssetData.IsInstanceOf(UAtomSoundBase::StaticClass()))
	{
		OutErrorMsg = NSLOCTEXT("CanCreateActor", "NoAtomSoundAsset", "A valid Atom sound asset must be specified.");
		return false;
	}

	return true;
}

void UActorFactoryAtomAmbientSound::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	UAtomSoundBase* AmbientSound = Cast<UAtomSoundBase>(Asset);

	if (AmbientSound != nullptr)
	{
		AAtomAmbientSound* NewSound = CastChecked<AAtomAmbientSound>(NewActor);
		NewSound->GetAtomComponent()->SetSound(AmbientSound);
	}
}

UObject* UActorFactoryAtomAmbientSound::GetAssetFromActorInstance(AActor* Instance)
{
	check(Instance->IsA(NewActorClass));
	AAtomAmbientSound* SoundActor = CastChecked<AAtomAmbientSound>(Instance);

	check(SoundActor->GetAtomComponent());
	return SoundActor->GetAtomComponent()->Sound;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
void UActorFactoryAtomAmbientSound::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (Asset != nullptr && CDO != nullptr)
	{
		UAtomSoundBase* AmbientSound = Cast<UAtomSoundBase>(Asset);

		if (AmbientSound != nullptr)
		{
			AAtomAmbientSound* NewSound = CastChecked<AAtomAmbientSound>(CDO);
			NewSound->GetAtomComponent()->SetSound(AmbientSound);
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
