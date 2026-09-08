/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomAmbientSound.cpp
 *
 ****************************************************************************/

#include "Atom/AtomAmbientSound.h"

#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Misc/MapErrors.h"
#include "Misc/CommandLine.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomAmbientSound)

#define LOCTEXT_NAMESPACE "AtomAmbientSound"

/*
 * AAtomAmbientSound implementation.
 ******************************************************************************/

AAtomAmbientSound::AAtomAmbientSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AtomComponent = CreateDefaultSubobject<UAtomComponent>(TEXT("AtomComponent0"));

	AtomComponent->bAutoActivate = true;
	AtomComponent->bStopWhenOwnerDestroyed = true;
	AtomComponent->bShouldRemainActiveIfDropped = true;
	AtomComponent->Mobility = EComponentMobility::Movable;

	RootComponent = AtomComponent;

	bReplicates = false;
	SetHidden(true);
	SetCanBeDamaged(false);
}

#if WITH_EDITOR
void AAtomAmbientSound::CheckForErrors( void )
{
	Super::CheckForErrors();

	if (!AtomComponent)
	{
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_AtomComponentNull", "Atom Ambient sound actor has null AtomComponent property. - Please delete.")))
			->AddToken(FMapErrorToken::Create(FMapErrors::AudioComponentNull));
	}
	else if (AtomComponent->Sound == nullptr)
	{
		FMessageLog("MapCheck").Warning()
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_AtomSoundCueNull", "Atom Ambient sound actor has null Sound Cue property.")))
			->AddToken(FMapErrorToken::Create(FMapErrors::SoundCueNull));
	}
}

bool AAtomAmbientSound::GetReferencedContentObjects( TArray<UObject*>& Objects ) const
{
	Super::GetReferencedContentObjects(Objects);

	if (AtomComponent->Sound)
	{
		Objects.Add(AtomComponent->Sound);
	}
	return true;
}

#endif

void AAtomAmbientSound::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

#if WITH_EDITORONLY_DATA
	if (AtomComponent && bHiddenEdLevel)
	{
		AtomComponent->Stop();
	}
#endif // WITH_EDITORONLY_DATA
}

FString AAtomAmbientSound::GetInternalSoundCueName()
{
	FString CueName;
#if WITH_EDITOR
	CueName = GetActorLabel();
#endif
	if (CueName.Len() == 0)
	{
		CueName = GetName();
	}
	CueName += TEXT("_SoundCue");

	return CueName;
}

#undef LOCTEXT_NAMESPACE
