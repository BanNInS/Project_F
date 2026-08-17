
#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "HAL/Platform.h"
#include "Templates/UniquePtr.h"
#include "UObject/NameTypes.h"

#include "Extensions/IAtomAnalyzerInterface.h"

namespace Atom
{
	IAnalyzerFactory* GetAnalyzerFactory(FName InFactoryName);

	/** FAnalyzer
	 *
	 * FAnalyzer provides a simplified interface for performing analysis on audio buffers from the same AudioAnalyzer.
	 */
	class FAnalyzerFacade
	{
		public:
			/**
			 * Create an FAnalyzerBatch with the analyzer settings and factory name.
			 */
			CRIWARECORE_API FAnalyzerFacade(TUniquePtr<IAnalyzerSettings> InSettings, IAnalyzerFactory* InFactory);

			/**
			 * Analyze the audio buffer.
			 */
			CRIWARECORE_API TUniquePtr<IAnalyzerResult> AnalyzeAudioBuffer(const TArray<float>& InAudioBuffer, int32 InNumChannels, float InSampleRate, TSharedPtr<IAnalyzerControls> InControls);

		private:

			TUniquePtr<IAnalyzerSettings> Settings;
			TUniquePtr<IAnalyzerWorker> Worker;

			IAnalyzerFactory* Factory;
			FName FactoryName;
	};
} // namespace
