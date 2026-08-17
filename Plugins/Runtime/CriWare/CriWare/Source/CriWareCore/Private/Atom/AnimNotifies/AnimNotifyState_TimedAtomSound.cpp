/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AnimNotifyState_TimedAtomSound.cpp
 *
 ****************************************************************************/

#include "Atom/AnimNotifies/AnimNotifyState_TimedAtomSound.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif

#include "Atom/AtomGameplayStatics.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundBase.h"

/* UAnimNotifyState_TimedAtomSound
 *****************************************************************************/

UAnimNotifyState_TimedAtomSound::UAnimNotifyState_TimedAtomSound()
	: Super()
{
	VolumeMultiplier = 1.f;
	PitchMultiplier = 1.f;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(196, 142, 255, 255);
	bPreviewIgnoreAttenuation = false;
#endif // WITH_EDITORONLY_DATA
}

void UAnimNotifyState_TimedAtomSound::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
}

void UAnimNotifyState_TimedAtomSound::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
    NotifyBegin(MeshComp, Animation, TotalDuration);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	// Don't call super to avoid call back in to blueprints
	if (Sound && MeshComp)
	{
		// looping OK 
		//if (!Sound->IsOneShot())
		//{
		//	UE_LOG(LogCriWareAtom, Warning, TEXT("PlayAtomSoundGate notify: Anim %s tried to play an Atom Sound asset which is not a one-shot: '%s'. Spawning suppressed."), *GetNameSafe(Animation), *GetNameSafe(Sound));
		//	return;
		//}

		UAtomComponent* AtomComponent = nullptr;
#if WITH_EDITORONLY_DATA
		UWorld* World = MeshComp->GetWorld();
		if (bPreviewIgnoreAttenuation && World && World->WorldType == EWorldType::EditorPreview)
		{
			AtomComponent = UAtomGameplayStatics::SpawnSound2D(World, Sound, VolumeMultiplier, PitchMultiplier);
		}
		else
#endif
		{
			if (bFollow)
			{
				if (!bStatic)
				{
					AtomComponent = UAtomGameplayStatics::SpawnSoundAttached(Sound, MeshComp, AttachName, FVector(ForceInit), EAttachLocation::SnapToTarget, false, VolumeMultiplier, PitchMultiplier);
				}
				else
				{
					AtomComponent = UAtomGameplayStatics::SpawnSoundAtLocation(MeshComp->GetWorld(), Sound, MeshComp->GetSocketLocation(AttachName), FRotator::ZeroRotator, VolumeMultiplier, PitchMultiplier);
				}
			}
			else
			{
				AtomComponent = UAtomGameplayStatics::SpawnSoundAtLocation(MeshComp->GetWorld(), Sound, MeshComp->GetComponentLocation(), FRotator::ZeroRotator, VolumeMultiplier, PitchMultiplier);
			}
		}

		if (AtomComponent)
		{
			AtomComponent->StopDelayed(TotalDuration);
		}
	}
}

void UAnimNotifyState_TimedAtomSound::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
}

void UAnimNotifyState_TimedAtomSound::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
    NotifyEnd(MeshComp, Animation);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	// Don't call super to avoid call back in to blueprints
}

FString UAnimNotifyState_TimedAtomSound::GetNotifyName_Implementation() const
{
	if (Sound)
	{
		return Sound->GetName();
	}
	else
	{
		return Super::GetNotifyName_Implementation();
	}
}

#if WITH_EDITOR
void UAnimNotifyState_TimedAtomSound::ValidateAssociatedAssets()
{
	// Looping OK
	//static const FName NAME_AssetCheck("AssetCheck");
	
	//if (Sound != nullptr && !Sound->IsOneShot())
	//{
	//	UObject* ContainingAsset = GetContainingAsset();

	//	FMessageLog AssetCheckLog(NAME_AssetCheck);

	//	const FText MessageLooping = FText::Format(
	//		NSLOCTEXT("AnimNotify", "AtomSound_ShouldNotLoop", "Atom Sound {0} used in anim notify for asset {1} is set to looping, but the slot is a one-shot (it won't be played to avoid leaking an instance per notify)."),
	//		FText::AsCultureInvariant(Sound->GetPathName()),
	//		FText::AsCultureInvariant(ContainingAsset->GetPathName()));
	//	AssetCheckLog.Warning()
	//		->AddToken(FUObjectToken::Create(ContainingAsset))
	//		->AddToken(FTextToken::Create(MessageLooping));

	//	if (GIsEditor)
	//	{
	//		AssetCheckLog.Notify(MessageLooping, EMessageSeverity::Warning, /*bForce=*/ true);
	//	}
	//}
}
#endif
