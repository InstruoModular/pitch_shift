#pragma once
/* C interface to the firmware Shift (repo-root shift.hpp/shift.cpp).
 *
 * shift.cpp and isl are written for GCC/Clang (__builtin_memcpy, constexpr wrappers around <cmath>) and don't
 * compile under MSVC, while the JUCE plugin is built with MSVC. So shift.cpp is built by the MSYS2 g++ tree
 * (tools/shiftbench, target shift_capi -> build/bench/shift_capi.dll) and the plugin loads it at runtime through
 * this plain C ABI. Rebuild that target after changing shift.hpp/shift.cpp.
 */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(SHIFT_CAPI_BUILD)
#   define SHIFT_CAPI __declspec(dllexport)
#else
#   define SHIFT_CAPI
#endif

typedef struct ShiftHandle ShiftHandle;

SHIFT_CAPI int          shift_capi_version(void);            /* bump on any signature change */
SHIFT_CAPI int          shift_block_size(void);              /* firmware audio_block_size */
SHIFT_CAPI double       shift_sample_rate(void);             /* firmware sample rate (Hz) */
SHIFT_CAPI ShiftHandle* shift_create(void);                  /* constructed and reset */
SHIFT_CAPI void         shift_destroy(ShiftHandle* handle);
SHIFT_CAPI void         shift_reset(ShiftHandle* handle);
SHIFT_CAPI void         shift_set_semitones(ShiftHandle* handle, int semitones);
/* Processes exactly shift_block_size() samples. `in` and `out` must not overlap. */
SHIFT_CAPI void         shift_process(ShiftHandle* handle, const float* in, float* out);

#ifdef __cplusplus
}
#endif
