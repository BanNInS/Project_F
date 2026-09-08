
#include "AtomSoundscapeSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#include "AtomSoundscapeColorPoint.h"
#include "AtomSoundscapeSettings.h"
#include "AtomSoundscapePalette.h"

static FAutoConsoleCommandWithWorld GAtomResetSoundscape(
	TEXT("atom.soundscape.ResetSoundscape"),
	TEXT("Forces restart on all active Soundscape Palettes."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (const UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UAtomSoundscapeSubsystem* SoundscapeSubsystem = GameInstance->GetSubsystem<UAtomSoundscapeSubsystem>())
		{
			SoundscapeSubsystem->RestartSoundscape();
		}
	}
}));

void UAtomSoundscapeColorPointHashMap::InitializeHash(float HashCellSizeIn, FVector GridCenterIn)
{
	// Cache Hash Cell Size
	HashCellSize = FMath::Max(HashCellSizeIn, 1.0f);

	// MaxGridWidth / Hash Cell Size
	GridWidth = FMath::FloorToInt(MaxGridWidth / HashCellSize);

	// Grid Width Cubed
	NumCells = int64(GridWidth) * int64(GridWidth) * int64(GridWidth);

	// Cell Size Inverted
	HashCellFactor = 1 / HashCellSize;

	// Cache Grid Centerpoint
	GridCenter = GridCenterIn;

	// Calculate Grid Origin Offset
	GridOriginOffset = GridCenter - FVector(float(HalfMaxGridWidth), float(HalfMaxGridWidth), float(HalfMaxGridWidth));
}

void UAtomSoundscapeColorPointHashMap::ClearHash()
{
	// Cycle through Color Point Hash Map
	for (auto It = ColorPointHashMap.CreateIterator(); It; ++It)
	{
		// If Color Point has an existing hash map, empty it
		if (UAtomSoundscapeColorPointHashCellDensity* ColorPointHashCell = It->Value)
		{
			ColorPointHashCell->ColorPointHashCellDensity.Empty();
		}
	}

	// Empty Hash Map
	ColorPointHashMap.Empty();
}

int32 UAtomSoundscapeColorPointHashMap::NumColorPointsInCell(const FVector& Location, const FGameplayTag ColorPoint)
{
	// Determine if the Hash Map contains the Color Point Density Table Pointer
	if (TObjectPtr<UAtomSoundscapeColorPointHashCellDensity>* ColorPointHashCellPtr = ColorPointHashMap.Find(ColorPoint))
	{
		// Validate the double pointer
		if (UAtomSoundscapeColorPointHashCellDensity* ColorPointHashCell = *ColorPointHashCellPtr)
		{
			// Validate the Color Point Density Map
			if (ColorPointHashCell != nullptr)
			{
				// Look up Color Point Density at that Location
				int32* CellDensity = ColorPointHashCell->ColorPointHashCellDensity.Find(CalculateHashIndex(Location));

				// If Cell Density pointer is valid, return value
				if (CellDensity != nullptr)
				{
					return *CellDensity;
				}
			}
		}
	}

	// Otherwise return 0
	return 0;
}

bool UAtomSoundscapeColorPointHashMap::AddColorPointToHash(const FVector& Location, const FGameplayTag ColorPoint)
{
	// Add a single Color Point to the Color Point Hash Map
	// If the Color Point Coordinates exceed the Max Grid Width, it is out of bounds of the hash table
	// Incoming Location values are added to the Grid Origin Offset in the case that the Grid is not centered around Origin 0,0,0
	if (FMath::Abs(GridCenter.X + Location.X) > HalfMaxGridWidth
		|| FMath::Abs(GridCenter.Y + Location.Y) > HalfMaxGridWidth
		|| FMath::Abs(GridCenter.Z + Location.Z) > HalfMaxGridWidth)
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Warning, TEXT("Location out of Hash Bounds"));

		// Return false, unable to add Color Point to Hash
		return false;
	}

	// Get ColorPoint Density Map
	UAtomSoundscapeColorPointHashCellDensity* ColorPointDensityMap = GetColorPointDensityMap(ColorPoint);

	// Create a new Color Point Hash for this Color Point if one needs to be created
	if (ColorPointDensityMap == nullptr)
	{
		// Create new Color Point Density Map
		ColorPointDensityMap = NewObject<UAtomSoundscapeColorPointHashCellDensity>(this);

		// Add to Color Point Hash Map
		ColorPointHashMap.Add(ColorPoint, ColorPointDensityMap);
	}


	// If a Color Point Map exists for this Color Point, use it
	if (ColorPointDensityMap)
	{
		// Cache Hash Index
		uint64 HashIndex = CalculateHashIndex(Location);

		// Get potential Density value from Hash Index
		int32* ColorPointDensity = ColorPointDensityMap->ColorPointHashCellDensity.Find(HashIndex);

		// Validate Density pointer
		if (ColorPointDensity != nullptr)
		{
			// Color Point already tracked at this hash cell, increment
			++(*ColorPointDensity);
		}
		else
		{
			// Density Value invalid, new Hash Key/Value must be added at that index
			ColorPointDensityMap->ColorPointHashCellDensity.Add(HashIndex, 1);
		}
	}

	// Color Point successfully added
	return true;
}

