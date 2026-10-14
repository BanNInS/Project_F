
//
// Notes: 
// 
// Like Unreal Audio Mixer conterpart, Atom Mixer handles the input and output sources and voices for an Atom runtime,
// but real mixage is performed by Atom engine. 
// 
// DSP patches from Unreal Audio, that are not dependent to a specific audio rngine,
// are used to make some buffered sources compatible between Atom and Unreal Audio. 
//

#pragma once

#include "CoreMinimal.h"

CRIWARECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomMixer, Log, All);
CRIWARECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomMixerDebug, Warning, All);

DECLARE_STATS_GROUP(TEXT("Atom Mixer"), STATGROUP_AtomMixer, STATCAT_Advanced);

#ifndef ATOM_MIXER_ENABLE_DEBUG_MODE
// This define enables a bunch of more expensive debug checks and logging capabilities that are intended to be off most of the time even in debug builds of game/editor.
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
#define ATOM_MIXER_ENABLE_DEBUG_MODE 0
#else
#define ATOM_MIXER_ENABLE_DEBUG_MODE 1
#endif
#endif

#define ATOM_MIXER_ENABLE_PCM_TRACE 0

#define ATOM_MIXER_MAX_OUTPUT_CHANNELS 16 // Max number of speakers/channels supported (7.1.4.4)

 // Whether to use patch inputs to send audio to each audio bus or re-use single fixed buffers send to each audio bus source sends.
#define ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS 1

// Enable debug checking for audio mixer
#if ATOM_MIXER_ENABLE_DEBUG_MODE
#define ATOM_MIXER_CHECK(expr) ensure(expr)
#define ATOM_MIXER_CHECK_GAME_THREAD(_MixerRuntime)	(_MixerRuntime->CheckAtomThread())
#define ATOM_MIXER_CHECK_AUDIO_THREAD(_MixerRuntime)	(_MixerRuntime->CheckAudioRenderingThread())
#else
#define ATOM_MIXER_CHECK(expr)
#define ATOM_MIXER_CHECK_GAME_THREAD(_MixerRuntime)
#define ATOM_MIXER_CHECK_AUDIO_THREAD(_MixerRuntime)
#endif

namespace Atom
{
	enum class EMixerSpeaker : int32
	{
		FrontLeft,
		FrontRight,
		FrontCenter,
		LowFrequency,
		SurroundLeft,
		SurroundRight,
		SurroundBackLeft,
		SurroundBackRight,
		TopFrontLeft,
		TopFrontRight,
		TopRearLeft,
		TopRearRight,
		BottomFrontLeft,
		BottomFrontRight,
		BottomRearLeft,
		BottomRearRight,
		Count,
		Unknown,
		DefaultChannel = FrontLeft
	};

	static const int32 MaxSupportedSpeakers = (int32)EMixerSpeaker::Count;

	inline const TCHAR* ToString(EMixerSpeaker InSpeaker)
	{
		switch (InSpeaker)
		{
		case EMixerSpeaker::FrontLeft:			return TEXT("FrontLeft");
		case EMixerSpeaker::FrontRight:			return TEXT("FrontRight");
		case EMixerSpeaker::FrontCenter:		return TEXT("FrontCenter");
		case EMixerSpeaker::LowFrequency:		return TEXT("LowFrequency");
		case EMixerSpeaker::SurroundLeft:		return TEXT("SurroundLeft");
		case EMixerSpeaker::SurroundRight:		return TEXT("SurroundRight");
		case EMixerSpeaker::SurroundBackLeft:	return TEXT("SurroundBackLeft");
		case EMixerSpeaker::SurroundBackRight:	return TEXT("SurroundBackRight");
		case EMixerSpeaker::TopFrontLeft:		return TEXT("TopFrontLeft");
		case EMixerSpeaker::TopFrontRight:		return TEXT("TopFrontRight");
		case EMixerSpeaker::TopRearLeft:		return TEXT("TopRearLeft");
		case EMixerSpeaker::TopRearRight:		return TEXT("TopRearRight");
		case EMixerSpeaker::BottomFrontLeft:	return TEXT("BottomFrontLeft");
		case EMixerSpeaker::BottomFrontRight:	return TEXT("BottomFrontRight");
		case EMixerSpeaker::BottomRearLeft:		return TEXT("BottomRearLeft");
		case EMixerSpeaker::BottomRearRight:	return TEXT("BottomRearRight");
		case EMixerSpeaker::Unknown:			return TEXT("Unknown");

		default:
			return TEXT("UNSUPPORTED");
		}
	}

	enum class EMixerDataFormat : int32
	{
		Unknown,

		/** Signed Integer 16-bits */
		Int16,

		/** Float 32-bits */
		Float32,

		Unsupported
	};

	/** Structure to hold platform device information **/
	struct FMixerPlatformInfo
	{
		/** The name of the audio device */
		FString Name;

		/** ID of the device. */
		FString DeviceID;

		/** The number of channels supported by the audio device */
		int32 NumChannels;

		/** The number of channels above the base stereo or 7.1 channels supported by the audio device */
		int32 NumDirectOutChannels;

		/** The sample rate of the audio device */
		int32 SampleRate;

		/** The data format of the audio stream */
		EMixerDataFormat Format;

		/** The output channel array of the audio device */
		TArray<EMixerSpeaker> OutputChannelArray;

		/** Whether or not this device is the system default */
		uint8 bIsSystemDefault : 1;

		FMixerPlatformInfo()
		{
			Reset();
		}

		void Reset()
		{
			Name = TEXT("Unknown");
			DeviceID = TEXT("Unknown");
			NumChannels = 0;
			NumDirectOutChannels = 0;
			SampleRate = 0;
			Format = EMixerDataFormat::Unknown;
			OutputChannelArray.Reset();
			bIsSystemDefault = false;
		}
	};

	struct FChannelPositionInfo
	{
		EMixerSpeaker Channel = EMixerSpeaker::DefaultChannel;

		// Horizontal angle of the position of this channel, in radians.
		// Increases as the channel moves clockwise about the listener.
		// Goes from -PI to PI.
		float Azimuth = 0.0f;
		// Vertical angle of the position of this channel, in radians.
		// Increases as the height of the channel increases proportional to the channel's distance from the listener.
		// Goes from -PI to PI.
		float Elevation = 0.0f;

		// distance from the listener. By default, channels are typically on the unit sphere and have a radius of 1.0f.
		// For spatialized inputs, this radius will be expressed in Unreal Units.
		float Radius = 1.0f;
	};
} // namespace
