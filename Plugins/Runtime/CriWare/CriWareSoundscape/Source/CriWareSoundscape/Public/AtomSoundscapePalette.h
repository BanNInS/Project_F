
#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "GameplayTagContainer.h"

#include "CriWareSoundscapeModule.h"

#include "AtomSoundscapePalette.generated.h"

class UAtomSoundscapeColor;
class UAtomActiveSoundscapeColor;

// Struct storing Modulation State
USTRUCT(BlueprintType)
struct CRIWARESOUNDSCAPE_API FAtomSoundscapePaletteColor
{
	GENERATED_BODY()

	// Soundscape Color to Play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette")
	TObjectPtr<UAtomSoundscapeColor> SoundscapeColor = nullptr;

	// Base Volume Scalar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette", meta = (ClampMin = "0.0", ClampMax = "4.0", UIMin = "0.0", UIMax = "4.0", SliderExponent = "6.0"))
	float ColorVolume = 1.0f;

	// Base Pitch Scalar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette", meta = (ClampMin = "0.2", ClampMax = "4.0", UIMin = "0.2", UIMax = "4.0", SliderExponent = "3.0"))
	float ColorPitch = 1.0f;

	// Base Volume Scalar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ColorFadeIn = 1.0f;

	// Base Volume Scalar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ColorFadeOut = 1.0f;
};

// Class containing relevant data for a Soundscape Element
UCLASS(BlueprintType, ClassGroup = Soundscape)
class CRIWARESOUNDSCAPE_API UAtomSoundscapePalette : public UObject
{
	GENERATED_BODY()

public:
	UAtomSoundscapePalette();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette")
	FGameplayTagQuery SoundscapePalettePlaybackConditions;

	// Elements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|Palette")
	TArray<FAtomSoundscapePaletteColor> Colors;

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface
};


UCLASS(BlueprintType, ClassGroup = Soundscape)
class CRIWARESOUNDSCAPE_API UAtomActiveSoundscapePalette : public UObject
{
	GENERATED_BODY()

public:
	void InitializeSettings(UObject* WorldContextObject, UAtomSoundscapePalette* SoundscapePalette);

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void Play();

	UFUNCTION(BlueprintCallable, Category = "Soundscape")
	void Stop();

private:
	UPROPERTY()
	TObjectPtr<UWorld> World;

	UPROPERTY()
	TArray<TObjectPtr<UAtomActiveSoundscapeColor>> ActiveSoundscapeColors;
};
