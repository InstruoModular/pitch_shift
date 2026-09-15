#pragma once
// Variant registry: every pitch-shifter implementation benchmarked by shiftbench exposes the
// firmware API (Controls{shift_amount}, reset(), process(const MonoDspBuffer&, MonoDspBuffer&))
// and is registered by name. Tuning constants a variant wants swept are exposed via set_param.
#include "engine/constant.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>

struct IShifter
{
    virtual ~IShifter() = default;
    virtual void reset() = 0;
    virtual void set_shift(int semitones) = 0;
    /** @return false if the variant has no such tuning parameter. */
    virtual bool set_param(const std::string&, double) { return false; }
    virtual void process(const MonoDspBuffer& in, MonoDspBuffer& out) = 0;
};

/** Wraps any class with the firmware Shift API. */
template<class T>
struct ShiftAdapter : IShifter
{
    T impl;
    void reset() override { impl.reset(); }
    void set_shift(int s) override { impl.controls.shift_amount = static_cast<int16_t>(s); }
    /** Specialise per variant (template<> bool ShiftAdapter<T>::set_param) to expose tuning constants. */
    bool set_param(const std::string&, double) override { return false; }
    void process(const MonoDspBuffer& in, MonoDspBuffer& out) override { impl.process(in, out); }
};

using ShifterFactory = std::function<std::unique_ptr<IShifter>()>;

inline std::map<std::string, ShifterFactory>& shifter_registry()
{
    static std::map<std::string, ShifterFactory> r;
    return r;
}

/** `static RegisterShifter reg{"name", []{ return std::make_unique<ShiftAdapter<MyShift>>(); }};` */
struct RegisterShifter
{
    RegisterShifter(const char* name, ShifterFactory f) { shifter_registry()[name] = std::move(f); }
};
