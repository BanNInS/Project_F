/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomMultiPositionComponent.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"

#include "AtomComponent.h"
#include "AtomAttenuation.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

#include "AtomMultiPositionComponent.generated.h"

/**
 * Interface for AtomComponent extensibility
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UAtomMultiPositionComponentExtension : public UAtomComponentExtension
{
	GENERATED_BODY()
};

class IAtomMultiPositionComponentExtension : public IAtomComponentExtension
{
	GENERATED_BODY()

public:

	/** Sets the MultiPosition settings. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MultiPosition")
	void SetMultiPositionSettings(const FAtomMultiPositionSettings& MultiPositionSettings);
	
	/** Populates an array with current positions of all attenuation sources. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MultiPosition")
	void GetAllPositions(TArray<FTransform>& Positions) const;
};

/**
 * Settings proxy for AtomComponent extensibility
 */
UCLASS(MinimalAPI)
class UAtomMultiPositionComponentExtensionSettings
	: public UAtomComponentExtensionSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MultiPosition", meta = (ShowOnlyInnerProperties))
	FAtomMultiPositionSettings MultiPositionSettings;

	virtual TSubclassOf<UAtomComponent> GetAtomComponentClass() const override;
#if WITH_EDITOR
	virtual FName GetDisplayName() const override;
#endif
};

/**
 * AtomMultiPositionComponent is an AtomComponent using multiple source positions for attenuation.
 *
 * @see UAtomComponent
 */
UCLASS(ClassGroup = Atom, 
AutoExpandCategories = (Transform, StaticMesh, MultiPosition),
AutoCollapseCategories = (Physics, Collision, Lighting, Rendering, Cooking, Tags),
BlueprintType, Blueprintable,
meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomMultiPositionComponent
	: public UAtomComponent
	, public IAtomComponentExtension
{
	GENERATED_BODY()

public:

	UAtomMultiPositionComponent(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MultiPosition", meta = (ShowOnlyInnerProperties))
	FAtomMultiPositionSettings MultiPositionSettings;
	
	//~ Begin IAtomMultiPositionComponentExtension Interface
	
	/** Sets the MultiPosition settings. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MultiPosition")
	void SetMultiPositionSettings(const FAtomMultiPositionSettings& InMultiPositionSettings);
	void SetMultiPositionSettings_Implementation(const FAtomMultiPositionSettings& InMultiPositionSettings) { MultiPositionSettings = InMultiPositionSettings; }

	/** Populates an array with current positions of all attenuation sources. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MultiPosition")
	void GetAllPositions(TArray<FTransform>& Positions) const;
	void GetAllPositions_Implementation(TArray<FTransform>& Positions) const;

	virtual void ApplyExtensionSettings(UAtomComponentExtensionSettings* ExtensionSettings) override
	{
		if (auto MultiPositionExtensionSettings = Cast<UAtomMultiPositionComponentExtensionSettings>(ExtensionSettings))
		{
			MultiPositionSettings = MultiPositionExtensionSettings->MultiPositionSettings;
		}
	}

	//~ End IAtomMultiPositionComponentExtension Interface

	//~ Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent Interface

private:

	void UpdatePositions();
};
