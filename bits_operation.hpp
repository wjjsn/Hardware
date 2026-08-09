#pragma once

#include <cstddef>
#include <type_traits>
namespace BIT
{
	template <typename T>
	inline void SET(T &var, std::size_t pos)
	{
		static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
		static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
		// if constexpr (std::is_constant_evaluated())//C++20才支持
		// {
		// 	static_assert(pos >= 0 && pos < (int)(sizeof(T) * 8), "Bit position out of range");
		// }
		if (pos < sizeof(T) * 8U) var |= (T(1) << pos);
	}

	template <typename T>
	inline void CLR(T &var, std::size_t pos)
	{
		static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
		static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
		if (pos < sizeof(T) * 8U) var &= ~(T(1) << pos);
	}

	template <typename T>
	inline void TGL(T &var, std::size_t pos)
	{
		static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
		static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
		if (pos < sizeof(T) * 8U) var ^= (T(1) << pos);
	}

	template <typename T>
	inline bool READ(T var, std::size_t pos)
	{
		static_assert(std::is_unsigned<T>::value, "only works on unsigned integers");
		static_assert(sizeof(T) <= 8, "only supports up to 64-bit types");
		return pos < sizeof(T) * 8U && ((var >> pos) & T(1)) != 0;
	}
}
