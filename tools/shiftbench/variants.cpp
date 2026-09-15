// "current" = the firmware shift.cpp at the repo root. Other variants live in variants/*.cpp
// (globbed by CMake) and register themselves the same way.
#include "variant.hpp"
#include "instrument/shift.hpp"

static RegisterShifter reg_current{"current", [] { return std::make_unique<ShiftAdapter<Shift>>(); }};