void UAtomSoundscapeColorPointHashMap::AddColorPointArrayToHash(const TArray<FVector>& Locations, const FGameplayTag ColorPoint)
{

	// Local cache for Hash Indices
	TArray<uint64> HashIndices;

	// Get ColorPoint Density Map
	UAtomSoundscapeColorPointHashCellDensity* ColorPointDensityMap = GetColorPointDensityMap(ColorPoint);

	// Create a new Color Point Hash for this Color Point if one needs to be created
	if (ColorPointDensityMap == nullptr)
	{
		// Create new Color Point Density Map
		ColorPointDensityMap = NewObject<UAtomSoundscapeColorPointHashCellDensity>(this);

		// Add to Color Point Hash Map
		ColorPointHashMap.Add(ColorPoint, ColorPointDensityMap);
	}


	// If a Color Point Map exists for this Color Point, use it
	if(ColorPointDensityMap)
	{
		// Cycle through input Locations, add to temp array the indices calculated by the hashing function
		for (auto IndividualLocation = Locations.CreateConstIterator(); IndividualLocation; ++IndividualLocation)
		{
			// Make sure incoming Location Vector is within Hash Map Bounds
			if (FMath::Abs(GridCenter.X + IndividualLocation->X) <= HalfMaxGridWidth
				&& FMath::Abs(GridCenter.Y + IndividualLocation->Y) <= HalfMaxGridWidth
				&& FMath::Abs(GridCenter.Z + IndividualLocation->Z) <= HalfMaxGridWidth)
			{
				// Add Hash Index to temp Array
				HashIndices.Add(CalculateHashIndex(*IndividualLocation));
			}
			else
			{
				// If Location out of Hash Map Bounds, throw it away and spit out a warning
				UE_LOG(LogCriWareSoundscapeSubsystem, Warning, TEXT("Location out of Hash Bounds"));
			}
		}

		// Cycle through temp array of hash indices
		for (auto HashIndex = HashIndices.CreateConstIterator(); HashIndex; ++HashIndex)
		{
			// Cache index
			uint64 NewKey = *HashIndex;

			// Look for Color Point Density at that Index
			int32* ColorPointDensity = ColorPointDensityMap->ColorPointHashCellDensity.Find(NewKey);

			// If Color Point Density is valid, 
			if (ColorPointDensity != nullptr)
			{
				// Increment the value at the pointer
				++(*ColorPointDensity);
			}
			else
			{
				// Add a new Cell
				ColorPointDensityMap->ColorPointHashCellDensity.Add(NewKey, 1);
			}
		}

	}
}

uint64 UAtomSoundscapeColorPointHashMap::CalculateHashIndex(const FVector& Location)
{
	const FVector OffsetLocation = Location - GridOriginOffset;
	uint64 HashX = uint64(FMath::FloorToInt(OffsetLocation.X * HashCellFactor));
	uint64 HashY = uint64(FMath::FloorToInt(OffsetLocation.Y * HashCellFactor)) * uint64(GridWidth);
	uint64 HashZ = uint64(FMath::FloorToInt(OffsetLocation.Z * HashCellFactor)) * (uint64(GridWidth) * uint64(GridWidth));

	return HashX + HashY + HashZ;
}

void UAtomSoundscapeColorPointHashMap::SetGridCenterpoint(const FVector& Location)
{
	// Cache new Centerpoint
	GridCenter = Location;

	// Calculate Grid Origin Offset
	GridOriginOffset = Location - FVector(float(HalfMaxGridWidth), float(HalfMaxGridWidth), float(HalfMaxGridWidth));
}

TMap<FGameplayTag, int32> UAtomSoundscapeColorPointHashMap::GetHashMapColorPointDensitySummary()
{
	TMap<FGameplayTag, int32> DensitySummary;

	for (auto It = ColorPointHashMap.CreateConstIterator(); It; ++It)
	{
		FGameplayTag ColorPoint = It.Key();

		if (ColorPoint.IsValid())
		{
			int32 ColorDensity = 0;

			if (UAtomSoundscapeColorPointHashCellDensity* ColorPointHashLayer = It.Value())
			{
				for (auto Jt = ColorPointHashLayer->ColorPointHashCellDensity.CreateConstIterator(); Jt; ++Jt)
				{
					ColorDensity += Jt.Value();
				}
			}

			DensitySummary.Add(ColorPoint, ColorDensity);
		}
	}

	return DensitySummary;
}

