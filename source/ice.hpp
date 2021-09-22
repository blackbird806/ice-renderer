#pragma once

#include <cstdint>

using uint = unsigned int;
using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8 = uint8_t;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8 = int8_t;

using byte = uint8;
using uchar = unsigned char;

#define ICE_NON_COPYABLE_CLASS(Class) Class(Class const&) = delete;\
Class& operator=(Class const&) = delete;

#define ICE_NON_MOVABLE_CLASS(Class) Class(Class&&) = delete;\
	Class& operator=(Class &&) = delete;

#define ICE_NON_DISPATCHABLE_CLASS(Class) ICE_NON_COPYABLE_CLASS(Class) ICE_NON_MOVABLE_CLASS(Class)