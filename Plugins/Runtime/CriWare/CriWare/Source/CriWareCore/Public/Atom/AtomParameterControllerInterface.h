#pragma once

#include "AudioParameterControllerInterface.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#if 0
#include "AtomParameterControllerInterface.generated.h"


// Forward Declarations
class FAtomRuntime;
class USoundBase;
struct FActiveSound;

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class ENGINE_API UAtomParameterControllerInterface : public UAudioParameterControllerInterface
{
	GENERATED_UINTERFACE_BODY()
};

// UObject interface for all object types that are controlling parameter values sent
// to sound instances (i.e. sources)
class ENGINE_API IAtomParameterControllerInterface : public IAudioParameterControllerInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	// IAudioParameterControllerInterface
	void ResetParameters() override;

	void SetTriggerParameter(FName InName) override;
	void SetBoolParameter(FName InName, bool InBool) override;
	void SetBoolArrayParameter(FName InName, const TArray<bool>& InValue) override;
	void SetIntParameter(FName InName, int32 InInt) override;
	void SetIntArrayParameter(FName InName, const TArray<int32>& InValue) override;
	void SetFloatParameter(FName InName, float InFloat) override;
	void SetFloatArrayParameter(FName InName, const TArray<float>& InValue) override;
	void SetStringParameter(FName InName, const FString& InValue) override;
	void SetStringArrayParameter(FName InName, const TArray<FString>& InValue) override;
	//void SetObjectParameter(FName InName, UObject* InValue) override;
	//void SetObjectArrayParameter(FName InName, const TArray<UObject*>& InValue) override;

	void SetParameter(FAtomParameter&& InValue) override;
	void SetParameters(TArray<FAtomParameter>&& InValues) override;
	void SetParameters_Blueprint(const TArray<FAtomParameter>& InValues) override;

	/** Returns the active audio device to use for this component based on whether or not the component is playing in a world. */
	virtual FAtomRuntime* GetAtomRuntime() const = 0;

	/** Returns the id of the sound owner's instance associated with the parameter interface. */
	virtual uint64 GetInstanceOwnerID() const = 0;

	/** Returns the Game Thread copy of parameters to modify in place. */
	virtual TArray<FAtomParameter>& GetInstanceParameters() = 0;

	/** Returns the USoundBase used to initialize instance parameters to update. */
	virtual UAtomSoundBase* GetSound() = 0;

	virtual bool IsPlaying() const = 0;

	virtual bool GetDisableParameterUpdatesWhilePlaying() const = 0;
};

UCLASS()
class ENGINE_API UAtomParameterConversionStatics 
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter BooleanToAtomParameter(FName Name, bool Bool);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter FloatToAtomParameter(FName Name, float Float);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter IntegerToAtomParameter(FName Name, int32 Integer);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter StringToAtomParameter(FName Name, FString String);

	//UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	//static UPARAM(DisplayName = "Parameter") FAtomParameter ObjectToAtomParameter(FName Name, UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter BooleanArrayToAtomParameter(FName Name, TArray<bool> Bools);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter FloatArrayToAtomParameter(FName Name, TArray<float> Floats);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter IntegerArrayToAtomParameter(FName Name, TArray<int32> Integers);

	UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	static UPARAM(DisplayName = "Parameter") FAtomParameter StringArrayToAtomParameter(FName Name, TArray<FString> Strings);

	//UFUNCTION(BlueprintPure, Category = "Audio|Parameter", meta = (Keywords = "make construct convert create"))
	//static UPARAM(DisplayName = "Parameter") FAtomParameter ObjectArrayToAtomParameter(FName Name, TArray<UObject*> Objects);
};
#endif
