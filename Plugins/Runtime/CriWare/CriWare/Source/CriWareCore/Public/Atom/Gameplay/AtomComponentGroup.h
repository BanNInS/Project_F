
#pragma once

#include "Components/SceneComponent.h"

#include "AtomComponentGroupExtension.h"
#include "Atom/AtomAisacPatch.h"
#include "Extensions/IAtomAisacParameterController.h"

#include "AtomComponentGroup.generated.h"

#define CRI_API CRIWARECORE_API

template <typename InterfaceType> class TScriptInterface;

class UAtomSoundBase;
//class UParamCollection;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAtomSoundGroupChanged);
DECLARE_DYNAMIC_DELEGATE_OneParam(FAtomSoundCallback, const FName&, EventName);
//DECLARE_DYNAMIC_DELEGATE_OneParam(FBoolParamCallback, const bool, ParamValue);
//DECLARE_DYNAMIC_DELEGATE_OneParam(FStringParamCallback, const FString&, Value);

/*
 * Automatic Handler for voices and parameters across any number of AtomComponents
 */
UCLASS(ClassGroup = (Atom), MinimalAPI, BlueprintType, DefaultToInstanced, meta = (BlueprintSpawnableComponent))
class UAtomComponentGroup : public USceneComponent, public IAtomAisacParameterController
{
	GENERATED_BODY()

public:

	CRI_API UAtomComponentGroup(const FObjectInitializer& ObjectInitializer);

	virtual ~UAtomComponentGroup() = default;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	static CRI_API UAtomComponentGroup* StaticGetOrCreateComponentGroup(AActor* Actor);

	// UObject Interface
	CRI_API virtual void BeginPlay() override;
	CRI_API virtual void OnRegister() override;
	CRI_API virtual void OnUnregister() override;
#if WITH_EDITOR
	CRI_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Stop all instances of this Sound on any internal or external components
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void StopSound(UAtomSoundBase* Sound, const float FadeTime = 0.f);

	UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API bool IsPlayingAny() const;

	UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = AtomComponentGroup)
	bool IsVirtualized() const { return bIsVirtualized; }

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void BroadcastStopAll();

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void BroadcastKill();

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void BroadcastEvent(const FName EventName);

	// Component interaction
	CRI_API UAtomComponent* GetNextAvailableComponent();

	CRI_API UAtomComponent* AddComponent();
	CRI_API UAtomComponent* ResetComponent(UAtomComponent* Component) const;
	CRI_API void RemoveComponent(const UAtomComponent* InComponent);

	CRI_API void AddActorComponents(AActor* Actor, bool bIncludeFromChildActors = false);
	CRI_API void RemoveActorComponents(AActor* Actor);

	//Allow an externally created AtomComponent to share parameters with this SoundGroup
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void AddExternalComponent(UAtomComponent* ComponentToAdd);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void RemoveExternalComponent(UAtomComponent* ComponentToRemove);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void EnableVirtualization();

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void DisableVirtualization();

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	void SetVolumeMultiplier(const float InVolume) { GroupModifier.Volume = InVolume; }
	
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	void SetPitchMultiplier(const float InPitch) { GroupModifier.Pitch = InPitch; }

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	void SetLowPassFilter(const float InFrequency) { GroupModifier.LowPassFrequency = InFrequency; }

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	void SetHighPassFilter(const float InFrequency) { GroupModifier.HighPassFrequency = InFrequency; }

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	void SetSoundClass(UAtomSoundClass* SoundClass) { GroupSoundClass = SoundClass; }

	const TArray<FAtomAisacParameter>& GetParams() { return PersistentParams; }

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void AddExtension(TScriptInterface<IAtomComponentGroupExtension> NewExtension);
	
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void RemoveExtension(TScriptInterface<IAtomComponentGroupExtension> NewExtension);

	CRI_API void UpdateExtensions(const float DeltaTime);

	CRI_API virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	UFUNCTION(BlueprintPure, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API float GetAisacParamValue(const FAtomAisacControl& ParamControl) const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AtomComponentGroup)
	CRI_API void SubscribeToEvent(const FName EventName, FAtomSoundCallback Delegate);
	
	//~ Begin IAudioParameterControllerInterface interface 
	CRI_API virtual void ResetParameters() override;

	CRI_API virtual void SetTriggerParameter(const FAtomAisacControl& InControl) override;
	CRI_API virtual void SetAisacParameter(const FAtomAisacControl& InControl, float InFloat) override;

	CRI_API virtual void SetParameter(FAtomAisacParameter&& InValue) override;
	CRI_API virtual void SetParameters(TArray<FAtomAisacParameter>&& InValues) override;
	CRI_API virtual void SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InParameters) override;
	//~ End IAudioParameterControllerInterface interface

	UPROPERTY(BlueprintAssignable)
	FAtomSoundGroupChanged OnStopped;

	UPROPERTY(BlueprintAssignable)
	FAtomSoundGroupChanged OnKilled;

	UPROPERTY(BlueprintAssignable)
	FAtomSoundGroupChanged OnVirtualized;

	UPROPERTY(BlueprintAssignable)
	FAtomSoundGroupChanged OnUnvirtualized;

	/** Automatically adds root actor and attached actors Atom components as external components of this group. */
	UPROPERTY(EditAnywhere, Category = AtomComponentGroup)
	bool bAddAllActorAtomComponents = false;

	CRI_API void IterateComponents(const TFunction<void(UAtomComponent*)> OnIterate);