UAtomSoundscapeColorPointHashCellDensity* UAtomSoundscapeColorPointHashMap::GetColorPointDensityMap(FGameplayTag ColorPoint)
{
	if (TObjectPtr<UAtomSoundscapeColorPointHashCellDensity>* ColorPointHashCellPtr = ColorPointHashMap.Find(ColorPoint))
	{
		// Get Color Point Hash Cell Pointer
		return *ColorPointHashCellPtr;
	}

	return nullptr;
}

const FVector UAtomSoundscapeColorPointHashMap::GetHashCellCenterpoint(FVector Location)
{
	const int32 LocationX = FMath::FloorToInt((Location.X - GridOriginOffset.X) / HashCellSize);
	const int32 LocationY = FMath::FloorToInt((Location.Y - GridOriginOffset.Y) / HashCellSize);
	const int32 LocationZ = FMath::FloorToInt((Location.Z - GridOriginOffset.Z) / HashCellSize);

	const float HalfHashCellSize = HashCellSize * 0.5;

	return FVector(((float)LocationX * HashCellSize) + HalfHashCellSize + GridOriginOffset.X
				, ((float)LocationY * HashCellSize) + HalfHashCellSize + GridOriginOffset.Y
				, ((float)LocationZ * HashCellSize) + HalfHashCellSize + GridOriginOffset.Z);
}

const float UAtomSoundscapeColorPointHashMap::GetHashCellWidth()
{
	return HashCellSize;
}

void UAtomSoundscapeColorPointHashMapCollection::InitializeCollection()
{
	// Get plugin settings on Subsystem initialization
	if (const UAtomSoundscapeSettings* ProjectSettings = GetDefault<UAtomSoundscapeSettings>())
	{
		// Hash Cell Width for LOD1
		LOD1ColorPointHashWidth = FMath::Max(ProjectSettings->LOD1ColorPointHashWidth, 1.0f);

		// Hash Cell LOD1 Max Distance
		LOD1ColorPointHashDistance = FMath::Max(ProjectSettings->LOD1ColorPointHashDistance, 1.0f);

		// Hash Cell Width for LOD2
		LOD2ColorPointHashWidth = FMath::Max(ProjectSettings->LOD2ColorPointHashWidth, 1.0f);

		// Hash Cell LOD2 Max Distance
		LOD2ColorPointHashDistance = FMath::Max(ProjectSettings->LOD2ColorPointHashDistance, 1.0f);

		// Hash Cell Width for LOD3
		LOD3ColorPointHashWidth = FMath::Max(ProjectSettings->LOD3ColorPointHashWidth, 1.0f);
	}

	// Initialize LOD Hashes
	ColorPointHashMapLOD1 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD1->InitializeHash(LOD1ColorPointHashWidth, FVector::ZeroVector);

	ColorPointHashMapLOD2 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD2->InitializeHash(LOD2ColorPointHashWidth, FVector::ZeroVector);

	ColorPointHashMapLOD3 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD3->InitializeHash(LOD3ColorPointHashWidth, FVector::ZeroVector);
}

void UAtomSoundscapeColorPointHashMapCollection::AddColorPointArrayToHashMapCollection(const TArray<FVector>& Locations, const FGameplayTag ColorPoint)
{
	if (ColorPointHashMapLOD1)
	{
		ColorPointHashMapLOD1->AddColorPointArrayToHash(Locations, ColorPoint);
	}

	if (ColorPointHashMapLOD2)
	{
		ColorPointHashMapLOD2->AddColorPointArrayToHash(Locations, ColorPoint);
	}

	if (ColorPointHashMapLOD3)
	{
		ColorPointHashMapLOD3->AddColorPointArrayToHash(Locations, ColorPoint);
	}
}

void UAtomSoundscapeColorPointHashMapCollection::ClearColorPointHashMapCollection()
{
	if (ColorPointHashMapLOD1)
	{
		ColorPointHashMapLOD1->ClearHash();
	}

	if (ColorPointHashMapLOD2)
	{
		ColorPointHashMapLOD2->ClearHash();
	}

	if (ColorPointHashMapLOD3)
	{
		ColorPointHashMapLOD3->ClearHash();
	}
}

int32 UAtomSoundscapeColorPointHashMapCollection::GetColorPointHashMapCollectionDensity(const FVector Location, const FGameplayTag ColorPoint, const EAtomSoundscapeLOD SoundscapeLOD)
{
	if (ColorPointHashMapLOD1 && ColorPointHashMapLOD2 && ColorPointHashMapLOD3)
	{
		switch (SoundscapeLOD)
		{
		case EAtomSoundscapeLOD::LOD1:
			return ColorPointHashMapLOD1->NumColorPointsInCell(Location, ColorPoint);

		case EAtomSoundscapeLOD::LOD2:
			return ColorPointHashMapLOD2->NumColorPointsInCell(Location, ColorPoint);

		case EAtomSoundscapeLOD::LOD3:
			return ColorPointHashMapLOD3->NumColorPointsInCell(Location, ColorPoint);
		}
	}

	return 0;
}

