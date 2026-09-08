#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "AtomAisacPatch.generated.h"

// Forward Definitions
class UAtomConfig;
class UAtomModulatorBase;
struct FAtomAisacControlSettings;

/** Struct that define an AISAC control. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAisacControl
{
	GENERATED_BODY()

public:

	FAtomAisacControl()
		: ID(INDEX_NONE)
	{}

	FAtomAisacControl(FName InName, int32 InID)
		: Name(InName)
		, ID(InID)
	{}

	bool operator==(const FAtomAisacControl& Other) const
	{
		return ID == Other.ID;
	}

	bool operator!=(const FAtomAisacControl& Other) const
	{
		return !(*this == Other);
	}

	bool IsValid() const { return ID != INDEX_NONE; }

	/** Name of the AISAC control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	FName Name;

	/** Unique Atom Craft ID of the AISAC control. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	int32 ID;
};

inline uint32 GetTypeHash(FAtomAisacControl A) { return static_cast<uint32>(A.ID); }

/** Class to fully define an AISAC patch. */
UCLASS(HideCategories = Object, BlueprintType)
class CRIWARECORE_API UAtomAisacPatch
	: public UObject
{
	GENERATED_BODY()

public:

	void Init(FName InName, bool InDefaultControlFlag, float InDefaultControlValue, const FAtomAisacControl& InControl)
	{
		Name = InName;
		DefaultControlFlag = InDefaultControlFlag;
		DefaultControlValue = InDefaultControlValue;
		Control = InControl;
	}

	/** Name of AISAC patch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	FName Name;

	/** Flag wether a control value is specified. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	bool DefaultControlFlag;

	/** Default control value when specified. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	float DefaultControlValue;

	/** The control used */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AISAC")
	FAtomAisacControl Control;

	/** Gets the Atom configuaration this AISAC patch belong to. */
	UFUNCTION(BlueprintCallable, Category = "Config")
	UAtomConfig* GetConfig() const;
};

/** An AISAC parameter is the AISAC control and its value. */
USTRUCT(BlueprintType)
struct FAtomAisacParameter
{
	GENERATED_BODY()

public:

	FAtomAisacParameter() = default;

	FAtomAisacParameter(const FAtomAisacControl& InControl, float InValue)
		: Control(InControl)
		, Value(InValue)
	{}

	/** The AISAC value to target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISAC")
	FAtomAisacControl Control;

	/** The AISAC value to target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISAC", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float Value = 0.0f;

	static void Merge(TArray<FAtomAisacParameter>&& InParams, TArray<FAtomAisacParameter>& OutParams);

	// Common find algorithm
	static const FAtomAisacParameter* FindParam(const TArray<FAtomAisacParameter>& InParams, const FAtomAisacControl& InParamControl)
	{
		if (InParamControl.IsValid())
		{
			for (const FAtomAisacParameter& ExistingParam : InParams)
			{
				if (ExistingParam.Control == InParamControl)
				{
					return &ExistingParam;
				}
			}
		}

		return nullptr;
	}

	// Common find & add algorithm
	static FAtomAisacParameter* FindOrAddParam(TArray<FAtomAisacParameter>& OutParams, const FAtomAisacControl& InParamControl)
	{
		FAtomAisacParameter* Param = nullptr;
		if (!InParamControl.IsValid())
		{
			return Param;
		}

		for (FAtomAisacParameter& ExistingParam : OutParams)
		{
			if (ExistingParam.Control == InParamControl)
			{
				Param = &ExistingParam;
				break;
			}
		}

		if (!Param)
		{
			Param = &OutParams.AddDefaulted_GetRef();
			Param->Control = InParamControl;
		}

		return Param;
	}
};
