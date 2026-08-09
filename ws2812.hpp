#pragma once

#include <atomic>
#include <cstddef>
#include <stdint.h>
#include "bits_operation.hpp"
namespace Hardware
{
template <typename pwm, std::size_t led_count>
class WS2812
{
	static_assert(led_count > 0, "led_count must be non-zero");
	enum class CompareValue : uint16_t
	{
		Code0 = 30,
		Code1 = 60,
		Reset = 0
	};
	uint16_t rgb_buffer_[led_count * 24U + 1U]{};
	std::atomic<std::size_t> step_{led_count * 24U + 1U};

public:
	bool set_one(std::size_t position, uint32_t color)
	{
		if (position >= led_count) return false;
		const uint8_t red = static_cast<uint8_t>(color >> 16U);
		const uint8_t green = static_cast<uint8_t>(color >> 8U);
		const uint8_t blue = static_cast<uint8_t>(color);
		for (std::size_t bit = 0; bit < 8U; ++bit)
		{
			rgb_buffer_[position * 24U + bit] = BIT::READ(green, 7U - bit) ? value(CompareValue::Code1) : value(CompareValue::Code0);
			rgb_buffer_[position * 24U + bit + 8U] = BIT::READ(red, 7U - bit) ? value(CompareValue::Code1) : value(CompareValue::Code0);
			rgb_buffer_[position * 24U + bit + 16U] = BIT::READ(blue, 7U - bit) ? value(CompareValue::Code1) : value(CompareValue::Code0);
		}
		return true;
	}
	bool set_multiple(std::size_t start, std::size_t stop, uint32_t color)
	{
		if (start > stop || stop > led_count) return false;
		for (std::size_t i = start; i < stop; ++i)
		{
			set_one(i, color);
		}
		return true;
	}
	void update()
	{
		rgb_buffer_[led_count * 24U] = value(CompareValue::Reset);
		step_.store(0, std::memory_order_release);
	}
	void update_isr()
	{
		const std::size_t step = step_.fetch_add(1U, std::memory_order_acquire);
		if (step < led_count * 24U + 1U) pwm::set_compare(rgb_buffer_[step]);
		else pwm::set_compare(value(CompareValue::Reset));
	}

private:
	static constexpr uint16_t value(CompareValue value) { return static_cast<uint16_t>(value); }
};
}