void UAtomSoundscapeColorPointHashMapCollection::CalculateTotalColorPointDensity(TMap<FGameplayTag, int32>& TotalHashMapColorPointDensity, EAtomSoundscapeLOD SoundscapeLOD)
{
	if (ColorPointHashMapLOD1 && ColorPointHashMapLOD2 && ColorPointHashMapLOD3)
	{
		switch (SoundscapeLOD)
		{
		case EAtomSoundscapeLOD::LOD1:
			TotalHashMapColorPointDensity = ColorPointHashMapLOD1->GetHashMapColorPointDensitySummary();

		case EAtomSoundscapeLOD::LOD2:
			TotalHashMapColorPointDensity = ColorPointHashMapLOD2->GetHashMapColorPointDensitySummary();

		case EAtomSoundscapeLOD::LOD3:
			TotalHashMapColorPointDensity = ColorPointHashMapLOD3->GetHashMapColorPointDensitySummary();
		}
	}
}

void UAtomSoundscapeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Subsystem Initializing"));

	// Get Subsystem World
	UWorld* World = GetWorld();

	// Initialize AudioDeviceID
	AtomRuntimeID = INDEX_NONE;

	// Get world
	if (World)
	{
		// Get Audio Device Handle
		FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);

		if (AtomRuntime)
		{
			AtomRuntimeID = AtomRuntime->GetAtomRuntimeID();
		}

	}

	// Get plugin settings on Subsystem initialization
	if (const UAtomSoundscapeSettings* ProjectSettings = GetDefault<UAtomSoundscapeSettings>())
	{
		if (ProjectSettings->SoundscapePaletteCollection.IsEmpty() == false)
		{
			FAtomSoundscapePaletteCollection PaletteCollectionToAdd;

			PaletteCollectionToAdd.SoundscapePaletteCollection = ProjectSettings->SoundscapePaletteCollection;

			UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Project settings Soundscape Palette Collection is valid"));

			AddPaletteCollection(FName("GlobalPaletteCollection"), PaletteCollectionToAdd);			
		}
		else
		{
			UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Project settings Soundscape Palette Collection is empty"));
		}

		bDebugMode = ProjectSettings->bDebugDraw;

		// Hash Cell Width for LOD1
		LOD1ColorPointHashWidth = FMath::Max(ProjectSettings->LOD1ColorPointHashWidth, 1.0f);

		// Hash Cell LOD1 Max Distance
		LOD1ColorPointHashDistance = FMath::Max(ProjectSettings->LOD1ColorPointHashDistance, 1.0f);

		// Hash Cell Width for LOD2
		LOD2ColorPointHashWidth = FMath::Max(ProjectSettings->LOD2ColorPointHashWidth, 1.0f);

		// Hash Cell LOD2 Max Distance
		LOD2ColorPointHashDistance = FMath::Max(ProjectSettings->LOD2ColorPointHashDistance, 1.0f);

		// Hash Cell Width for LOD3
		LOD3ColorPointHashWidth = FMath::Max(ProjectSettings->LOD3ColorPointHashWidth, 1.0f);

		// Hash Cell Width for the Active Hash
		ActiveColorPointHashWidth = FMath::Max(ProjectSettings->ActiveColorPointHashWidth, 1.0f);

		// Hash Cell Update Timing for the Active Hash
		ActiveColorPointHashUpdateTimeSeconds = FMath::Max(ProjectSettings->ActiveColorPointHashUpdateTimeSeconds, 0.2f);
	}
	else
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Invalid Project Settings"));
	}

	// Initialize LOD Hashes
	ColorPointHashMapLOD1 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD1->InitializeHash(LOD1ColorPointHashWidth, FVector::ZeroVector);

	ColorPointHashMapLOD2 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD2->InitializeHash(LOD2ColorPointHashWidth, FVector::ZeroVector);

	ColorPointHashMapLOD3 = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ColorPointHashMapLOD3->InitializeHash(LOD3ColorPointHashWidth, FVector::ZeroVector);

	// Initialize Active Hash Map
	ActiveColorPointHashMap = NewObject<UAtomSoundscapeColorPointHashMap>(this);
	ActiveColorPointHashMap->InitializeHash(ActiveColorPointHashWidth, FVector::ZeroVector);

	UpdateActiveColorPointHashMap();
}

void UAtomSoundscapeSubsystem::Deinitialize()
{
	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Subsystem Deinitializing"));

	ActiveColorPointUpdateTimer.Invalidate();
}

bool UAtomSoundscapeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Check with parent first
	if (Super::ShouldCreateSubsystem(Outer) == false)
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Subsystem will not be created based on parent rules"));

		return false;
	}

	// Get World
	if (UWorld* OuterWorld = Outer->GetWorld())
	{
		if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(OuterWorld))
		{
			UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Atom runtime valid, will create subsystem"));

			// If we do have an audio device, we can create this subsystem.
			return true;
		}
	}

	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Subsystem will not be created"));

	// If we do not have an audio device, we do not need to create this subsystem.
	return false;
}

