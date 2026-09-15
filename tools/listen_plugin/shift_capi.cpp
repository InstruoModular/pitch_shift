// GCC side of the C interface in shift_capi.h: wraps the firmware Shift exactly as shiftbench's `current` variant.
#define SHIFT_CAPI_BUILD
#include "shift_capi.h"

#include "instrument/shift.hpp"

#include <new>

struct ShiftHandle
{
    Shift shift;   // Shift() resets itself
};

int shift_capi_version(void) { return 1; }

int shift_block_size(void) { return static_cast<int>(audio_block_size); }

double shift_sample_rate(void) { return static_cast<double>(sample_rate); }

ShiftHandle* shift_create(void) { return new (std::nothrow) ShiftHandle(); }

void shift_destroy(ShiftHandle* handle) { delete handle; }

void shift_reset(ShiftHandle* handle)
{
    if(handle) handle->shift.reset();
}

void shift_set_semitones(ShiftHandle* handle, int semitones)
{
    if(handle) handle->shift.controls.shift_amount = static_cast<int16_t>(semitones);
}

void shift_process(ShiftHandle* handle, const float* in, float* out)
{
    if(!handle) return;
    // Fresh buffers per call: the isl buffer types hold an internal view onto their own storage, so they are
    // filled element-wise rather than copied.
    MonoDspBuffer block_in{}, block_out{};
    for(size_t i = 0; i < audio_block_size; i++) block_in[i] = in[i];
    handle->shift.process(block_in, block_out);
    for(size_t i = 0; i < audio_block_size; i++) out[i] = block_out[i];
}
