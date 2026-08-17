
#pragma once

#include "Atom/AtomVolume.h"

// Forward Declarations 
class FAtomProxyVolumeMutator;
struct FAtomProxyMutatorSearchResult;

/**
 *  FAtomInterpolatedInteriorSettings
 */
class FAtomInterpolatedInteriorSettings
{
public:

	FAtomInterpolatedInteriorSettings() = default;

	// Set a new interior settings target
	void Apply(const FAtomInteriorSettings& NewSettings);

	// Interpolates between the source and end
	float Interpolate(double CurrentTime, double EndTime) const;

	// Updates the current state of the interior settings
	void UpdateInteriorValues();

	FORCEINLINE void SetInteriorStartTime(double InTime) { InteriorStartTime = InTime; }

	FORCEINLINE double GetInteriorStartTime() const { return InteriorStartTime; }

	FORCEINLINE float GetInteriorVolume() const { return InteriorSettings.InteriorVolume; }
	FORCEINLINE float GetInteriorVolumeInterp() const { return InteriorVolumeInterp; }

	FORCEINLINE float GetExteriorVolume() const { return InteriorSettings.ExteriorVolume; }
	FORCEINLINE float GetExteriorVolumeInterp() const { return ExteriorVolumeInterp; }

	FORCEINLINE float GetInteriorLPF() const { return InteriorSettings.InteriorLPF; }
	FORCEINLINE float GetInteriorLPFInterp() const { return InteriorLPFInterp; }

	FORCEINLINE float GetExteriorLPF() const { return InteriorSettings.ExteriorLPF; }
	FORCEINLINE float GetExteriorLPFInterp() const { return ExteriorLPFInterp; }

	FORCEINLINE bool IsWorldSettings() const { return InteriorSettings.bIsWorldSettings; }

protected:

	double InteriorStartTime = 0.0;

	double InteriorEndTime = 0.0;
	double ExteriorEndTime = 0.0;

	double InteriorLPFEndTime = 0.0;
	double ExteriorLPFEndTime = 0.0;

	float InteriorVolumeInterp = 0.f;
	float ExteriorVolumeInterp = 0.f;

	float InteriorLPFInterp = 0.f;
	float ExteriorLPFInterp = 0.f;

	FAtomInteriorSettings InteriorSettings;
};

/**
 *  FAtomGameplayVolumeListener
 */
class FAtomGameplayVolumeListener
{
public:

	FAtomGameplayVolumeListener() = default;

	void Update(const FAtomProxyMutatorSearchResult& Result, const FVector& InPosition, uint32 InRuntimeID);

	const TSet<uint32>& GetCurrentProxies() const { return CurrentProxies; }

	uint32 GetOwningRuntimeID() const { return OwningRuntimeID; }

	const FAtomInterpolatedInteriorSettings& GetInteriorSettings() const { return InteriorSettings; }

	const FVector& GetPosition() const { return Position; }

protected:

	FAtomInterpolatedInteriorSettings InteriorSettings;

	uint32 OwningRuntimeID = INDEX_NONE;
	FVector Position = FVector::ZeroVector;

	TSet<uint32> CurrentProxies;
	TSet<uint32> PreviousProxies;

	TArray<TSharedPtr<FAtomProxyVolumeMutator>> ActiveMutators;
};