void UAtomSoundscapeSubsystem::SetState(FGameplayTag SoundscapeState)
{
	if (SoundscapeState.IsValid())
	{
		// Add new state to the container
		SubsystemState.AddLeafTag(SoundscapeState);

		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Set State called"));

		UpdateState();
	}
	else
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Warning, TEXT("Set State called with invalid State Tag"));
	}
}

void UAtomSoundscapeSubsystem::ClearState(FGameplayTag SoundscapeState)
{
	if (SoundscapeState.IsValid())
	{
		// Add new state to the container
		SubsystemState.RemoveTag(SoundscapeState);

		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Clear State called"));

		UpdateState();
	}
	else
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Clear State called with invalid State Tag"));
	}
}

void UAtomSoundscapeSubsystem::RestartSoundscape()
{
	// Restart matching valid Soundscape Palettes
	for (const TPair<UAtomSoundscapePalette*, UAtomActiveSoundscapePalette*> ActiveSoundscapePalettePair : ActivePalettes)
	{
		UAtomSoundscapePalette* SoundscapePalette = ActiveSoundscapePalettePair.Key;
		UAtomActiveSoundscapePalette* ActiveSoundscapePalette = ActiveSoundscapePalettePair.Value;

		if (SoundscapePalette && ActiveSoundscapePalette)
		{
			ActiveSoundscapePalette->Stop();
			ActiveSoundscapePalette->Play();
		}
	}
}

bool UAtomSoundscapeSubsystem::AddPaletteCollection(FName PaletteCollectionName, FAtomSoundscapePaletteCollection PaletteCollection)
{
	if (UnloadedPaletteCollections.Contains(PaletteCollectionName) == false)
	{
		UnloadedPaletteCollections.Add(PaletteCollectionName, PaletteCollection);

		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Palette Collection %s added"), *PaletteCollectionName.ToString());

		bool bLoaded = LoadPaletteCollection(PaletteCollectionName);

		UpdateState();

		return bLoaded;
	}

	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Add Palette Collection called with empty Palette Collection"));

	return false;
}

bool UAtomSoundscapeSubsystem::RemovePaletteCollection(FName PaletteCollectionName)
{
	if (UnloadedPaletteCollections.Find(PaletteCollectionName))
	{
		if (UnloadPaletteCollection(PaletteCollectionName))
		{
			// Unloaded, now remove
			UnloadedPaletteCollections.Remove(PaletteCollectionName);

			UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Palette Collection %s removed"), *PaletteCollectionName.ToString());

			UpdateState();

			return true;
		}
	}

	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Remove Palette Collection called, but the name could not be found"));

	return false;
}

bool UAtomSoundscapeSubsystem::LoadPaletteCollection(FName PaletteCollectionName)
{
	if (UnloadedPaletteCollections.Contains(PaletteCollectionName))
	{
		FAtomSoundscapePaletteCollection PaletteCollectionToLoad = UnloadedPaletteCollections.FindRef(PaletteCollectionName);

		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Attempting to load Palette Collection %s"), *PaletteCollectionName.ToString());

		for (FSoftObjectPath& ObjPath : PaletteCollectionToLoad.SoundscapePaletteCollection)
		{
			if (UObject* PalettePath = ObjPath.TryLoad())
			{
				UAtomSoundscapePalette* SoundscapePalette = Cast<UAtomSoundscapePalette>(PalettePath);

				if (SoundscapePalette)
				{
					// If palette is valid, add it to the Subsystem Collection
					LoadedPaletteCollectionSet.Add(SoundscapePalette);

					UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Valid Soundscape Palette %s in Palette Collection %s"), *SoundscapePalette->GetFullName(), *PaletteCollectionName.ToString());
				}
				else
				{
					UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Invalid Soundscape Palette in Palette Collection %s"), *PaletteCollectionName.ToString());
				}
			}
		}

		return true;
	}

	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Attempted to load Palette Collection %s but failed"), *PaletteCollectionName.ToString());

	return false;
}

