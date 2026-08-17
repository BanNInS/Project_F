
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"

#include "Atom/Atom.h"

#include "AtomSoundscapeColorPoint.h"

#include "AtomSoundscapeSubsystem.generated.h"

class UAtomSoundscapePalette;
class UAtomActiveSoundscapePalette;

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareSoundscapeSubsystem, Log, All);

// Struct 
USTRUCT(BlueprintType)
struct CRIWARESOUNDSCAPE_API FAtomSoundscapePaletteCollection
{
	GENERATED_BODY()

		// Soundscape Palette Collection
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette", meta = (AllowedClasses = "/Script/Soundscape.AtomSoundscapePalette"))
		TSet<FSoftObjectPath> SoundscapePaletteCollection;
};

// Struct 
USTRUCT()
struct CRIWARESOUNDSCAPE_API FAtomSoundscapePaletteCollectionLoaded
{
	GENERATED_BODY()

	// Soundscape Palette Collection
	UPROPERTY(EditAnywhere, Category = "Soundscape|Palette")
	TSet<TObjectPtr<UAtomSoundscapePalette>> SoundscapePaletteCollection;
};

// Struct
USTRUCT(BlueprintType)
struct CRIWARESOUNDSCAPE_API FAtomSoundscapeColorPointCollection
{
	GENERATED_BODY()

	// Soundscape Color Point Collection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint")
	TArray<FAtomSoundscapeColorPointVectorArray> ColorPointCollection;
};

// Class
UCLASS()
class CRIWARESOUNDSCAPE_API UAtomSoundscapeColorPointHashCellDensity : public UObject
{
	GENERATED_BODY()

public:

	// Soundscape Color Point Density for a Hash Cell
	UPROPERTY(EditAnywhere, Category = "Soundscape|ColorPoint")
	TMap<uint64, int32> ColorPointHashCellDensity;

};

UCLASS()
class CRIWARESOUNDSCAPE_API UAtomSoundscapeColorPointHashMap : public UObject
{
	GENERATED_BODY()

public:
	// Clears and initializes hash map, sizes grid to HashCellWidth in Uunits
	UFUNCTION()
	void InitializeHash(float HashCellSizeIn, FVector GridCenterIn);

	// Clears hash map
	UFUNCTION()
	void ClearHash();

	// Returns the number of ColorPoints in a Cell containing the Location
	UFUNCTION()
	int32 NumColorPointsInCell(const FVector& Location, const FGameplayTag ColorPoint);

	// Returns true if ColorPoint added to hash, false if failed (likely location is out of Hash Bounds)
	UFUNCTION()
	bool AddColorPointToHash(const FVector& Location, const FGameplayTag ColorPoint);

	// Returns true if ColorPoint added to hash, false if failed (likely location is out of Hash Bounds)
	UFUNCTION()
	void AddColorPointArrayToHash(const TArray<FVector>& Locations, const FGameplayTag ColorPoint);

	// Calculates Hash Index
	UFUNCTION()
	uint64 CalculateHashIndex(const FVector& Location);

	// Sets ceterpoint of grid to Location
	UFUNCTION()
	void SetGridCenterpoint(const FVector& Location);

	TMap<FGameplayTag, int32> GetHashMapColorPointDensitySummary();
	
protected:
	// Returns ColorPointDensityMap from Hash Map given a Color Point
	UAtomSoundscapeColorPointHashCellDensity* GetColorPointDensityMap(FGameplayTag ColorPoint);

private:
	// Color Point Hash Map
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UAtomSoundscapeColorPointHashCellDensity>> ColorPointHashMap;

	// Cubic Root of the Max Value of int64 rounded down to the nearest million
	static const int32 MaxGridWidth = 2000000;
	static const int32 HalfMaxGridWidth = 1000000;

	// Hash Cell Width
	UPROPERTY()
	float HashCellSize = 500.0f;

	// MaxGridWidth / Hash Cell Size
	UPROPERTY()
	int32 GridWidth = 4000;

	// Grid Width Cubed
	UPROPERTY()
	int64 NumCells = 64000000000;

	// Cell Size Inverted
	UPROPERTY()
	float HashCellFactor = 0.002f;

	// Current Grid Centerpoint
	UPROPERTY()
	FVector GridCenter;

	// Grid Origin Offset
	UPROPERTY()
	FVector GridOriginOffset;

public:
	const FVector GetHashCellCenterpoint(FVector Location);
	const float GetHashCellWidth();
};

UENUM()
enum class EAtomSoundscapeLOD : uint8
{
	LOD1 = 0 UMETA(DisplayName = "LOD 1"),
	LOD2 UMETA(DisplayName = "LOD 2"),
	LOD3 UMETA(DisplayName = "LOD 3")
};

UCLASS()
class CRIWARESOUNDSCAPE_API UAtomSoundscapeColorPointHashMapCollection : public UObject
{
	GENERATED_BODY()

public:

	void InitializeCollection();

	void AddColorPointArrayToHashMapCollection(const TArray<FVector>& Locations, const FGameplayTag ColorPoint);

	void ClearColorPointHashMapCollection();

	int32 GetColorPointHashMapCollectionDensity(const FVector Location, const FGameplayTag ColorPoint, const EAtomSoundscapeLOD SoundscapeLOD);

public:
	void CalculateTotalColorPointDensity(TMap<FGameplayTag, int32>& TotalHashMapColorPointDensity, EAtomSoundscapeLOD SoundscapeLOD);

private:

