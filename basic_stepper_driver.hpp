#pragma once
#include <atomic>
#include <stdint.h>
#include <limits>

namespace Hardware
{
template <typename timer, typename direction_gpio, typename step_gpio,
		  uint16_t motor_steps, uint16_t rpm, uint8_t microsteps>
struct BasicStepper
{
	static_assert(motor_steps > 0, "motor_steps must be non-zero");
	static_assert(rpm > 0, "rpm must be non-zero");
	static_assert(microsteps > 0, "microsteps must be non-zero");

	std::atomic<uint32_t> remaining_edges_{0};

	static void init()
	{
		constexpr uint64_t edge_frequency_64 =
			(static_cast<uint64_t>(motor_steps) * rpm * microsteps * 2U) / 60U;
		static_assert(edge_frequency_64 > 0, "step frequency must be non-zero");
		static_assert(edge_frequency_64 <= std::numeric_limits<uint32_t>::max(), "step frequency is too high");
		constexpr uint32_t edge_frequency = static_cast<uint32_t>(edge_frequency_64);
		direction_gpio::init();
		step_gpio::init();
		timer::init();
		const uint32_t clock = timer::get_clock_frequency();
		timer::set_autoreload(edge_frequency >= clock ? 0U : clock / edge_frequency - 1U);
		timer::set_counter(0);
		timer::start_it();
	}
	bool move(int32_t steps)
	{
		if (steps == 0) return true;
		if (remaining_edges_.load(std::memory_order_acquire) != 0U) return false;
		steps > 0 ? direction_gpio::set() : direction_gpio::clear();
		const uint32_t magnitude = steps == std::numeric_limits<int32_t>::min()
			? (uint32_t{1} << 31U)
			: static_cast<uint32_t>(steps < 0 ? -steps : steps);
		const uint64_t edges = static_cast<uint64_t>(magnitude) * microsteps * 2U;
		const uint32_t desired = edges > std::numeric_limits<uint32_t>::max()
			? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(edges);
		uint32_t expected = 0;
		if (!remaining_edges_.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_relaxed)) return false;
		return true;
	}
	void step_isr()
	{
		uint32_t current = remaining_edges_.load(std::memory_order_acquire);
		while (current > 0U)
		{
			if (remaining_edges_.compare_exchange_weak(current, current - 1U, std::memory_order_relaxed))
			{
				step_gpio::toggle();
				break;
			}
		}
	}
};
}