bool UAtomSoundscapeSubsystem::UnloadPaletteCollection(FName PaletteCollectionName)
{
	if (UnloadedPaletteCollections.Find(PaletteCollectionName))
	{
		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Attempting to unload Palette Collection %s"), *PaletteCollectionName.ToString());

		// Get unloaded set to remove from list
		FAtomSoundscapePaletteCollection PaletteCollectionToUnload = UnloadedPaletteCollections.FindRef(PaletteCollectionName);
		TSet<FSoftObjectPath> PaletteCollectionSetToUnload;
		PaletteCollectionSetToUnload.Append(PaletteCollectionToUnload.SoundscapePaletteCollection);
		TSet<FSoftObjectPath> PaletteCollectionSetToKeep;

		// Go through all unloaded collections and construct a unique list of Soft Object Paths
		for (TPair<FName, FAtomSoundscapePaletteCollection>& UnloadedCollectionName : UnloadedPaletteCollections)
		{
			if (UnloadedCollectionName.Key != PaletteCollectionName)
			{
				PaletteCollectionSetToKeep.Append(UnloadedCollectionName.Value.SoundscapePaletteCollection.Intersect(PaletteCollectionSetToUnload));
			}
		}

		// Find the difference between what we want to retain and what we want to unload
		PaletteCollectionSetToUnload = PaletteCollectionSetToUnload.Difference(PaletteCollectionSetToKeep);

		// Temp set of palettes to remove and stop
		decltype(LoadedPaletteCollectionSet) PalettesToUnloadStopAndRemove;

		// Go through collection, get their loaded pointers, add to temp removal set
		for (FSoftObjectPath& ObjPath : PaletteCollectionSetToUnload)
		{
			if (UObject* PalettePath = ObjPath.TryLoad())
			{
				UAtomSoundscapePalette* SoundscapePalette = Cast<UAtomSoundscapePalette>(PalettePath);

				if (SoundscapePalette)
				{
					// If palette is valid, add it to the Subsystem Collection
					PalettesToUnloadStopAndRemove.Add(SoundscapePalette);
				}
			}
		}

		// Stop and remove from Active list
		for (UAtomSoundscapePalette* PaletteKeyToStop : PalettesToUnloadStopAndRemove)
		{
			if (PaletteKeyToStop)
			{
				if (UAtomActiveSoundscapePalette* ActiveSoundscapePalette = ActivePalettes.FindRef(PaletteKeyToStop))
				{
					ActiveSoundscapePalette->Stop();

					ActivePalettes.Remove(PaletteKeyToStop);

					UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Soundscape Palette %s stopped"), *ActiveSoundscapePalette->GetFullName());
				}
			}
		}

		// Remove from main list of loaded palettes
		LoadedPaletteCollectionSet = LoadedPaletteCollectionSet.Difference(PalettesToUnloadStopAndRemove);

		UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Palette Collection %s unloaded"), *PaletteCollectionName.ToString());

		return true;
	}

	UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Attempted to unload Palette Collection %s but could not find matching collection"), *PaletteCollectionName.ToString());

	return false;
}

void UAtomSoundscapeSubsystem::AddColorPointCollection(FName ColorPointCollectionName, FAtomSoundscapeColorPointCollection ColorPointCollection)
{
	ColorPointCollections.Add(ColorPointCollectionName, ColorPointCollection);

	UpdateColorPointHashMap(*ColorPointHashMapLOD1);
	UpdateColorPointHashMap(*ColorPointHashMapLOD2);
	UpdateColorPointHashMap(*ColorPointHashMapLOD3);
}

bool UAtomSoundscapeSubsystem::RemoveColorPointCollection(FName ColorPointCollectionName)
{
	return (ColorPointCollections.Remove(ColorPointCollectionName) > 0);
}

int32 UAtomSoundscapeSubsystem::CheckColorPointDensity(FVector Location, FGameplayTag ColorPoint)
{
	FVector ListenerLocation;
	int32 ColorPointDensity = 0;
	EAtomSoundscapeLOD CurrentLOD = EAtomSoundscapeLOD::LOD1;

	if (UWorld* World = GetWorld())
	{
		if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World))
		{
			// Get available Listener Proxies
			TArray<FAtomListenerProxy>& ListenerProxies = AtomRuntime->ListenerProxies;

			// TODO: Handle Split Screen
			if (ListenerProxies.Num())
			{
				// Get Location and Direction from Listener
				FTransform& ListenerTransform = ListenerProxies[0].Transform;
				ListenerLocation = ListenerTransform.GetLocation();
			}
		}
	}

	// Get relative distance from the Listener Location
	FVector RelativeLocation = Location - ListenerLocation;

	// If relative distance is less than LOD 1 cutoff distance, use LOD 1 Map
	if (RelativeLocation.Length() < LOD1ColorPointHashDistance)
	{
		CurrentLOD = EAtomSoundscapeLOD::LOD1;
		ColorPointDensity = ColorPointDensity + ColorPointHashMapLOD1->NumColorPointsInCell(Location, ColorPoint);
	}
	else if (RelativeLocation.Length() < LOD2ColorPointHashDistance)
	{
		CurrentLOD = EAtomSoundscapeLOD::LOD2;
		// If relative distance is less than LOD 2 cutoff distance, use LOD 2 Map
		ColorPointDensity = ColorPointDensity + ColorPointHashMapLOD2->NumColorPointsInCell(Location, ColorPoint);
	}
	else
	{
		CurrentLOD = EAtomSoundscapeLOD::LOD3;
		// If distance exceeds both cutoff distances, use LOD 3 Map
		ColorPointDensity = ColorPointDensity + ColorPointHashMapLOD3->NumColorPointsInCell(Location, ColorPoint);
	}

	// Check Active Color Point Map
	ColorPointDensity = ColorPointDensity + ActiveColorPointHashMap->NumColorPointsInCell(Location, ColorPoint);

	// Check Color Point Hash Map Collections
	if (ColorPointHashMapCollections.Num() > 0)
	{
		for (auto It = ColorPointHashMapCollections.CreateConstIterator(); It; ++It)
		{
			if (It)
			{
				UAtomSoundscapeColorPointHashMapCollection* HashMapCollection = *It;

				if (HashMapCollection)
				{
					ColorPointDensity = ColorPointDensity + HashMapCollection->GetColorPointHashMapCollectionDensity(Location, ColorPoint, CurrentLOD);
				}
			}
		}
	}

	// Return total
	return ColorPointDensity;
}