	// Color Point Hash Maps
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD1;
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD2;
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD3;

	// Hash Cell Width for LOD1
	UPROPERTY()
	float LOD1ColorPointHashWidth = 500.0f;

	// Hash Cell LOD1 Max Distance
	UPROPERTY()
	float LOD1ColorPointHashDistance = 5000.0f;

	// Hash Cell Width for LOD2
	UPROPERTY()
	float LOD2ColorPointHashWidth = 2500.0f;

	// Hash Cell LOD2 Max Distance
	UPROPERTY()
	float LOD2ColorPointHashDistance = 10000.0f;

	// Hash Cell Width for LOD3
	UPROPERTY()
	float LOD3ColorPointHashWidth = 10000.0f;
};

/**
 * 
 */
UCLASS()
class CRIWARESOUNDSCAPE_API UAtomSoundscapeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// End USubsystem


	// Settings
public:

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void SetState(FGameplayTag SoundscapeState);

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void ClearState(FGameplayTag SoundscapeState);

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void RestartSoundscape();

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	bool AddPaletteCollection(FName PaletteCollectionName, FAtomSoundscapePaletteCollection PaletteCollection);

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	bool RemovePaletteCollection(FName PaletteCollectionName);

	bool bDebugMode = false;

private:
	UPROPERTY()
	TSet<TObjectPtr<UAtomSoundscapePalette>> LoadedPaletteCollectionSet;

	UPROPERTY()
	TMap<FName, FAtomSoundscapePaletteCollection> UnloadedPaletteCollections;

	FGameplayTagContainer SubsystemState;

	UPROPERTY()
	TMap<TObjectPtr<UAtomSoundscapePalette>, TObjectPtr<UAtomActiveSoundscapePalette>> ActivePalettes;

	bool LoadPaletteCollection(FName PaletteCollectionName);

	bool UnloadPaletteCollection(FName PaletteCollectionName);

	void UpdateState();

private:

	FAtomRuntimeId AtomRuntimeID;

	// Soundscape Color Point system
public:
	// Add a Color Point Collection to the Subsystem, returns true if successful
	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void AddColorPointCollection(FName ColorPointCollectionName, FAtomSoundscapeColorPointCollection ColorPointCollection);

	// Remove a Color Point Collection from the Subsystem, returns true if successful
	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	bool RemoveColorPointCollection(FName ColorPointCollectionName);

	// Check Color Point Density for a Location Cell
	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	int32 CheckColorPointDensity(FVector Location, FGameplayTag ColorPoint);

	// Add Hash Map Collection
	void AddColorPointHashMapCollection(UAtomSoundscapeColorPointHashMapCollection* ColorPointHashMapCollection);

	// Add Hash Map Collection
	bool RemoveColorPointHashMapCollection(UAtomSoundscapeColorPointHashMapCollection* ColorPointHashMapCollection);

	// Draw Debug Cell
	void DrawDebugCell(FVector Location, bool bSuccess);

	// Calculate Debug Cell Dimensions
	TPair<FVector, FVector> CalculateDebugCellDimensions(FVector Location, EAtomSoundscapeLOD SoundscapeLOD);

	// Add Active Color Point
	void AddActiveColorPoint(const UAtomSoundscapeColorPointComponent* SoundscapeColorPointComponent);

	// Remove Active Color Point
	void RemoveActiveColorPoint(const UAtomSoundscapeColorPointComponent* SoundscapeColorPointComponent);

private:
	// Stored Hash Map Collections
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAtomSoundscapeColorPointHashMapCollection>> ColorPointHashMapCollections;

	// Stored Color Point Collections
	TMap<FName, FAtomSoundscapeColorPointCollection> ColorPointCollections;

	// Active Color Point Collection
	FAtomSoundscapeColorPointCollection ActiveColorPointCollection;

	// Color Point Hash Maps
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD1;
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD2;
	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ColorPointHashMapLOD3;

	UPROPERTY()
	TObjectPtr<UAtomSoundscapeColorPointHashMap> ActiveColorPointHashMap;

	// Hash Cell Width for LOD1
	float LOD1ColorPointHashWidth = 500.0f;

	// Hash Cell LOD1 Max Distance
	float LOD1ColorPointHashDistance = 5000.0f;

	// Hash Cell Width for LOD2
	float LOD2ColorPointHashWidth = 2500.0f;

	// Hash Cell LOD2 Max Distance
	float LOD2ColorPointHashDistance = 10000.0f;

	// Hash Cell Width for LOD3
	float LOD3ColorPointHashWidth = 10000.0f;

	// Hash Cell Width for the Active Hash
	float ActiveColorPointHashWidth = 500.0f;

	// Hash Cell Update Timing for the Active Hash
	float ActiveColorPointHashUpdateTimeSeconds = 1.0f;

	// Update Hash Map
	void UpdateColorPointHashMap(UAtomSoundscapeColorPointHashMap& SoundscapeColorPointHashMap);

	// Update Active Hash Map
	void UpdateActiveColorPointHashMap();

	// Timer delegate for amortized updates
	FTimerHandle ActiveColorPointUpdateTimer;
	
	// Active Color Point Components
	TArray<const UAtomSoundscapeColorPointComponent*> ActiveSoundscapeColorPointComponents;
};
