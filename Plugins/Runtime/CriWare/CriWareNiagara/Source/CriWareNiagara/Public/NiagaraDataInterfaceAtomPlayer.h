
#pragma once

#include "Templates/Function.h"
#include "NiagaraCommon.h"
#include "NiagaraShared.h"
#include "NiagaraDataInterface.h"

#include "NiagaraDataInterfaceAtomPlayer.generated.h"

class UAtomComponent;
class UAtomSoundBase;
class UAtomAttenuation;
class UAtomConcurrency;
class UAtomAisacPatch;
struct FAtomAisacControlSettings;
struct FAtomPlayerInterface_InstanceData;

struct FAtomParticleData
{
	FVector Position;
	FRotator Rotation;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	float StartTime = 1.0f;
};

struct FPersistentAtomParticleData
{
	int32 AtomHandle = 0;

	/** The update callback is executed in PerInstanceTickPostSimulate, which runs on the game thread */
	TFunction<void(FAtomPlayerInterface_InstanceData*, UAtomComponent*, FNiagaraSystemInstance*)> UpdateCallback;
};

struct FAtomPlayerInterface_InstanceData
{
	/** We use a lock-free queue here because multiple threads might try to push data to it at the same time. */
	TQueue<FAtomParticleData, EQueueMode::Mpsc> PlayAtomQueue;
	TQueue<FPersistentAtomParticleData, EQueueMode::Mpsc> PersistentAtomActionQueue;
	FThreadSafeCounter HandleCount;

	TSortedMap<int32, TWeakObjectPtr<UAtomComponent>> PersistentAtomMapping;

	TWeakObjectPtr<UAtomSoundBase> SoundToPlay;
	TWeakObjectPtr<UAtomAttenuation> Attenuation;
	TWeakObjectPtr<UAtomConcurrency> Concurrency;
	TArray<TWeakObjectPtr<UAtomAisacPatch>> AdditionalAisacPatches;
	TArray<FAtomAisacControlSettings> AisacControls;

	FNiagaraLWCConverter LWCConverter;
	int32 MaxPlaysPerTick = 0;
	bool bStopWhenComponentIsDestroyed = true;
	bool bValidOneShotSound = false;
#if WITH_EDITORONLY_DATA
	bool bOnlyActiveDuringGameplay = false;
#endif
};

/** This Data Interface can be used to play one-shot audio effects driven by particle data using CriWare ADX. */
UCLASS(EditInlineNew, Category = "Sound", meta = (DisplayName = "Atom Player"))
class CRIWARENIAGARA_API UNiagaraDataInterfaceAtomPlayer : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:

	UNiagaraDataInterfaceAtomPlayer(const FObjectInitializer& ObjectInitializer);

	/** Reference to the Atom audio asset to play */
	UPROPERTY(EditAnywhere, Category = "Sound")
    TObjectPtr<UAtomSoundBase> SoundToPlay;

	/** Optional sound attenuation setting to use */
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<UAtomAttenuation> Attenuation;

	/** Optional sound concurrency setting to use */
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<UAtomConcurrency> Concurrency;

	/** A set of global ASIAC patches to add that can be used when setting the value on persistent Atom. */
	UPROPERTY(EditAnywhere, Category = "AISAC")
	TArray<TObjectPtr<UAtomAisacPatch>> AdditionalAisacPatches;
	
	/** A set of ASIAC controls setting that can be referenced via the control name when setting the value on persistent Atom. */
	UPROPERTY(EditAnywhere, Category = "AISAC")
	TArray<FAtomAisacControlSettings> AisacControls;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sound", meta = (InlineEditConditionToggle))
    bool bLimitPlaysPerTick;

	/** This sets the max number of sounds played each tick.
	 *  If more particles try to play a sound in a given tick, then it will play sounds until the limit is reached and discard the rest.
	 *  The particles to discard when over the limit are *not* chosen in a deterministic way. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sound", meta=(EditCondition="bLimitPlaysPerTick", ClampMin="0", UIMin="0"))
    int32 MaxPlaysPerTick;

	/** If false then it the Atom component keeps playing after the niagara component was destroyed. Looping sounds are always stopped when the component is destroyed. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sound")
	bool bStopWhenComponentIsDestroyed = true;

	/** Playing looping sounds as persistent Atom audio is not a problem, as the sound is stopped when a particle dies, but one-shot audio outlives the niagara system and can never be stopped. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sound")
	bool bAllowLoopingOneShotSounds = false;

#if WITH_EDITORONLY_DATA
	/** If true then this data interface only processes sounds during active gameplay. This is useful when you are working in the preview window and the sounds annoy you. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sound")
	bool bOnlyActiveDuringGameplay = false;

	virtual bool UpgradeFunctionCall(FNiagaraFunctionSignature& FunctionSignature) override;
#endif
	
	//UObject Interface
	virtual void PostInitProperties() override;
	//UObject Interface End

	//UNiagaraDataInterface Interface
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction &OutFunc) override;
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual int32 PerInstanceDataSize() const override { return sizeof(FAtomPlayerInterface_InstanceData); }
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual bool PerInstanceTickPostSimulate(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target) const override { return Target == ENiagaraSimTarget::CPUSim; }

	virtual bool HasPreSimulateTick() const override { return true; }
	virtual bool HasPostSimulateTick() const override { return true; }
	virtual bool PostSimulateCanOverlapFrames() const { return false; }
	//UNiagaraDataInterface Interface

	virtual void PlayOneShotAtom(FVectorVMExternalFunctionContext& Context);
	virtual void PlayPersistentAtom(FVectorVMExternalFunctionContext& Context);
	//virtual void SetParameterBool(FVectorVMExternalFunctionContext& Context);
	//virtual void SetParameterInteger(FVectorVMExternalFunctionContext& Context);
	//virtual void SetParameterFloat(FVectorVMExternalFunctionContext& Context);
	virtual void SetAisacControlValue(FVectorVMExternalFunctionContext& Context);
	virtual void UpdateVolume(FVectorVMExternalFunctionContext& Context);
	virtual void UpdatePitch(FVectorVMExternalFunctionContext& Context);
	virtual void UpdateLocation(FVectorVMExternalFunctionContext& Context);
	virtual void UpdateRotation(FVectorVMExternalFunctionContext& Context);
	virtual void SetPausedState(FVectorVMExternalFunctionContext& Context);

protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;
	
private:
	static const FName PlayAtomName;
	static const FName PlayPersistentAtomName;
	static const FName SetPersistentAtomVolumeName;
	static const FName SetPersistentAtomPitchName;
	static const FName SetPersistentAtomLocationName;
	static const FName SetPersistentAtomRotationName;
	//static const FName SetPersistentAtomBoolParamName;
	//static const FName SetPersistentAtomIntegerParamName;
	//static const FName SetPersistentAtomFloatParamName;
	static const FName SetPersistentAtomAisacControlValueName;
	static const FName PausePersistentAtomName;
};