void UAtomSoundscapeSubsystem::AddColorPointHashMapCollection(UAtomSoundscapeColorPointHashMapCollection* ColorPointHashMapCollection)
{
	if (ColorPointHashMapCollection)
	{
		ColorPointHashMapCollections.Add(ColorPointHashMapCollection);
	}
}

bool UAtomSoundscapeSubsystem::RemoveColorPointHashMapCollection(UAtomSoundscapeColorPointHashMapCollection* ColorPointHashMapCollection)
{
	if (ColorPointHashMapCollection)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		ColorPointHashMapCollections.RemoveSingleSwap(ColorPointHashMapCollection, EAllowShrinking::Yes);
#else
		ColorPointHashMapCollections.RemoveSingleSwap(ColorPointHashMapCollection, true);
#endif
		return true;
	}

	return false;
}

void UAtomSoundscapeSubsystem::DrawDebugCell(FVector Location, bool bSuccess)
{
	if (bDebugMode)
	{
		FVector ListenerLocation;
		TPair<FVector, FVector> BoxDimensions;

		if (UWorld* World = GetWorld())
		{
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World))
			{
				// Get available Listener Proxies
				TArray<FAtomListenerProxy>& ListenerProxies = AtomRuntime->ListenerProxies;

				// TODO: Handle Split Screen
				if (ListenerProxies.Num())
				{
					// Get Location and Direction from Listener
					FTransform& ListenerTransform = ListenerProxies[0].Transform;
					ListenerLocation = ListenerTransform.GetLocation();
				}
			}
		}

		// Get relative distance from the Listener Location
		FVector RelativeLocation = Location - ListenerLocation;

		// If relative distance is less than LOD 1 cutoff distance, use LOD 1 Map
		if (RelativeLocation.Length() < LOD1ColorPointHashDistance)
		{
			BoxDimensions = CalculateDebugCellDimensions(Location, EAtomSoundscapeLOD::LOD1);
		}
		else if (RelativeLocation.Length() < LOD2ColorPointHashDistance)
		{
			BoxDimensions = CalculateDebugCellDimensions(Location, EAtomSoundscapeLOD::LOD2);
		}
		else
		{
			BoxDimensions = CalculateDebugCellDimensions(Location, EAtomSoundscapeLOD::LOD3);
		}

		FColor BoxColor = bSuccess ? FColor::Green : FColor::Red;
		DrawDebugBox(GetWorld(), BoxDimensions.Key, BoxDimensions.Value, BoxColor, false, 5.0f, '\000', 5.0f);
	}
}

TPair<FVector, FVector> UAtomSoundscapeSubsystem::CalculateDebugCellDimensions(FVector Location, EAtomSoundscapeLOD SoundscapeLOD)
{
	UAtomSoundscapeColorPointHashMap* LODHashMap = nullptr;
	TPair<FVector, FVector> BoxResults;

	// If relative distance is less than LOD 1 cutoff distance, use LOD 1 Map
	switch (SoundscapeLOD)
	{
	case EAtomSoundscapeLOD::LOD1:
		LODHashMap = ColorPointHashMapLOD1;
		break;
	case EAtomSoundscapeLOD::LOD2:
		LODHashMap = ColorPointHashMapLOD2;
		break;
	case EAtomSoundscapeLOD::LOD3:
		LODHashMap = ColorPointHashMapLOD3;
	}

	if (LODHashMap)
	{
		FVector BoxCenter = LODHashMap->GetHashCellCenterpoint(Location);
		float BoxWidth = LODHashMap->GetHashCellWidth();

		BoxResults.Key = BoxCenter;
		BoxResults.Value = FVector(BoxWidth, BoxWidth, BoxWidth);
	}

	return BoxResults;
}

void UAtomSoundscapeSubsystem::AddActiveColorPoint(const UAtomSoundscapeColorPointComponent* SoundscapeColorPointComponent)
{
	if (SoundscapeColorPointComponent)
	{
		ActiveSoundscapeColorPointComponents.AddUnique(SoundscapeColorPointComponent);
	}
}

void UAtomSoundscapeSubsystem::RemoveActiveColorPoint(const UAtomSoundscapeColorPointComponent* SoundscapeColorPointComponent)
{
	if (SoundscapeColorPointComponent)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		ActiveSoundscapeColorPointComponents.RemoveSingleSwap(SoundscapeColorPointComponent, EAllowShrinking::Yes);
#else
		ActiveSoundscapeColorPointComponents.RemoveSingleSwap(SoundscapeColorPointComponent, true);
#endif
	}
}

