
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Extensions/IAtomProxyInitializer.h"

#include "AtomAudioBus.generated.h"

// Forward Declarations
class FAtomRuntime;

// The number of channels to mix audio into the source bus
UENUM(BlueprintType)
enum class EAtomAudioBusChannels : uint8
{
	Mono = 0								UMETA(DisplayName = "Mono (1.0)"),						// 1ch
	Stereo = 1								UMETA(DisplayName = "Stereo (2.0)"),					// 2ch
	StereoWithCenter = 2					UMETA(DisplayName = "Stereo With Center (3.0)"),		// 3ch
	Quad = 3								UMETA(DisplayName = "Quad (4.0)"),						// 4ch
	FivePointOne = 5						UMETA(DisplayName = "Five Point One (5.1)"),			// 6ch
	SevenPointOne = 7						UMETA(DisplayName = "Seven Point One (7.1)"),			// 8ch
	//FivePointOnePointTwo = 7				UMETA(DisplayName = "Five Point One Point Two (5.1.2)"), // 8ch Dolby
	//FivePointOnePointFour = 9				UMETA(DisplayName = "Five Point One Point Four (5.1.4)"), // 10ch Dolby
	//SevenPointOnePointTwo = 9				UMETA(DisplayName = "Seven Point One Point Two (7.1.2)"), // 10ch Dolby
	//SevenPointOnePointFour = 11			UMETA(DisplayName = "Seven Point One Point Four (7.1.4)"), // 12ch Dolby
	//SevenPointOnePointFourPointFour = 15	UMETA(DisplayName = "Seven Point One Point Four Point Four (7.1.4.4)"), // 16ch full spatial
};

namespace AtomAudioBusUtils
{
	static EAtomAudioBusChannels ConvertIntToEAtomAudioBusChannels(const int32 InValue)
	{
		switch (InValue)
		{
		case 1: return EAtomAudioBusChannels::Mono;
		case 2:	return EAtomAudioBusChannels::Stereo;
		case 3: return EAtomAudioBusChannels::StereoWithCenter;
		case 4:	return EAtomAudioBusChannels::Quad;
		case 6:	return EAtomAudioBusChannels::FivePointOne;
		case 8:	return EAtomAudioBusChannels::SevenPointOne;
		//case 10: return EAtomAudioBusChannels::SevenPointOnePointTwo;
		//case 12: return EAtomAudioBusChannels::SevenPointOnePointFour;
		//case 16: return EAtomAudioBusChannels::SevenPointOnePointFourPointFour;
		default:
			//UE_LOG(LogAudio, Error, TEXT("Number of channels: %d not available in AudioBusChannels configurations. Make sure that the number of channels used is 1, 2, 4, 6 or 8."), InValue);
			check(false);
			return EAtomAudioBusChannels::Mono;
		}
	};
}

class CRIWARECORE_API FAtomAudioBusProxy final
	: public Atom::TProxyData<FAtomAudioBusProxy>
{
public:
	IMPL_ATOMPROXY_CLASS(FAtomAudioBusProxy);

	explicit FAtomAudioBusProxy(UAtomAudioBus* InAudioBus);

	FAtomAudioBusProxy(const FAtomAudioBusProxy& Other) = default;

	virtual ~FAtomAudioBusProxy() override {}

	uint32 AudioBusID = INDEX_NONE;
	int32 NumChannels = INDEX_NONE;
};

using FAtomAudioBusProxyPtr = TSharedPtr<FAtomAudioBusProxy, ESPMode::ThreadSafe>;

// An audio bus is an object which represents an audio patch cord. Audio can be sent to it. It can be sonified using UAtomSoundSourceBuses.
// Instances of the audio bus are created in the Atom runtime. 
UCLASS(ClassGroup = Sound, meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomAudioBus
	: public UObject
	, public IAtomProxyDataFactory
{
	GENERATED_BODY()

public:

	UAtomAudioBus(const FObjectInitializer& ObjectInitializer);

	/** Number of channels to use for the Audio Bus. */
	UPROPERTY(EditAnywhere, Category = BusProperties)
	EAtomAudioBusChannels AudioBusChannels = EAtomAudioBusChannels::Mono;

	// Returns the number of channels of the audio bus in integer format
	int32 GetNumChannels() const { return (int32)AudioBusChannels + 1; }

	//~ Begin UObject Interface.
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	//~ End UObject Interface.

	//~ Begin IAtomProxy Interface
	virtual TSharedPtr<Atom::IProxyData> CreateProxyData(const Atom::FProxyDataInitParams& InitParams) override;
	//~ End IAtomProxy Interface
};