#pragma once

#include "ProfilingDebugging/CpuProfilerTrace.h"

#if !defined(ATOM_MIXER_CPUPROFILERTRACE_ENABLED)
#if CPUPROFILERTRACE_ENABLED
#define ATOM_MIXER_CPUPROFILERTRACE_ENABLED 1
#else
#define ATOM_MIXER_CPUPROFILERTRACE_ENABLED 0
#endif
#endif

#if ATOM_MIXER_CPUPROFILERTRACE_ENABLED
// AtomRuntime CPU profiler trace enabled

#define ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(Name) \
    TRACE_CPUPROFILER_EVENT_SCOPE(Name)

#else
// AtomRuntime CPU profiler trace *not* enabled

#define ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(Name)

#endif