protected:

	CRI_API void ApplyParams(UAtomComponent* Component) const;
	CRI_API void ApplyModifiers(UAtomComponent* Component, const FAtomComponentModifier& Modifier) const;
	CRI_API void ApplySoundClass(UAtomComponent* Component, UAtomSoundClass* SoundClass) const;

	CRI_API void UpdateComponentParameters();

	CRI_API float GetComponentVolume() const;

	CRI_API void GetAllAtomComponents(TArray<TObjectPtr<UAtomComponent>>& OutComponents) const;

	//CRI_API void ExecuteStringParamSubscriptions(const FAudioParameter& StringParam);
	//CRI_API void ExecuteBoolParamSubscriptions(const FAudioParameter& BoolParam);
	CRI_API void ExecuteEventSubscriptions(const FName EventName);

	CRI_API const FAtomAisacParameter* GetParamInternal(const FAtomAisacControl& ParamControl) const;

	UPROPERTY(VisibleAnywhere, Category = AtomComponentGroup)
	TArray<TObjectPtr<UAtomComponent>> Components;

	UPROPERTY(Transient)
	TArray<FAtomAisacParameter> ParamsToSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomComponentGroup)
	TArray<FAtomAisacParameter> PersistentParams;

	// SoundClass for the group, can be set externally via BP functions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomComponentGroup)
	TObjectPtr<UAtomSoundClass> GroupSoundClass;

	UPROPERTY(Transient)
	TArray<TScriptInterface<IAtomComponentGroupExtension>> Extensions;

	// Modifier set externally via BP functions
	UPROPERTY(EditAnywhere, Category = AtomComponentGroup)
	FAtomComponentModifier GroupModifier;

	// final values set last update
	FAtomComponentModifier CachedModifier;

	UPROPERTY(VisibleAnywhere, Category = AtomComponentGroup)
	TArray<TWeakObjectPtr<AActor>> Actors;

	//Components managed externally that won't be used in the pool, but can still share parameters
	UPROPERTY(VisibleAnywhere, Category = AtomComponentGroup)
	TArray<TWeakObjectPtr<UAtomComponent>> ExternalComponents;

	//TMap<FName, TArray<FStringParamCallback>> StringSubscriptions;
	TMap<FName, TArray<FAtomSoundCallback>> EventSubscriptions;
	//TMap<FName, TArray<FBoolParamCallback>> BoolSubscriptions;

	bool bIsVirtualized = false;

	friend class FAtomComponentGroupDebug;
};

#undef CRI_API
