
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "AtomSoundscapeColorPoint.generated.h"

USTRUCT(BlueprintType)
struct CRIWARESOUNDSCAPE_API FAtomSoundscapeColorPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint")
	FGameplayTag ColorPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint")
	FVector Location = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct CRIWARESOUNDSCAPE_API FAtomSoundscapeColorPointVectorArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint")
	FGameplayTag ColorPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint")
	TArray<FVector> Locations;
};

UCLASS( ClassGroup=(Soundscape), meta=(BlueprintSpawnableComponent) )
class CRIWARESOUNDSCAPE_API UAtomSoundscapeColorPointComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAtomSoundscapeColorPointComponent();

	// This Component's Soundscape Pigment value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soundscape|ColorPoint", meta = (Categories = "Soundscape"))
	FGameplayTag ColorPoint;

	// Outputs information about this Active Color Point (Color Point value and Location of Parent Actor)
	void GetInfo(FGameplayTag& ColorPoint, FVector& Location) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called when the game ends
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
