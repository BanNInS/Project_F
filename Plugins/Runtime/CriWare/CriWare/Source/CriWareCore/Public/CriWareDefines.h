#pragma once

#if !defined(CRIWARECORE_USE_LLM)
#define CRIWARECORE_USE_LLM 1
#endif

/**
 * Maximum number of channels that can be set using the ini setting
 */
#define ATOM_MAX_AUDIOCHANNELS				64

 /**
  * Length of sound in seconds to be considered as looping forever
  */
#define ATOM_INDEFINITELY_LOOPING_DURATION	10000.0f

  /**
   * Some defaults to help cross platform consistency
   */
#define ATOM_SPEAKER_COUNT					6

#define ATOM_DEFAULT_LOW_FREQUENCY			600.0f
#define ATOM_DEFAULT_MID_FREQUENCY			1000.0f
#define ATOM_DEFAULT_HIGH_FREQUENCY			2000.0f

#define ATOM_MAX_VOLUME						4.0f
#define ATOM_MIN_PITCH						0.4f
#define ATOM_MAX_PITCH						2.0f

#define ATOM_MIN_VOLUME_LINEAR				SMALL_NUMBER
#define ATOM_MIN_VOLUME_DECIBELS			-96.0f

#define ATOM_MIN_SOUND_PRIORITY				0.0f
#define ATOM_MAX_SOUND_PRIORITY				100.0f

//#define ATOM_DEFAULT_SUBTITLE_PRIORITY		10000.0f

/**
* Some filters don't work properly with extreme values, so these are the limits
*/
#define ATOM_MIN_FILTER_GAIN				0.126f
#define ATOM_MAX_FILTER_GAIN				7.94f

#define ATOM_MIN_FILTER_FREQUENCY			24.0f
#define ATOM_MAX_FILTER_FREQUENCY			24000.0f

#define ATOM_MIN_FILTER_BANDWIDTH			0.1f
#define ATOM_MAX_FILTER_BANDWIDTH			2.0f

/**
* DSP don't work properly with extreme values, so these are the limits
*/
#define ATOM_MIN_DSP_CHANNELS				1
#define ATOM_MAX_DSP_CHANNELS				16

#define ATOM_MIN_DSP_SAMPLING_RATE			1
#define ATOM_MAX_DSP_SAMPLING_RATE			384000

/**
 * Debugger and Profiling are available on non-shipping builds
 */
#define ENABLE_ATOM_DEBUG !UE_BUILD_SHIPPING
#define ENABLE_ATOM_MONITOR !UE_BUILD_SHIPPING

#define ATOM_MONITOR_DEFAULT_PORT			2000

/**
 * Typed identifier for AtomRuntime ID
 */
using FAtomRuntimeId = uint32;
