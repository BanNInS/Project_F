
#include "Atom/Modulation/Generators/AtomModulationEnvelopeFollower.h"

#include "Algo/MaxElement.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/EnvelopeFollower.h"
#include "DSP/MultithreadedPatching.h"

#include "Sound/AudioBus.h"

#include "CriWareDefines.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/Modulation/AtomModulation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationEnvelopeFollower)

namespace AtomModulation
{
	namespace GeneratorEnvelopeFollowerPrivate
	{
		static const TArray<FString> DebugCategories =
		{
			TEXT("Value"),
			TEXT("Gain"),
			TEXT("Attack"),
			TEXT("Release"),
		};

		static const FString DebugName = TEXT("EnvelopeFollower");
	}

	class CRIWARECORE_API FEnvelopeFollowerGenerator
		: public IGenerator
	{
	public:

		FEnvelopeFollowerGenerator(const FEnvelopeFollowerGenerator& InGenerator)
			: AudioBusPatch(InGenerator.AudioBusPatch)
			, TempBuffer(InGenerator.TempBuffer)
			, EnvelopeFollower(InGenerator.EnvelopeFollower)
			, InitParams(InGenerator.InitParams)
			, BusId(InGenerator.BusId)
			, CurrentValue(InGenerator.CurrentValue)
			, Gain(InGenerator.Gain)
			, bBypass(InGenerator.bBypass)
			, bInvert(InGenerator.bInvert)
			, bBusRequiresPatch(InGenerator.bBusRequiresPatch)
		{
			RuntimeDestroyedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddRaw(this, &FEnvelopeFollowerGenerator::OnRuntimeDestroyed);
		}

		FEnvelopeFollowerGenerator(const FAtomEnvelopeFollowerGeneratorParams& InGeneratorParams)
			: Gain(InGeneratorParams.Gain)
			, bBypass(InGeneratorParams.bBypass ? 1 : 0)
			, bInvert(InGeneratorParams.bInvert ? 1 : 0)
			, bBusRequiresPatch(1)
		{
			RuntimeDestroyedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddRaw(this, &FEnvelopeFollowerGenerator::OnRuntimeDestroyed);

			if (UAtomAudioBus* AudioBus = InGeneratorParams.AudioBus)
			{
				BusId = AudioBus->GetUniqueID();
				InitParams.NumChannels = AudioBus->GetNumChannels();
			}

			InitParams.AttackTimeMsec = InGeneratorParams.AttackTime * 1000.0f;
			InitParams.ReleaseTimeMsec = InGeneratorParams.ReleaseTime * 1000.0f;
			InitParams.Mode = Audio::EPeakMode::Peak;

			if (bInvert)
			{
				CurrentValue = 1.0f;
			}
		}

		virtual ~FEnvelopeFollowerGenerator()
		{
			FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Remove(RuntimeDestroyedHandle);

			AudioBusPatch.Reset();
		}

#if !UE_BUILD_SHIPPING
		virtual void GetDebugCategories(TArray<FString>& OutDebugCategories) const override
		{
			OutDebugCategories = GeneratorEnvelopeFollowerPrivate::DebugCategories;
		}

		virtual const FString& GetDebugName() const override
		{
			return GeneratorEnvelopeFollowerPrivate::DebugName;
		}

		virtual void GetDebugValues(TArray<FString>& OutDebugValues) const override
		{
			const float AttackTime = EnvelopeFollower.GetAttackTimeMsec() / 1000.f;
			const float ReleaseTime = EnvelopeFollower.GetReleaseTimeMsec() / 1000.f;

			OutDebugValues.Add(FString::Printf(TEXT("%.4f"), GetValue()));
			OutDebugValues.Add(FString::Printf(TEXT("%.4f"), Gain));
			OutDebugValues.Add(FString::Printf(TEXT("%.4f"), AttackTime));
			OutDebugValues.Add(FString::Printf(TEXT("%.4f"), ReleaseTime));
		}

#endif // !UE_BUILD_SHIPPING

		virtual AtomModulation::FGeneratorPtr Clone() const override
		{
			return AtomModulation::FGeneratorPtr(new FEnvelopeFollowerGenerator(*this));
		}

		virtual void UpdateGenerator(AtomModulation::FGeneratorPtr&& InGenerator) override
		{
			if (!ensure(InGenerator.IsValid()))
			{
				return;
			}

			//AtomRenderThreadCommand([this, NewGenerator = MoveTemp(InGenerator)]()
			AtomThreadCommand([this, NewGenerator = MoveTemp(InGenerator)]()
			{
				const FEnvelopeFollowerGenerator* Generator = static_cast<const FEnvelopeFollowerGenerator*>(NewGenerator.Get());

				bBypass = Generator->bBypass;
				bInvert = Generator->bInvert;

				EnvelopeFollower.SetAnalog(Generator->InitParams.bIsAnalog);
				EnvelopeFollower.SetAttackTime(Generator->InitParams.AttackTimeMsec);
				EnvelopeFollower.SetMode(Generator->InitParams.Mode);
				EnvelopeFollower.SetNumChannels(Generator->InitParams.NumChannels);
				EnvelopeFollower.SetReleaseTime(Generator->InitParams.ReleaseTimeMsec);

				if (Generator->BusId != BusId || !FMath::IsNearlyEqual(Gain, Generator->Gain))
				{
					BusId = Generator->BusId;
					Gain = Generator->Gain;
					bBusRequiresPatch = true;
				}
			});
		}

		virtual float GetValue() const override
		{
			return CurrentValue;
		}

		virtual void Init(FAtomRuntimeId InRuntimeID) override
		{
			if (AtomRuntimeID == INDEX_NONE)
			{
				AtomRuntimeID = InRuntimeID;
				if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
				{
					FAtomRuntime* AtomRuntime = RuntimeManager->GetAtomRuntimeRaw(InRuntimeID);
					InitParams.SampleRate = 48000;
				}

				EnvelopeFollower = Audio::FEnvelopeFollower(InitParams);
			}
		}

		virtual bool IsBypassed() const override
		{
			return bBypass;
		}

		virtual void Update(double InElapsed) override
		{
			if (bBusRequiresPatch)
			{
				CreatePatchForBus();
			}

			if (AudioBusPatch.IsValid() && !AudioBusPatch->IsInputStale())
			{
				const int32 NumSamples = AudioBusPatch->GetNumSamplesAvailable();
				const int32 NumFrames = NumSamples / EnvelopeFollower.GetNumChannels();

				if (NumSamples > 0)
				{
					TempBuffer.Reset();
					TempBuffer.AddZeroed(NumSamples);

					AudioBusPatch->PopAudio(TempBuffer.GetData(), NumSamples, true /* bUseLatestAudio */);

					EnvelopeFollower.ProcessAudio(TempBuffer.GetData(), NumFrames);
				}

				float MaxValue = 0.f;
				if (const float* MaxEnvelopePtr = Algo::MaxElement(EnvelopeFollower.GetEnvelopeValues()))
				{
					MaxValue = FMath::Clamp(*MaxEnvelopePtr, 0.f, 1.f);
				}

				CurrentValue = bInvert ? 1.0f - MaxValue : MaxValue;
			}
			else
			{
				bBusRequiresPatch = true;
				CurrentValue = bInvert ? 1.f : 0.f;
			}

		}

		void CreatePatchForBus()
		{
			if (BusId != INDEX_NONE)
			{
				if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
				{
					FAtomRuntime* AtomRuntime = RuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);

					if (UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>())
					{
						AudioBusPatch = AudioBusSubsystem->AddPatchOutputForAudioBus(Atom::FAudioBusKey(BusId), 256, EnvelopeFollower.GetNumChannels(), Gain);
						bBusRequiresPatch = false;
					}
				}
			}
		}

		void OnRuntimeDestroyed(FAtomRuntimeId InRuntimeID)
		{
			if (InRuntimeID == AtomRuntimeID)
			{
				// Reset bus id on runtime destruction to avoid reinitializing
				// it during FAtomRuntime::Teardown via Update.
				BusId = INDEX_NONE;
				FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Remove(RuntimeDestroyedHandle);
			}
		}

	private:

		Audio::FPatchOutputStrongPtr AudioBusPatch;
		Audio::FAlignedFloatBuffer TempBuffer;
		Audio::FEnvelopeFollower EnvelopeFollower;
		Audio::FEnvelopeFollowerInitParams InitParams;

		uint32 BusId = INDEX_NONE;
		float CurrentValue = 0.0f;
		float Gain = 1.0f;
		uint8 bBypass : 1;
		uint8 bInvert : 1;
		uint8 bBusRequiresPatch : 1;

		FDelegateHandle RuntimeDestroyedHandle;
	};
} // namespace

AtomModulation::FGeneratorPtr UAtomModulationGeneratorEnvelopeFollower::CreateInstance() const
{
	using namespace AtomModulation;

	return FGeneratorPtr(new FEnvelopeFollowerGenerator(Params));
}