void UAtomSoundscapeSubsystem::UpdateState()
{
	// Palettes the keep
	decltype(ActivePalettes) ActivePalettesToKeep;

	// Remove tags that don't match
	for (const TPair<UAtomSoundscapePalette*, UAtomActiveSoundscapePalette*> ActiveSoundscapePalettePair : ActivePalettes)
	{
		UAtomSoundscapePalette* SoundscapePalette = ActiveSoundscapePalettePair.Key;
		UAtomActiveSoundscapePalette* ActiveSoundscapePalette = ActiveSoundscapePalettePair.Value;

		if (SoundscapePalette && ActiveSoundscapePalette)
		{
			if (SoundscapePalette->SoundscapePalettePlaybackConditions.IsEmpty() || SoundscapePalette->SoundscapePalettePlaybackConditions.Matches(SubsystemState))
			{
				ActivePalettesToKeep.Add(SoundscapePalette, ActiveSoundscapePalette);
			}
			else
			{
				ActiveSoundscapePalette->Stop();

				UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Soundscape Palette %s stopped"), *ActiveSoundscapePalette->GetFullName());
			}
		}
	}

	// Empty active palettes an append ones to keep
	ActivePalettes.Empty();
	ActivePalettes.Append(ActivePalettesToKeep);

	// Add matching palettes
	for (auto PaletteIterator = LoadedPaletteCollectionSet.CreateConstIterator(); PaletteIterator; ++PaletteIterator)
	{
		if (UAtomSoundscapePalette* SoundscapePalette = *PaletteIterator)
		{
			// Evaluate if the Soundscape Palette matches
			if ((SoundscapePalette->SoundscapePalettePlaybackConditions.IsEmpty() || SoundscapePalette->SoundscapePalettePlaybackConditions.Matches(SubsystemState)) && ActivePalettes.Contains(SoundscapePalette) == false)
			{
				UWorld* World = GetWorld();

				if (World)
				{
					// Create a new ActiveSoundscapeColor
					UAtomActiveSoundscapePalette* ActiveSoundscapePalette = NewObject<UAtomActiveSoundscapePalette>(World);

					//
					ActiveSoundscapePalette->InitializeSettings(World, SoundscapePalette);

					ActiveSoundscapePalette->Play();

					UE_LOG(LogCriWareSoundscapeSubsystem, Verbose, TEXT("Soundscape Palette %s started"), *ActiveSoundscapePalette->GetFullName());

					ActivePalettes.Add(SoundscapePalette, ActiveSoundscapePalette);

				}
			}
		}
	}
}

void UAtomSoundscapeSubsystem::UpdateColorPointHashMap(UAtomSoundscapeColorPointHashMap& SoundscapeColorPointHashMap)
{
	// Clear out hash Map
	SoundscapeColorPointHashMap.ClearHash();

	// Temp Collection Array
	TArray<FAtomSoundscapeColorPointCollection> ColorPointCollectionArray;

	// Fill Temp Array
	ColorPointCollections.GenerateValueArray(ColorPointCollectionArray);
	
	// Cycle through Collections
	for (auto Collection = ColorPointCollectionArray.CreateConstIterator(); Collection; ++Collection)
	{
		// Cycle through Color Point Vector Arrays
		for (auto ColorPointVectorArray = Collection->ColorPointCollection.CreateConstIterator(); ColorPointVectorArray; ++ColorPointVectorArray)
		{
			// Add Vector Array to Color Point Map
			SoundscapeColorPointHashMap.AddColorPointArrayToHash(ColorPointVectorArray->Locations, ColorPointVectorArray->ColorPoint);
		}
	}
}

void UAtomSoundscapeSubsystem::UpdateActiveColorPointHashMap()
{
	ActiveColorPointHashMap->ClearHash();

	for (auto It = ActiveSoundscapeColorPointComponents.CreateConstIterator(); It; ++It)
	{
		if (It)
		{
			if (const UAtomSoundscapeColorPointComponent* ActiveColorPointComponent = *It)
			{
				FVector ColorPointLocation;
				FGameplayTag ColorPointValue;

				ActiveColorPointComponent->GetInfo(ColorPointValue, ColorPointLocation);

				ActiveColorPointHashMap->AddColorPointToHash(ColorPointLocation, ColorPointValue);
			}
		}
	}

	// Set up for first Timer delay
	float DelayTime = FMath::Max(0.2f, ActiveColorPointHashUpdateTimeSeconds);

	if (UWorld* World = GetWorld())
	{

		World->GetTimerManager().SetTimer(ActiveColorPointUpdateTimer, this, &UAtomSoundscapeSubsystem::UpdateActiveColorPointHashMap, DelayTime);
	}
}

DEFINE_LOG_CATEGORY(LogCriWareSoundscapeSubsystem);
