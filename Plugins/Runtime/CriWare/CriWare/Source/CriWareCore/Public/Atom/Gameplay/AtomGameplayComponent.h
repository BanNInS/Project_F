// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "AtomGameplayFlags.h"

#include "AtomGameplayComponent.generated.h"

UCLASS(Blueprintable, Config = Game, meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomGameplayComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:

	using PayloadFlags = AtomGameplay::EComponentPayload;

	UAtomGameplayComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayComponent() = default;

	//~ Begin UActorComponent interface
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

	virtual bool HasPayloadType(PayloadFlags InType) const;

protected:

	virtual void Enable();
	virtual void Disable();

	PayloadFlags PayloadType = PayloadFlags::AGCP_None;
};
