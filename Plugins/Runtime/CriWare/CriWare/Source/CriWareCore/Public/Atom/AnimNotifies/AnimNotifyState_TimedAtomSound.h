/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AnimNotifyState_TimedAtomSound.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "AnimNotifyState_TimedAtomSound.generated.h"

// Forward Declarations
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UAtomComponent;
class UAtomSoundBase;

UCLASS(const, HideCategories = Object, CollapseCategories, Config = Game, meta = (DisplayName = "Timed Atom Sound"))
class CRIWARECORE_API UAnimNotifyState_TimedAtomSound
	: public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UAnimNotifyState_TimedAtomSound();

	//~ Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	UE_DEPRECATED(5.0, "Please use the other Notify function instead")
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	UE_DEPRECATED(5.0, "Please use the other Notify function instead")
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
	//~ End UAnimNotify interface

	// Sound to Play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta = (ExposeOnSpawn = true))
	TObjectPtr<UAtomSoundBase> Sound;

	// Volume Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta = (ExposeOnSpawn = true))
	float VolumeMultiplier;

	// Pitch Multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AnimNotify", meta = (ExposeOnSpawn = true))
	float PitchMultiplier;

	// If this sound should follow its owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	uint32 bFollow:1;

	// If this sound should remain static after being spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (EditCondition = "bFollow"))
	uint32 bStatic:1;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "AnimNotify")
	uint32 bPreviewIgnoreAttenuation:1;
#endif

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (EditCondition = "bFollow", ExposeOnSpawn = true))
	FName AttachName;
};
