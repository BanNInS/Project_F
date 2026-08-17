/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomAmbientSound.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"

#include "AtomAmbientSound.generated.h"

class UAtomComponent;

/** A sound actor that can be placed in a level */
UCLASS(AutoExpandCategories = Atom, ClassGroup = Atom, HideCategories(Collision, Input, Game), ShowCategories=("Input|MouseInput", "Input|TouchInput", "Game|Damage"), ComponentWrapperClass)
class CRIWARECORE_API AAtomAmbientSound
	: public AActor
{
	GENERATED_BODY()

private:
	
	/** Audio component that handles sound playing */
	UPROPERTY(Category = Atom, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Atom,Atom|Components|Atom", AllowPrivateAccess = "true"))
	TObjectPtr<class UAtomComponent> AtomComponent;

public:

	AAtomAmbientSound(const FObjectInitializer& ObjectInitializer);

	FString GetInternalSoundCueName();

	//~ Begin AActor Interface.
#if WITH_EDITOR
	virtual void CheckForErrors() override;
	virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const override;
#endif
	virtual void PostRegisterAllComponents() override;
	//~ End AActor Interface.

public:

	/** Returns AudioComponent subobject **/
	class UAtomComponent* GetAtomComponent() const { return AtomComponent; }
};



