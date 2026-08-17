
#pragma once

#include "IAudioLink.h"
#include "IAudioLinkBlueprintInterface.h"

#include "Atom/Components/AtomSynthComponent.h"
#include "Atom/AtomSoundGenerator.h"
#include "Atom/AtomSoundWaveProcedural.h"

#include "AtomAudioLinkInputClient.h"

#include "AtomAudioLinkComponent.generated.h"

// Forward Definitions
class UAudioComponent;
class UAtomAudioLinkSettings;

struct CRIWARECORE_API FAtomAudioLinkSoundGeneratorInitParams
{
	FAtomAudioLinkInputClientPtr ClientSP;

	static void Reset(FAtomAudioLinkSoundGeneratorInitParams& InParams);
};

class CRIWARECORE_API FAtomAudioLinkSoundGenerator : public IAtomSoundGenerator
{
public:

	explicit FAtomAudioLinkSoundGenerator(FAtomAudioLinkSoundGeneratorInitParams&& InParams);

	//~ Begin IAtomSoundGenerator Interface
	// Called when a new buffer is required.
	virtual int32 OnGenerateAudio(float** OutAudio, int32 NumFrames, int32 NumChannels) override;

	// Returns the number of samples to render per callback
	virtual int32 GetDesiredNumSamplesToRenderPerCallback() const override;

	virtual bool IsFinished() const override;
	//~ End IAtomSoundGenerator Interface

public:

	FAtomAudioLinkSoundGeneratorInitParams Params;
	
	FThreadSafeBool bGeneratingAudio = false;
};

/**
 * Sound with a generator used for pushed source or submix
 */
UCLASS(MinimalAPI)
class UAtomAudioLinkSound
	: public UAtomSoundWaveProcedural
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomAudioLinkSound(const FObjectInitializer& ObjectInitializer);

	CRIWARECORE_API void Init(FAtomAudioLinkInputClientPtr InClientSP);

	/** Begin UAtomSoundWaveProcedural */
	CRIWARECORE_API virtual IAtomSoundGeneratorPtr CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams) override;
	/** End UAtomSoundWaveProcedural */

	FAtomAudioLinkInputClientPtr ClientSP;

	bool bIsGenerating = false;
};

UCLASS(ClassGroup = (Audio, Common), HideCategories = (Object, ActorComponent, Physics, Rendering, Mobility, LOD), 
	ShowCategories = Trigger, meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomAudioLinkComponent
	: public UAtomSynthComponent
	, public IAudioLinkBlueprintInterface
{
    GENERATED_BODY()

public:

	UAtomAudioLinkComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UAtomSynthComponent interface
	virtual IAtomSoundGeneratorPtr CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams) override;
	virtual void OnEndGenerate(IAtomSoundGeneratorPtr Generator) override;
	//~ End UAtomSynthComponent interface

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioLink")
    TObjectPtr<UAtomAudioLinkSettings> Settings;

    /** The sound to be played */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
    TObjectPtr<class USoundBase> Sound;

	/** Whether or not to play the Link on start*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioLink")
	bool bAutoPlay = false;

protected:

    //~ Begin IAudioLinkBlueprintInterface
    virtual void SetLinkSound(USoundBase* NewSound) override;
    virtual void PlayLink(float StartTime = 0.0f) override;
    virtual void StopLink() override;
    virtual bool IsLinkPlaying() const override;
    //~ End IAudioLinkBlueprintInterface

    //~ Begin ActorComponent Interface.
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    //~ End ActorComponent Interface.

    void CreateAudioComponent();

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioComponent;

	void CreateLink();
	void ResetLink();
    TUniquePtr<IAudioLink> LinkInstance;

private:

	TSharedPtr<FAtomAudioLinkSoundGenerator, ESPMode::ThreadSafe> SoundGenerator;

	// called by AudioLinkInputClient when ready
	void OnLinkStart(FAtomAudioLinkInputClientPtr InClientSP);

	// called by AudioLinkInputClient when stopped
	void OnLinkStop(FAtomAudioLinkInputClientPtr InClientSP, int32 SourceID);

	friend class FAtomAudioLinkInputClient;
};
