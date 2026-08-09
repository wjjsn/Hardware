#pragma once

#include <stdint.h>
namespace Hardware
{
template <typename pwm>
struct Motor
{
	static void init()
	{
		pwm::init();
		pwm::start();
	}
	static void set_speed(float speed)
	{
		if (speed < 0 || speed > 100)
			return;
		pwm::set_compare(static_cast<uint32_t>(speed * static_cast<float>(pwm::get_autoreload()) / 100.0f));
	}
};
}
