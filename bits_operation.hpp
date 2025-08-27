#pragma once 

#include <stdint.h>

template <typename T>
inline void SET_BIT(T &var, int pos)
{
	static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
	static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
	// if constexpr (std::is_constant_evaluated())//C++20才支持
	// {
	// 	static_assert(pos >= 0 && pos < (int)(sizeof(T) * 8), "Bit position out of range");
	// }
	var |= (T(1) << pos);
}

template <typename T>
inline void CLR_BIT(T &var, int pos)
{
	static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
	static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
	var &= ~(T(1) << pos);
}

template <typename T>
inline void TGL_BIT(T &var, int pos)
{
	static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
	static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
	var ^= (T(1) << pos);
}

template <typename T>
inline bool READ_BIT(T var, int pos)
{
	static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
	static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
	return (var >> pos) & 1;
}