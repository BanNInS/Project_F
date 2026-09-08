
#pragma once

#include "UObject/Interface.h"

#include "CriWareDefines.h"

#include "AtomComponentGroupExtension.generated.h"

class UAtomComponent;
class UAtomComponentGroup;

USTRUCT()
struct FAtomComponentModifier
{
	GENERATED_BODY()

	/** Modifies the volume multiplier over all AtomComponents. */
	UPROPERTY(EditAnywhere, Category = Modifier)
	float Volume = 1.f;

	/** Modifies the pitch factor over all AtomComponents. */
	UPROPERTY(EditAnywhere, Category = Modifier)
	float Pitch = 1.f;

	/** Modifies the low-pass frequency if filter is enabled in AtomComponent. */
	UPROPERTY(EditAnywhere, Category = Modifier)
	float LowPassFrequency = ATOM_MAX_FILTER_FREQUENCY;

	/** Modifies the high-pass frequency if filter is enabled in AtomComponent. */
	UPROPERTY(EditAnywhere, Category = Modifier)
	float HighPassFrequency = ATOM_MIN_FILTER_FREQUENCY;

	void Combine(const FAtomComponentModifier& Other)
	{
		Volume *= Other.Volume;
		Pitch *= Other.Pitch;
		LowPassFrequency = FMath::Min(LowPassFrequency, Other.LowPassFrequency);
		HighPassFrequency = FMath::Max(HighPassFrequency, Other.HighPassFrequency);
	}

	bool IsNearlyEqual(const FAtomComponentModifier& Other) const
	{
		return FMath::IsNearlyEqual(Volume, Other.Volume)
			&& FMath::IsNearlyEqual(Pitch, Other.Pitch)
			&& FMath::IsNearlyEqual(LowPassFrequency, Other.LowPassFrequency)
			&& FMath::IsNearlyEqual(HighPassFrequency, Other.HighPassFrequency);
	}

	static FAtomComponentModifier& Default()
	{
		static FAtomComponentModifier DefaultModifier;
		return DefaultModifier;
	}
};

UINTERFACE(MinimalAPI, BlueprintType)
class UAtomComponentGroupExtension : public UInterface
{
	GENERATED_BODY()
};

class IAtomComponentGroupExtension : public IInterface
{
	GENERATED_BODY()

public:

	virtual void Update(const float DeltaTime, UAtomComponentGroup* Group, FAtomComponentModifier& OutModifier) {}

	virtual void OnAddedToGroup(UAtomComponentGroup* NewGroup) {}

	virtual void OnComponentAdded(UAtomComponent* NewComponent) {}
};
